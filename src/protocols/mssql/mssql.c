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

#include "mssql.h"

#include <dbshell/buffer.h>
#include <dbshell/client.h>
#include <dbshell/dbshell.h>
#include <dbshell/settings.h>
#include <dbshell/table.h>
#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <guacamole/string.h>
#include <guacamole/timestamp.h>

#include <ctpublic.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/**
 * The maximum number of bytes of a single column value fetched from the
 * server, including the null terminator. Longer values are truncated by
 * the client library.
 */
#define GUAC_MSSQL_MAX_VALUE_LENGTH 8192

/**
 * Returns whether the given CT-Library datatype holds numeric values and
 * should be right-aligned when rendered.
 *
 * @param datatype
 *     The CS_*_TYPE constant to test.
 *
 * @return
 *     Non-zero if the datatype is numeric, zero otherwise.
 */
static int guac_mssql_is_numeric(CS_INT datatype) {
    return datatype == CS_TINYINT_TYPE
        || datatype == CS_SMALLINT_TYPE
        || datatype == CS_INT_TYPE
        || datatype == CS_BIGINT_TYPE
        || datatype == CS_REAL_TYPE
        || datatype == CS_FLOAT_TYPE
        || datatype == CS_MONEY_TYPE
        || datatype == CS_MONEY4_TYPE
        || datatype == CS_NUMERIC_TYPE
        || datatype == CS_DECIMAL_TYPE;
}

/**
 * Returns the session associated with the given CT-Library connection, as
 * previously stored via the CS_USERDATA property.
 *
 * @param connection
 *     The connection whose session should be retrieved.
 *
 * @return
 *     The session associated with the connection, or NULL if no session
 *     has been stored.
 */
static guac_dbshell_session* guac_mssql_get_session(
        CS_CONNECTION* connection) {

    guac_dbshell_session* session = NULL;
    CS_INT length = 0;

    if (ct_con_props(connection, CS_GET, CS_USERDATA, &session,
                sizeof(session), &length) != CS_SUCCEED)
        return NULL;

    return session;

}

/**
 * Client message callback registered with CT-Library, rendering client-side
 * errors (connection failures, protocol errors, etc.) to the terminal of
 * the session associated with the connection.
 *
 * @param context
 *     The CT-Library context.
 *
 * @param connection
 *     The connection the message applies to.
 *
 * @param message
 *     The client message.
 *
 * @return
 *     Always CS_SUCCEED.
 */
static CS_RETCODE guac_mssql_client_message(CS_CONTEXT* context,
        CS_CONNECTION* connection, CS_CLIENTMSG* message) {

    guac_dbshell_session* session = guac_mssql_get_session(connection);
    if (session != NULL)
        guac_dbshell_println(session, "ERROR: %s", message->msgstring);

    return CS_SUCCEED;

}

/**
 * Server message callback registered with CT-Library, rendering messages
 * raised by the SQL Server (errors, PRINT output, informational messages)
 * to the terminal of the session associated with the connection.
 *
 * @param context
 *     The CT-Library context.
 *
 * @param connection
 *     The connection the message applies to.
 *
 * @param message
 *     The server message.
 *
 * @return
 *     Always CS_SUCCEED.
 */
static CS_RETCODE guac_mssql_server_message(CS_CONTEXT* context,
        CS_CONNECTION* connection, CS_SERVERMSG* message) {

    guac_dbshell_session* session = guac_mssql_get_session(connection);
    if (session == NULL)
        return CS_SUCCEED;

    /* Severity 0 messages are informational output (e.g. PRINT) */
    if (message->severity == 0)
        guac_dbshell_println(session, "%s", message->text);
    else
        guac_dbshell_println(session, "Msg %ld, Level %ld, State %ld: %s",
                (long) message->msgnumber, (long) message->severity,
                (long) message->state, message->text);

    return CS_SUCCEED;

}

/**
 * Renders the current row result set of the given command to the session's
 * terminal as a table.
 *
 * @param session
 *     The session to render to.
 *
 * @param command
 *     The command whose row results should be rendered.
 *
 * @return
 *     The number of rows rendered, or -1 if results could not be
 *     processed.
 */
static long guac_mssql_render_rows(guac_dbshell_session* session,
        CS_COMMAND* command) {

    CS_INT num_columns = 0;
    if (ct_res_info(command, CS_NUMDATA, &num_columns, CS_UNUSED, NULL)
            != CS_SUCCEED || num_columns <= 0)
        return -1;

    /* Per-column fetch buffers, bound once for all rows */
    char** values = guac_mem_zalloc(guac_mem_ckd_mul_or_die(num_columns,
                sizeof(char*)));
    CS_SMALLINT* indicators = guac_mem_zalloc(guac_mem_ckd_mul_or_die(
                num_columns, sizeof(CS_SMALLINT)));

    guac_dbshell_table* table = guac_dbshell_table_begin(session->term,
            num_columns);

    /* Describe and bind each column as null-terminated text */
    for (CS_INT i = 0; i < num_columns; i++) {

        CS_DATAFMT column;
        memset(&column, 0, sizeof(column));
        ct_describe(command, i + 1, &column);

        guac_dbshell_table_set_header(table, i, column.name,
                guac_mssql_is_numeric(column.datatype));

        values[i] = guac_mem_alloc(GUAC_MSSQL_MAX_VALUE_LENGTH);

        CS_DATAFMT binding;
        memset(&binding, 0, sizeof(binding));
        binding.datatype = CS_CHAR_TYPE;
        binding.format = CS_FMT_NULLTERM;
        binding.maxlength = GUAC_MSSQL_MAX_VALUE_LENGTH;
        binding.count = 1;

        ct_bind(command, i + 1, &binding, values[i], NULL,
                &indicators[i]);

    }

    /* Fetch and render all rows */
    long rows = 0;
    CS_RETCODE result;
    CS_INT fetched;

    const char** row = guac_mem_zalloc(guac_mem_ckd_mul_or_die(num_columns,
                sizeof(char*)));

    while ((result = ct_fetch(command, CS_UNUSED, CS_UNUSED, CS_UNUSED,
                    &fetched)) == CS_SUCCEED
            || result == CS_ROW_FAIL) {

        if (result == CS_ROW_FAIL)
            continue;

        for (CS_INT i = 0; i < num_columns; i++)
            row[i] = (indicators[i] == -1) ? NULL : values[i];

        guac_dbshell_table_add_row(table, row);
        rows++;

    }

    guac_dbshell_table_end(table);

    for (CS_INT i = 0; i < num_columns; i++)
        guac_mem_free(values[i]);
    guac_mem_free(values);
    guac_mem_free(indicators);
    guac_mem_free(row);

    /* A fetch result other than end-of-data denotes failure */
    if (result != CS_END_DATA)
        return -1;

    return rows;

}

/**
 * Establishes the connection to the SQL Server described by the settings
 * of the given session. This handler implements
 * guac_dbshell_driver.connect_handler.
 *
 * @param session
 *     The session on whose behalf the connection is being established.
 *
 * @return
 *     Zero on success, non-zero on failure.
 */
static int guac_mssql_connect(guac_dbshell_session* session) {

    guac_dbshell_settings* settings =
        (guac_dbshell_settings*) session->settings;

    guac_dbshell_client* dbshell_client =
        (guac_dbshell_client*) session->client->data;
    guac_mssql_extra_settings* extra =
        (guac_mssql_extra_settings*) dbshell_client->extra_settings;

    /* SQL Server authentication always involves a username and password;
     * prompt for whichever is missing */
    guac_dbshell_prompt_credentials(session, true, true);

    /* Allocate and initialize the library context */
    CS_CONTEXT* context = NULL;
    if (cs_ctx_alloc(CS_VERSION_100, &context) != CS_SUCCEED
            || ct_init(context, CS_VERSION_100) != CS_SUCCEED) {

        guac_dbshell_println(session, "ERROR: Unable to initialize the "
                "FreeTDS CT-Library context.");

        if (context != NULL)
            cs_ctx_drop(context);

        return 1;

    }

    /* Bound the connection attempt */
    CS_INT timeout = settings->timeout;
    ct_config(context, CS_SET, CS_LOGIN_TIMEOUT, &timeout, CS_UNUSED,
            NULL);

    /* Route client and server messages to the terminal */
    ct_callback(context, NULL, CS_SET, CS_CLIENTMSG_CB,
            (CS_VOID*) guac_mssql_client_message);
    ct_callback(context, NULL, CS_SET, CS_SERVERMSG_CB,
            (CS_VOID*) guac_mssql_server_message);

    /* Allocate the connection */
    CS_CONNECTION* connection = NULL;
    if (ct_con_alloc(context, &connection) != CS_SUCCEED) {
        guac_dbshell_println(session, "ERROR: Unable to allocate the "
                "database connection.");
        ct_exit(context, CS_FORCE_EXIT);
        cs_ctx_drop(context);
        return 1;
    }

    /* Associate the session with the connection for message callbacks */
    guac_dbshell_session* userdata = session;
    ct_con_props(connection, CS_SET, CS_USERDATA, &userdata,
            sizeof(userdata), NULL);

    /* Apply credentials */
    ct_con_props(connection, CS_SET, CS_USERNAME,
            settings->username, CS_NULLTERM, NULL);
    ct_con_props(connection, CS_SET, CS_PASSWORD,
            settings->password != NULL ? settings->password : "",
            CS_NULLTERM, NULL);
    ct_con_props(connection, CS_SET, CS_APPNAME,
            (CS_VOID*) "guacd", CS_NULLTERM, NULL);

    /* Request a specific TDS protocol version if configured */
    if (extra->tds_version != NULL) {

        CS_INT version = 0;

        if (strcmp(extra->tds_version, "7.1") == 0)
            version = CS_TDS_71;
        else if (strcmp(extra->tds_version, "7.2") == 0)
            version = CS_TDS_72;
        else if (strcmp(extra->tds_version, "7.3") == 0)
            version = CS_TDS_73;
        else if (strcmp(extra->tds_version, "7.4") == 0)
            version = CS_TDS_74;

        if (version != 0)
            ct_con_props(connection, CS_SET, CS_TDS_VERSION, &version,
                    CS_UNUSED, NULL);

    }

    /* Use UTF-8 for all character data */
    CS_LOCALE* locale = NULL;
    if (cs_loc_alloc(context, &locale) == CS_SUCCEED) {

        if (cs_locale(context, CS_SET, locale, CS_SYB_CHARSET,
                    (CS_CHAR*) "UTF-8", CS_NULLTERM, NULL) == CS_SUCCEED)
            ct_con_props(connection, CS_SET, CS_LOC_PROP, locale,
                    CS_UNUSED, NULL);

        cs_loc_drop(context, locale);

    }

    /* Connect directly to the given host and port, bypassing any
     * freetds.conf server definitions */
    char server_addr[512];
    snprintf(server_addr, sizeof(server_addr), "%s %i",
            settings->hostname, settings->port);
    ct_con_props(connection, CS_SET, CS_SERVERADDR, server_addr,
            CS_NULLTERM, NULL);

    if (ct_connect(connection, NULL, 0) != CS_SUCCEED) {

        /* Details will have been rendered by the message callbacks */
        guac_dbshell_println(session, "ERROR: Unable to connect to the "
                "SQL Server database.");

        ct_con_drop(connection);
        ct_exit(context, CS_FORCE_EXIT);
        cs_ctx_drop(context);
        return 1;

    }

    guac_mssql_data* data = guac_mem_zalloc(sizeof(guac_mssql_data));
    data->context = context;
    data->connection = connection;
    session->driver_data = data;

    guac_dbshell_println(session, "Connected to %s. Statements end with "
            "';' or a line containing only \"GO\". Type \\h for help.",
            settings->hostname);
    guac_dbshell_println(session, "");

    /* Switch to the requested database, if any */
    if (settings->database != NULL) {

        /* Quote the database name, doubling any closing brackets */
        guac_dbshell_buffer use_statement;
        guac_dbshell_buffer_init(&use_statement);
        guac_dbshell_buffer_append_string(&use_statement, "USE [");

        for (char* c = settings->database; *c != '\0'; c++) {
            guac_dbshell_buffer_append(&use_statement, c, 1);
            if (*c == ']')
                guac_dbshell_buffer_append(&use_statement, "]", 1);
        }

        guac_dbshell_buffer_append(&use_statement, "]", 1);
        guac_dbshell_buffer_append(&use_statement, "", 1);

        session->driver->execute_handler(session, use_statement.data);
        guac_dbshell_buffer_destroy(&use_statement);

    }

    return 0;

}

/**
 * Closes the connection to the SQL Server database. This handler
 * implements guac_dbshell_driver.disconnect_handler.
 *
 * @param session
 *     The session whose connection should be closed.
 */
static void guac_mssql_disconnect(guac_dbshell_session* session) {

    guac_mssql_data* data = (guac_mssql_data*) session->driver_data;
    if (data == NULL)
        return;

    ct_close(data->connection, CS_FORCE_CLOSE);
    ct_con_drop(data->connection);
    ct_exit(data->context, CS_FORCE_EXIT);
    cs_ctx_drop(data->context);

    guac_mem_free(data);
    session->driver_data = NULL;

}

/**
 * Executes the given statement against the SQL Server database, rendering
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
static int guac_mssql_execute(guac_dbshell_session* session,
        const char* statement) {

    guac_mssql_data* data = (guac_mssql_data*) session->driver_data;

    guac_timestamp started = guac_timestamp_current();

    CS_COMMAND* command = NULL;
    if (ct_cmd_alloc(data->connection, &command) != CS_SUCCEED)
        return GUAC_DBSHELL_EXECUTE_LOST;

    /* Dispatch the statement */
    if (ct_command(command, CS_LANG_CMD, (CS_CHAR*) statement, CS_NULLTERM,
                CS_UNUSED) != CS_SUCCEED
            || ct_send(command) != CS_SUCCEED) {
        ct_cmd_drop(command);
        return GUAC_DBSHELL_EXECUTE_LOST;
    }

    /* Process every result of the statement */
    CS_RETCODE code;
    CS_INT result_type;
    bool failed = false;

    while ((code = ct_results(command, &result_type)) == CS_SUCCEED) {

        switch (result_type) {

            /* Render returned rows as a table */
            case CS_ROW_RESULT: {

                long rows = guac_mssql_render_rows(session, command);
                if (rows < 0) {
                    failed = true;
                    break;
                }

                guac_dbshell_print_summary(session, (unsigned long) rows,
                        0, guac_timestamp_current() - started);

                break;
            }

            /* Report affected rows for statements returning no rows */
            case CS_CMD_SUCCEED:
            case CS_CMD_DONE: {

                CS_INT affected = 0;
                if (ct_res_info(command, CS_ROW_COUNT, &affected,
                            CS_UNUSED, NULL) == CS_SUCCEED
                        && affected >= 0
                        && result_type == CS_CMD_DONE)
                    guac_dbshell_print_summary(session,
                            (unsigned long) affected, 1,
                            guac_timestamp_current() - started);

                break;
            }

            /* Statement errors will have been rendered by the server
             * message callback */
            case CS_CMD_FAIL:
                break;

            /* Discard any other result types (computed results, etc.) */
            default:
                ct_cancel(NULL, command, CS_CANCEL_CURRENT);
                break;

        }

        if (failed)
            break;

    }

    ct_cmd_drop(command);
    guac_dbshell_println(session, "");

    /* Results ending for any reason other than completion (or a fetch
     * failure) means the connection is no longer usable */
    if (failed || code == CS_FAIL) {

        /* Verify the connection is still alive */
        CS_INT status = 0;
        if (ct_con_props(data->connection, CS_GET, CS_CON_STATUS, &status,
                    CS_UNUSED, NULL) != CS_SUCCEED
                || (status & CS_CONSTAT_DEAD))
            return GUAC_DBSHELL_EXECUTE_LOST;

    }

    return GUAC_DBSHELL_EXECUTE_OK;

}

/**
 * Interrupts the statement currently executing on the given session by
 * sending a TDS attention, as sqsh and sqlcmd do on Ctrl+C. This handler
 * implements guac_dbshell_driver.cancel_handler.
 *
 * @param session
 *     The session whose current statement should be interrupted.
 */
static void guac_mssql_cancel(guac_dbshell_session* session) {

    guac_mssql_data* data = (guac_mssql_data*) session->driver_data;
    if (data == NULL)
        return;

    ct_cancel(data->connection, NULL, CS_CANCEL_ATTN);

}

const guac_dbshell_driver guac_mssql_driver = {

    .name = "mssql",
    .dialect = GUAC_DBSHELL_DIALECT_TSQL,

    .connect_handler = guac_mssql_connect,
    .disconnect_handler = guac_mssql_disconnect,
    .execute_handler = guac_mssql_execute,
    .cancel_handler = guac_mssql_cancel

};
