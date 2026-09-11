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

#include "mysql-driver.h"

#include <dbshell/client.h>
#include <dbshell/dbshell.h>
#include <dbshell/settings.h>
#include <dbshell/table.h>
#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <guacamole/timestamp.h>

#include <errmsg.h>
#include <mysql.h>
#include <mysqld_error.h>

#include <errno.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/**
 * The number of milliseconds to wait for socket activity before rechecking
 * whether the session is still running, while a statement is executing.
 */
#define GUAC_MYSQL_POLL_INTERVAL 500

/**
 * The number of seconds to wait for the secondary connection used to
 * deliver KILL QUERY requests.
 */
#define GUAC_MYSQL_CANCEL_TIMEOUT 5

/**
 * MySQL client error codes denoting loss of the server connection.
 *
 * @param errnum
 *     The MySQL client error code to test.
 *
 * @return
 *     Non-zero if the given error code denotes loss of the server
 *     connection, zero otherwise.
 */
static int guac_mysql_is_connection_error(unsigned int errnum) {
    return errnum == CR_SERVER_GONE_ERROR
        || errnum == CR_SERVER_LOST;
}

/**
 * Waits for the socket activity requested by the MariaDB non-blocking API,
 * rechecking at a fixed interval whether the session's client is still
 * running so that a dead or unresponsive database server can never hang
 * session teardown.
 *
 * @param session
 *     The session whose statement is executing.
 *
 * @param mysql
 *     The connection being waited on.
 *
 * @param status
 *     The combination of MYSQL_WAIT_* flags requested by the non-blocking
 *     API.
 *
 * @return
 *     The combination of MYSQL_WAIT_* flags describing the activity which
 *     occurred, or -1 if the client is no longer running and the operation
 *     should be abandoned.
 */
static int guac_mysql_wait(guac_dbshell_session* session, MYSQL* mysql,
        int status) {

    while (session->client->state == GUAC_CLIENT_RUNNING) {

        struct pollfd pfd = {
            .fd = mysql_get_socket(mysql),
            .events = 0
        };

        if (status & MYSQL_WAIT_READ)
            pfd.events |= POLLIN;

        if (status & MYSQL_WAIT_WRITE)
            pfd.events |= POLLOUT;

        if (status & MYSQL_WAIT_EXCEPT)
            pfd.events |= POLLPRI;

        int result = poll(&pfd, 1, GUAC_MYSQL_POLL_INTERVAL);

        /* Poll errors other than interruption are fatal */
        if (result < 0 && errno != EINTR)
            return -1;

        if (result > 0) {

            int occurred = 0;

            if (pfd.revents & POLLIN)
                occurred |= MYSQL_WAIT_READ;

            if (pfd.revents & POLLOUT)
                occurred |= MYSQL_WAIT_WRITE;

            if (pfd.revents & (POLLPRI | POLLERR | POLLHUP))
                occurred |= MYSQL_WAIT_EXCEPT;

            return occurred;

        }

        /* No activity yet; recheck client state and continue waiting */

    }

    return -1;

}

/**
 * Renders the current result set of the given connection to the session's
 * terminal as a table.
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
static unsigned long guac_mysql_render_result(guac_dbshell_session* session,
        MYSQL_RES* result) {

    unsigned int num_fields = mysql_num_fields(result);
    MYSQL_FIELD* fields = mysql_fetch_fields(result);

    guac_dbshell_table* table = guac_dbshell_table_begin(session->term,
            num_fields);
    if (table == NULL)
        return 0;

    /* Assign column headers, right-aligning numeric columns */
    for (unsigned int i = 0; i < num_fields; i++)
        guac_dbshell_table_set_header(table, i, fields[i].name,
                IS_NUM(fields[i].type));

    /* Render all rows */
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(result)) != NULL)
        guac_dbshell_table_add_row(table, (const char**) row);

    return guac_dbshell_table_end(table);

}

/**
 * Renders the MySQL error which just occurred on the given connection to
 * the session's terminal, in the style of the mysql CLI.
 *
 * @param session
 *     The session to render to.
 *
 * @param mysql
 *     The connection the error occurred on.
 */
static void guac_mysql_print_error(guac_dbshell_session* session,
        MYSQL* mysql) {
    guac_dbshell_println(session, "ERROR %u (%s): %s", mysql_errno(mysql),
            mysql_sqlstate(mysql), mysql_error(mysql));
}

/**
 * Applies the SSL/TLS settings requested by the connection arguments to
 * the given MySQL connection handle prior to connecting.
 *
 * @param mysql
 *     The connection handle to configure.
 *
 * @param extra
 *     The database-specific settings of the connection.
 */
static void guac_mysql_apply_ssl(MYSQL* mysql,
        guac_mysql_extra_settings* extra) {

    /* Apply certificate authority, if given */
    if (extra->ssl_ca_file != NULL)
        mysql_ssl_set(mysql, NULL, NULL, extra->ssl_ca_file, NULL, NULL);

    if (extra->ssl_mode == NULL)
        return;

    /* Require encryption for all modes except "disabled" and
     * "preferred" */
    my_bool enforce = strcmp(extra->ssl_mode, "required") == 0
        || strcmp(extra->ssl_mode, "verify-ca") == 0
        || strcmp(extra->ssl_mode, "verify-identity") == 0;

    if (enforce)
        mysql_options(mysql, MYSQL_OPT_SSL_ENFORCE, &enforce);

    /* Verify the server certificate for the verification modes */
    my_bool verify = strcmp(extra->ssl_mode, "verify-ca") == 0
        || strcmp(extra->ssl_mode, "verify-identity") == 0;

    if (verify)
        mysql_options(mysql, MYSQL_OPT_SSL_VERIFY_SERVER_CERT, &verify);

}

/**
 * Establishes the connection to the MySQL server described by the settings
 * of the given session. This handler implements
 * guac_dbshell_driver.connect_handler.
 *
 * @param session
 *     The session on whose behalf the connection is being established.
 *
 * @return
 *     Zero on success, non-zero on failure.
 */
static int guac_mysql_connect(guac_dbshell_session* session) {

    guac_dbshell_settings* settings =
        (guac_dbshell_settings*) session->settings;

    guac_dbshell_client* dbshell_client =
        (guac_dbshell_client*) session->client->data;
    guac_mysql_extra_settings* extra =
        (guac_mysql_extra_settings*) dbshell_client->extra_settings;

    /* A username is always meaningful to MySQL; prompt if missing */
    guac_dbshell_prompt_credentials(session, true, false);

    /* Attempt connection, prompting for a password and retrying once (on
     * a fresh handle) if access is denied without one */
    MYSQL* mysql;
    for (;;) {

        mysql = mysql_init(NULL);
        if (mysql == NULL) {
            guac_dbshell_println(session, "ERROR: Unable to initialize "
                    "the MySQL client library.");
            return 1;
        }

        /* Enable the non-blocking API for all subsequent operations */
        mysql_options(mysql, MYSQL_OPT_NONBLOCK, 0);

        /* Bound the connection attempt */
        unsigned int timeout = settings->timeout;
        mysql_options(mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

        /* Never allow the server to read files local to guacd */
        unsigned int local_infile = 0;
        mysql_options(mysql, MYSQL_OPT_LOCAL_INFILE, &local_infile);

        guac_mysql_apply_ssl(mysql, extra);

        if (mysql_real_connect(mysql, settings->hostname,
                    settings->username, settings->password,
                    settings->database, settings->port, NULL,
                    CLIENT_MULTI_RESULTS) != NULL)
            break;

        /* Prompt for the missing password and retry on access denied */
        if (mysql_errno(mysql) == ER_ACCESS_DENIED_ERROR
                && settings->password == NULL) {
            mysql_close(mysql);
            guac_dbshell_prompt_credentials(session, false, true);
            continue;
        }

        guac_mysql_print_error(session, mysql);
        mysql_close(mysql);
        return 1;

    }

    /* All terminal I/O is UTF-8 */
    if (mysql_set_character_set(mysql, "utf8mb4"))
        mysql_set_character_set(mysql, "utf8");

    guac_mysql_data* data = guac_mem_zalloc(sizeof(guac_mysql_data));
    data->mysql = mysql;
    data->thread_id = mysql_thread_id(mysql);
    session->driver_data = data;

    /* Greet the user in the style of the mysql CLI */
    guac_dbshell_println(session, "Connected to %s (server version %s). "
            "Statements end with ';'. Type \\h for help.",
            mysql_get_host_info(mysql), mysql_get_server_info(mysql));
    guac_dbshell_println(session, "");

    return 0;

}

/**
 * Closes the connection to the MySQL server. This handler implements
 * guac_dbshell_driver.disconnect_handler.
 *
 * @param session
 *     The session whose connection should be closed.
 */
static void guac_mysql_disconnect(guac_dbshell_session* session) {

    guac_mysql_data* data = (guac_mysql_data*) session->driver_data;
    if (data == NULL)
        return;

    mysql_close(data->mysql);
    guac_mem_free(data);
    session->driver_data = NULL;

}

/**
 * Executes the given statement against the MySQL server, rendering all
 * result sets and errors to the session's terminal. This handler
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
static int guac_mysql_execute(guac_dbshell_session* session,
        const char* statement) {

    guac_mysql_data* data = (guac_mysql_data*) session->driver_data;
    MYSQL* mysql = data->mysql;

    guac_timestamp started = guac_timestamp_current();

    /* Send the statement, waiting via the non-blocking API so that the
     * wait can be abandoned if the session ends */
    int error;
    int status = mysql_real_query_start(&error, mysql, statement,
            strlen(statement));

    while (status) {

        status = guac_mysql_wait(session, mysql, status);
        if (status < 0)
            return GUAC_DBSHELL_EXECUTE_LOST;

        status = mysql_real_query_cont(&error, mysql, status);

    }

    if (error) {

        guac_mysql_print_error(session, mysql);

        if (guac_mysql_is_connection_error(mysql_errno(mysql)))
            return GUAC_DBSHELL_EXECUTE_LOST;

        return GUAC_DBSHELL_EXECUTE_OK;

    }

    /* Process all result sets produced by the statement */
    for (;;) {

        /* Retrieve the next result set */
        MYSQL_RES* result;
        status = mysql_store_result_start(&result, mysql);

        while (status) {

            status = guac_mysql_wait(session, mysql, status);
            if (status < 0)
                return GUAC_DBSHELL_EXECUTE_LOST;

            status = mysql_store_result_cont(&result, mysql, status);

        }

        long millis = guac_timestamp_current() - started;

        /* Render the rows of the result set, if one was produced */
        if (result != NULL) {
            unsigned long rows = guac_mysql_render_result(session, result);
            mysql_free_result(result);
            guac_dbshell_print_summary(session, rows, 0, millis);
        }

        /* Otherwise report either the affected row count or the error */
        else if (mysql_field_count(mysql) == 0) {
            guac_dbshell_print_summary(session,
                    (unsigned long) mysql_affected_rows(mysql), 1, millis);
        }

        else {

            guac_mysql_print_error(session, mysql);

            if (guac_mysql_is_connection_error(mysql_errno(mysql)))
                return GUAC_DBSHELL_EXECUTE_LOST;

            return GUAC_DBSHELL_EXECUTE_OK;

        }

        /* Advance to the next result set, if any */
        int next;
        status = mysql_next_result_start(&next, mysql);

        while (status) {

            status = guac_mysql_wait(session, mysql, status);
            if (status < 0)
                return GUAC_DBSHELL_EXECUTE_LOST;

            status = mysql_next_result_cont(&next, mysql, status);

        }

        /* No more result sets */
        if (next != 0)
            break;

    }

    guac_dbshell_println(session, "");
    return GUAC_DBSHELL_EXECUTE_OK;

}

/**
 * Interrupts the statement currently executing on the given session by
 * issuing KILL QUERY over a separate, short-lived connection, exactly as
 * the mysql CLI does. This handler implements
 * guac_dbshell_driver.cancel_handler and is invoked from a thread other
 * than the session thread.
 *
 * @param session
 *     The session whose current statement should be interrupted.
 */
static void guac_mysql_cancel(guac_dbshell_session* session) {

    guac_dbshell_settings* settings =
        (guac_dbshell_settings*) session->settings;
    guac_mysql_data* data = (guac_mysql_data*) session->driver_data;

    if (data == NULL)
        return;

    MYSQL* kill_connection = mysql_init(NULL);
    if (kill_connection == NULL)
        return;

    unsigned int timeout = GUAC_MYSQL_CANCEL_TIMEOUT;
    mysql_options(kill_connection, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

    /* Deliver KILL QUERY for the session's server-side thread */
    if (mysql_real_connect(kill_connection, settings->hostname,
                settings->username, settings->password, NULL,
                settings->port, NULL, 0) != NULL) {

        char kill_query[64];
        snprintf(kill_query, sizeof(kill_query), "KILL QUERY %lu",
                data->thread_id);
        mysql_real_query(kill_connection, kill_query, strlen(kill_query));

    }

    mysql_close(kill_connection);

}

const guac_dbshell_driver guac_mysql_driver = {

    .name = "mysql",
    .dialect = GUAC_DBSHELL_DIALECT_MYSQL,

    .connect_handler = guac_mysql_connect,
    .disconnect_handler = guac_mysql_disconnect,
    .execute_handler = guac_mysql_execute,
    .cancel_handler = guac_mysql_cancel

};
