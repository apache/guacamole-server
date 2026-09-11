/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "config.h"

#include "postgresql.h"

#include <dbshell/client.h>
#include <dbshell/dbshell.h>
#include <dbshell/settings.h>
#include <dbshell/table.h>
#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <guacamole/string.h>
#include <guacamole/timestamp.h>

#include <libpq-fe.h>

#include <errno.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * The number of milliseconds to wait for socket activity before rechecking
 * whether the session is still running, while a statement is executing.
 */
#define GUAC_POSTGRESQL_POLL_INTERVAL 500

/**
 * The maximum number of connection parameters passed to
 * PQconnectdbParams(), including the terminating NULL entry.
 */
#define GUAC_POSTGRESQL_MAX_PARAMS 16

/**
 * Returns whether values of the given PostgreSQL type OID are numeric and
 * should be right-aligned when rendered.
 *
 * @param type
 *     The type OID to test.
 *
 * @return
 *     Non-zero if the type is numeric, zero otherwise.
 */
static int guac_postgresql_is_numeric(Oid type) {

    /* The built-in OIDs of int8, int2, int4, oid, float4, float8, and
     * numeric respectively */
    return type == 20 || type == 21 || type == 23 || type == 26
        || type == 700 || type == 701 || type == 1700;

}

/**
 * Notice processor invoked by libpq for each notice or warning raised by
 * the server, rendering the notice to the session's terminal.
 *
 * @param arg
 *     The guac_dbshell_session of the connection, as registered with
 *     PQsetNoticeProcessor().
 *
 * @param message
 *     The notice message, including any trailing newline.
 */
static void guac_postgresql_notice_processor(void* arg,
        const char* message) {

    guac_dbshell_session* session = (guac_dbshell_session*) arg;

    /* Trim the trailing newline supplied by libpq */
    int length = strlen(message);
    while (length > 0 && (message[length - 1] == '\n'
                || message[length - 1] == '\r'))
        length--;

    guac_dbshell_println(session, "%.*s", length, message);

}

/**
 * Waits for read activity on the given connection's socket, rechecking at
 * a fixed interval whether the session's client is still running so that a
 * dead or unresponsive database server can never hang session teardown.
 *
 * @param session
 *     The session whose statement is executing.
 *
 * @param connection
 *     The connection being waited on.
 *
 * @return
 *     Zero if read activity occurred, non-zero if the client is no longer
 *     running and the operation should be abandoned.
 */
static int guac_postgresql_wait(guac_dbshell_session* session,
        PGconn* connection) {

    while (session->client->state == GUAC_CLIENT_RUNNING) {

        struct pollfd pfd = {
            .fd = PQsocket(connection),
            .events = POLLIN
        };

        int result = poll(&pfd, 1, GUAC_POSTGRESQL_POLL_INTERVAL);

        /* Poll errors other than interruption are fatal */
        if (result < 0 && errno != EINTR)
            return 1;

        if (result > 0)
            return 0;

        /* No activity yet; recheck client state and continue waiting */

    }

    return 1;

}

/**
 * Renders the given result set to the session's terminal as a table.
 *
 * @param session
 *     The session to render to.
 *
 * @param result
 *     The result set to render.
 *
 * @return
 *     The number of rows rendered.
 */
static unsigned long guac_postgresql_render_result(
        guac_dbshell_session* session, PGresult* result) {

    int num_fields = PQnfields(result);
    int num_rows = PQntuples(result);

    guac_dbshell_table* table = guac_dbshell_table_begin(session->term,
            num_fields);
    if (table == NULL)
        return 0;

    /* Assign column headers, right-aligning numeric columns */
    for (int i = 0; i < num_fields; i++)
        guac_dbshell_table_set_header(table, i, PQfname(result, i),
                guac_postgresql_is_numeric(PQftype(result, i)));

    /* Render all rows */
    const char** values = guac_mem_alloc(guac_mem_ckd_mul_or_die(
                num_fields, sizeof(char*)));

    for (int row = 0; row < num_rows; row++) {

        for (int col = 0; col < num_fields; col++) {
            if (PQgetisnull(result, row, col))
                values[col] = NULL;
            else
                values[col] = PQgetvalue(result, row, col);
        }

        guac_dbshell_table_add_row(table, values);

    }

    guac_mem_free(values);
    return guac_dbshell_table_end(table);

}

/**
 * Renders the given PostgreSQL error message to the session's terminal,
 * trimming any trailing newlines.
 *
 * @param session
 *     The session to render to.
 *
 * @param message
 *     The error message to render, or NULL if no message is available.
 */
static void guac_postgresql_print_error(guac_dbshell_session* session,
        const char* message) {

    if (message == NULL || *message == '\0') {
        guac_dbshell_println(session, "ERROR: (no details available)");
        return;
    }

    int length = strlen(message);
    while (length > 0 && (message[length - 1] == '\n'
                || message[length - 1] == '\r'))
        length--;

    guac_dbshell_println(session, "%.*s", length, message);

}

/**
 * Attempts a single connection to the PostgreSQL server described by the
 * given settings.
 *
 * @param settings
 *     The common settings of the session.
 *
 * @param extra
 *     The PostgreSQL-specific settings of the session.
 *
 * @return
 *     The libpq connection object, which must be tested with PQstatus()
 *     and eventually freed with PQfinish().
 */
static PGconn* guac_postgresql_attempt(guac_dbshell_settings* settings,
        guac_postgresql_extra_settings* extra) {

    const char* keywords[GUAC_POSTGRESQL_MAX_PARAMS];
    const char* values[GUAC_POSTGRESQL_MAX_PARAMS];
    int param = 0;

    char port[GUAC_USHORT_STRING_BUFSIZE];
    guac_itoa_safe(port, sizeof(port), settings->port);

    char timeout[32];
    snprintf(timeout, sizeof(timeout), "%i", settings->timeout);

    keywords[param] = "host";            values[param++] = settings->hostname;
    keywords[param] = "port";            values[param++] = port;
    keywords[param] = "connect_timeout"; values[param++] = timeout;
    keywords[param] = "client_encoding"; values[param++] = "UTF8";

    if (settings->username != NULL) {
        keywords[param] = "user";
        values[param++] = settings->username;
    }

    if (settings->password != NULL) {
        keywords[param] = "password";
        values[param++] = settings->password;
    }

    if (settings->database != NULL) {
        keywords[param] = "dbname";
        values[param++] = settings->database;
    }

    if (extra->ssl_mode != NULL) {
        keywords[param] = "sslmode";
        values[param++] = extra->ssl_mode;
    }

    if (extra->ssl_ca_file != NULL) {
        keywords[param] = "sslrootcert";
        values[param++] = extra->ssl_ca_file;
    }

    keywords[param] = NULL;
    values[param] = NULL;

    return PQconnectdbParams(keywords, values, 0);

}

/**
 * Establishes the connection to the PostgreSQL server described by the
 * settings of the given session. This handler implements
 * guac_dbshell_driver.connect_handler.
 *
 * @param session
 *     The session on whose behalf the connection is being established.
 *
 * @return
 *     Zero on success, non-zero on failure.
 */
static int guac_postgresql_connect(guac_dbshell_session* session) {

    guac_dbshell_settings* settings =
        (guac_dbshell_settings*) session->settings;

    guac_dbshell_client* dbshell_client =
        (guac_dbshell_client*) session->client->data;
    guac_postgresql_extra_settings* extra =
        (guac_postgresql_extra_settings*) dbshell_client->extra_settings;

    /* PostgreSQL requires a username (there is no guacd-side identity to
     * fall back on); prompt if missing */
    guac_dbshell_prompt_credentials(session, true, false);

    /* Attempt connection, prompting for a password and retrying once if
     * the server demands one, as psql does */
    PGconn* connection = guac_postgresql_attempt(settings, extra);

    if (PQstatus(connection) != CONNECTION_OK
            && PQconnectionNeedsPassword(connection)
            && settings->password == NULL) {

        PQfinish(connection);
        guac_dbshell_prompt_credentials(session, false, true);
        connection = guac_postgresql_attempt(settings, extra);

    }

    if (PQstatus(connection) != CONNECTION_OK) {
        guac_postgresql_print_error(session,
                PQerrorMessage(connection));
        PQfinish(connection);
        return 1;
    }

    guac_postgresql_data* data =
        guac_mem_zalloc(sizeof(guac_postgresql_data));
    data->connection = connection;
    data->cancel = PQgetCancel(connection);
    session->driver_data = data;

    /* Render server notices to the terminal as they arrive */
    PQsetNoticeProcessor(connection, guac_postgresql_notice_processor,
            session);

    /* Greet the user in the style of psql */
    guac_dbshell_println(session, "Connected to %s (server version %s). "
            "Statements end with ';'. Type \\h for help.",
            PQdb(connection),
            PQparameterStatus(connection, "server_version"));
    guac_dbshell_println(session, "");

    return 0;

}

/**
 * Closes the connection to the PostgreSQL server. This handler implements
 * guac_dbshell_driver.disconnect_handler.
 *
 * @param session
 *     The session whose connection should be closed.
 */
static void guac_postgresql_disconnect(guac_dbshell_session* session) {

    guac_postgresql_data* data =
        (guac_postgresql_data*) session->driver_data;
    if (data == NULL)
        return;

    PQfreeCancel(data->cancel);
    PQfinish(data->connection);
    guac_mem_free(data);
    session->driver_data = NULL;

}

/**
 * Executes the given statement against the PostgreSQL server, rendering
 * all result sets and errors to the session's terminal. This handler
 * implements guac_dbshell_driver.execute_handler.
 *
 * @param session
 *     The session on whose behalf the statement is being executed.
 *
 * @param statement
 *     The statement to execute.
 *
 * @return
 *     GUAC_DBSHELL_EXECUTE_OK if the session may continue, or
 *     GUAC_DBSHELL_EXECUTE_LOST if the connection has been lost.
 */
static int guac_postgresql_execute(guac_dbshell_session* session,
        const char* statement) {

    guac_postgresql_data* data =
        (guac_postgresql_data*) session->driver_data;
    PGconn* connection = data->connection;

    guac_timestamp started = guac_timestamp_current();

    /* Dispatch the statement */
    if (!PQsendQuery(connection, statement)) {
        guac_postgresql_print_error(session,
                PQerrorMessage(connection));
        return PQstatus(connection) == CONNECTION_OK
            ? GUAC_DBSHELL_EXECUTE_OK : GUAC_DBSHELL_EXECUTE_LOST;
    }

    bool abandoned = false;

    /* Process every result produced by the statement */
    for (;;) {

        /* Wait (interruptibly) until the next result is available */
        while (!abandoned && PQisBusy(connection)) {

            if (guac_postgresql_wait(session, connection)
                    || !PQconsumeInput(connection)) {
                abandoned = true;
                break;
            }

        }

        /* Even once abandoned, drain any pending results so the
         * connection object can be freed cleanly */
        PGresult* result = PQgetResult(connection);
        if (result == NULL)
            break;

        if (abandoned) {
            PQclear(result);
            continue;
        }

        long millis = guac_timestamp_current() - started;

        switch (PQresultStatus(result)) {

            /* Render returned rows as a table */
            case PGRES_TUPLES_OK: {
                unsigned long rows = guac_postgresql_render_result(session,
                        result);
                guac_dbshell_print_summary(session, rows, 0, millis);
                break;
            }

            /* Report affected rows for statements returning no rows */
            case PGRES_COMMAND_OK: {

                const char* affected = PQcmdTuples(result);
                if (affected != NULL && *affected != '\0')
                    guac_dbshell_print_summary(session,
                            strtoul(affected, NULL, 10), 1, millis);
                else
                    guac_dbshell_println(session, "%s",
                            PQcmdStatus(result));

                break;
            }

            case PGRES_EMPTY_QUERY:
                break;

            /* All other statuses are errors */
            default:
                guac_postgresql_print_error(session,
                        PQresultErrorMessage(result));
                break;

        }

        PQclear(result);

    }

    guac_dbshell_println(session, "");

    /* The connection may have been lost while executing */
    if (abandoned || PQstatus(connection) != CONNECTION_OK)
        return GUAC_DBSHELL_EXECUTE_LOST;

    return GUAC_DBSHELL_EXECUTE_OK;

}

/**
 * Interrupts the statement currently executing on the given session using
 * libpq's thread-safe cancellation interface. This handler implements
 * guac_dbshell_driver.cancel_handler.
 *
 * @param session
 *     The session whose current statement should be interrupted.
 */
static void guac_postgresql_cancel(guac_dbshell_session* session) {

    guac_postgresql_data* data =
        (guac_postgresql_data*) session->driver_data;

    if (data == NULL || data->cancel == NULL)
        return;

    char error[256];
    PQcancel(data->cancel, error, sizeof(error));

}

const guac_dbshell_driver guac_postgresql_driver = {

    .name = "postgresql",
    .dialect = GUAC_DBSHELL_DIALECT_PGSQL,

    .connect_handler = guac_postgresql_connect,
    .disconnect_handler = guac_postgresql_disconnect,
    .execute_handler = guac_postgresql_execute,
    .cancel_handler = guac_postgresql_cancel

};
