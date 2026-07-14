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

#include "oracle.h"

#include <dbshell/client.h>
#include <dbshell/dbshell.h>
#include <dbshell/settings.h>
#include <dbshell/table.h>
#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <guacamole/timestamp.h>

#include <oci.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/**
 * The OCI character set ID of AL32UTF8, the Oracle character set
 * corresponding to UTF-8.
 */
#define GUAC_ORACLE_CHARSET_AL32UTF8 873

/**
 * The maximum number of bytes of a single column value fetched from the
 * server, including the null terminator. Longer values are truncated by
 * the client library.
 */
#define GUAC_ORACLE_MAX_VALUE_LENGTH 4000

/**
 * The maximum length of an EZConnect connection string, in bytes.
 */
#define GUAC_ORACLE_MAX_CONNECT_LENGTH 1024

/**
 * The maximum length of a column name, in bytes, including the null
 * terminator.
 */
#define GUAC_ORACLE_MAX_COLUMN_NAME 256

/**
 * Retrieves and renders the details of the most recent OCI error to the
 * session's terminal, returning the Oracle error code.
 *
 * @param session
 *     The session to render to.
 *
 * @param error
 *     The OCI error handle the failed call was invoked with.
 *
 * @return
 *     The Oracle error code (the numeric portion of "ORA-nnnnn"), or zero
 *     if no details are available.
 */
static sb4 guac_oracle_print_error(guac_dbshell_session* session,
        OCIError* error) {

    text message[512] = { 0 };
    sb4 code = 0;

    if (OCIErrorGet(error, 1, NULL, &code, message, sizeof(message),
                OCI_HTYPE_ERROR) == OCI_SUCCESS) {

        /* Trim the trailing newline of the message, if any */
        int length = strlen((char*) message);
        while (length > 0 && (message[length - 1] == '\n'
                    || message[length - 1] == '\r'))
            message[--length] = '\0';

        guac_dbshell_println(session, "%s", message);

    }
    else
        guac_dbshell_println(session, "ERROR: (no details available)");

    return code;

}

/**
 * Returns whether the given Oracle error code denotes loss of the server
 * connection.
 *
 * @param code
 *     The Oracle error code to test.
 *
 * @return
 *     Non-zero if the given error code denotes loss of the server
 *     connection, zero otherwise.
 */
static int guac_oracle_is_connection_error(sb4 code) {

    /* ORA-03113 (end-of-file on channel), ORA-03114 (not connected),
     * ORA-03135 (connection lost contact), ORA-12170 (connect timeout),
     * and ORA-12541 (no listener) all denote a dead connection */
    return code == 3113 || code == 3114 || code == 3135
        || code == 12170 || code == 12541;

}

/**
 * Returns whether values of the given Oracle internal datatype are numeric
 * and should be right-aligned when rendered.
 *
 * @param type
 *     The internal datatype code, as read from OCI_ATTR_DATA_TYPE.
 *
 * @return
 *     Non-zero if the datatype is numeric, zero otherwise.
 */
static int guac_oracle_is_numeric(ub2 type) {

    /* NUMBER, BINARY_FLOAT, and BINARY_DOUBLE respectively */
    return type == SQLT_NUM || type == 100 || type == 101;

}

/**
 * Renders the result set of the given executed SELECT statement to the
 * session's terminal as a table.
 *
 * @param session
 *     The session to render to.
 *
 * @param data
 *     The Oracle connection data of the session.
 *
 * @param statement
 *     The executed statement handle.
 *
 * @return
 *     The number of rows rendered, or -1 if fetching failed (the error
 *     having been rendered).
 */
static long guac_oracle_render_rows(guac_dbshell_session* session,
        guac_oracle_data* data, OCIStmt* statement) {

    /* Determine the number of columns */
    ub4 num_columns = 0;
    if (OCIAttrGet(statement, OCI_HTYPE_STMT, &num_columns, NULL,
                OCI_ATTR_PARAM_COUNT, data->error) != OCI_SUCCESS
            || num_columns == 0)
        return -1;

    guac_dbshell_table* table = guac_dbshell_table_begin(session->term,
            num_columns);

    /* Per-column fetch buffers and NULL indicators */
    char** values = guac_mem_zalloc(guac_mem_ckd_mul_or_die(num_columns,
                sizeof(char*)));
    sb2* indicators = guac_mem_zalloc(guac_mem_ckd_mul_or_die(num_columns,
                sizeof(sb2)));

    /* Describe and define each column as null-terminated text */
    for (ub4 i = 0; i < num_columns; i++) {

        char name[GUAC_ORACLE_MAX_COLUMN_NAME] = { 0 };
        ub2 type = 0;

        OCIParam* column = NULL;
        if (OCIParamGet(statement, OCI_HTYPE_STMT, data->error,
                    (void**) &column, i + 1) == OCI_SUCCESS) {

            text* column_name = NULL;
            ub4 name_length = 0;

            if (OCIAttrGet(column, OCI_DTYPE_PARAM, &column_name,
                        &name_length, OCI_ATTR_NAME, data->error)
                    == OCI_SUCCESS && column_name != NULL) {

                if (name_length > sizeof(name) - 1)
                    name_length = sizeof(name) - 1;

                memcpy(name, column_name, name_length);

            }

            OCIAttrGet(column, OCI_DTYPE_PARAM, &type, NULL,
                    OCI_ATTR_DATA_TYPE, data->error);

            OCIDescriptorFree(column, OCI_DTYPE_PARAM);

        }

        guac_dbshell_table_set_header(table, i, name,
                guac_oracle_is_numeric(type));

        values[i] = guac_mem_alloc(GUAC_ORACLE_MAX_VALUE_LENGTH);

        OCIDefine* define = NULL;
        OCIDefineByPos(statement, &define, data->error, i + 1, values[i],
                GUAC_ORACLE_MAX_VALUE_LENGTH, SQLT_STR, &indicators[i],
                NULL, NULL, OCI_DEFAULT);

    }

    /* Fetch and render all rows */
    long rows = 0;
    bool failed = false;

    const char** row = guac_mem_zalloc(guac_mem_ckd_mul_or_die(num_columns,
                sizeof(char*)));

    for (;;) {

        sword status = OCIStmtFetch2(statement, data->error, 1,
                OCI_FETCH_NEXT, 0, OCI_DEFAULT);

        if (status == OCI_NO_DATA)
            break;

        if (status != OCI_SUCCESS && status != OCI_SUCCESS_WITH_INFO) {
            failed = true;
            break;
        }

        for (ub4 i = 0; i < num_columns; i++)
            row[i] = (indicators[i] == -1) ? NULL : values[i];

        guac_dbshell_table_add_row(table, row);
        rows++;

    }

    guac_dbshell_table_end(table);

    for (ub4 i = 0; i < num_columns; i++)
        guac_mem_free(values[i]);
    guac_mem_free(values);
    guac_mem_free(indicators);
    guac_mem_free(row);

    if (failed) {
        guac_oracle_print_error(session, data->error);
        return -1;
    }

    return rows;

}

/**
 * Establishes the connection to the Oracle Database server described by
 * the settings of the given session. This handler implements
 * guac_dbshell_driver.connect_handler.
 *
 * @param session
 *     The session on whose behalf the connection is being established.
 *
 * @return
 *     Zero on success, non-zero on failure.
 */
static int guac_oracle_connect(guac_dbshell_session* session) {

    guac_dbshell_settings* settings =
        (guac_dbshell_settings*) session->settings;

    guac_dbshell_client* dbshell_client =
        (guac_dbshell_client*) session->client->data;
    guac_oracle_extra_settings* extra =
        (guac_oracle_extra_settings*) dbshell_client->extra_settings;

    /* The service name is required to address the database */
    if (extra->service_name == NULL) {
        guac_dbshell_println(session, "ERROR: The \"service-name\" "
                "parameter is required for Oracle connections.");
        return 1;
    }

    /* Oracle authentication always involves a username and password;
     * prompt for whichever is missing */
    guac_dbshell_prompt_credentials(session, true, true);

    /* Create a threaded, UTF-8 environment */
    OCIEnv* environment = NULL;
    if (OCIEnvNlsCreate(&environment, OCI_THREADED, NULL, NULL, NULL,
                NULL, 0, NULL, GUAC_ORACLE_CHARSET_AL32UTF8,
                GUAC_ORACLE_CHARSET_AL32UTF8) != OCI_SUCCESS) {
        guac_dbshell_println(session, "ERROR: Unable to initialize the "
                "Oracle client environment.");
        return 1;
    }

    /* Allocate error handles (one per thread which may use OCI) */
    OCIError* error = NULL;
    OCIError* cancel_error = NULL;
    OCIHandleAlloc(environment, (void**) &error, OCI_HTYPE_ERROR, 0,
            NULL);
    OCIHandleAlloc(environment, (void**) &cancel_error, OCI_HTYPE_ERROR,
            0, NULL);

    if (error == NULL || cancel_error == NULL) {

        guac_dbshell_println(session, "ERROR: Unable to allocate Oracle "
                "error handles.");

        if (error != NULL)
            OCIHandleFree(error, OCI_HTYPE_ERROR);
        if (cancel_error != NULL)
            OCIHandleFree(cancel_error, OCI_HTYPE_ERROR);

        OCIHandleFree(environment, OCI_HTYPE_ENV);
        return 1;

    }

    /* Address the database using EZConnect syntax */
    char connect_string[GUAC_ORACLE_MAX_CONNECT_LENGTH];
    snprintf(connect_string, sizeof(connect_string), "//%s:%i/%s",
            settings->hostname, settings->port, extra->service_name);

    /* Connect */
    OCISvcCtx* service = NULL;
    if (OCILogon2(environment, error, &service,
                (const OraText*) settings->username,
                strlen(settings->username),
                (const OraText*) (settings->password != NULL
                    ? settings->password : ""),
                settings->password != NULL
                    ? strlen(settings->password) : 0,
                (const OraText*) connect_string, strlen(connect_string),
                OCI_LOGON2_STMTCACHE) != OCI_SUCCESS) {

        guac_oracle_print_error(session, error);

        OCIHandleFree(error, OCI_HTYPE_ERROR);
        OCIHandleFree(cancel_error, OCI_HTYPE_ERROR);
        OCIHandleFree(environment, OCI_HTYPE_ENV);
        return 1;

    }

    guac_oracle_data* data = guac_mem_zalloc(sizeof(guac_oracle_data));
    data->environment = environment;
    data->error = error;
    data->cancel_error = cancel_error;
    data->service = service;
    session->driver_data = data;

    guac_dbshell_println(session, "Connected to %s. SQL statements end "
            "with ';'; PL/SQL blocks end with a line containing only "
            "\"/\". Type \\h for help.", connect_string);
    guac_dbshell_println(session, "");

    return 0;

}

/**
 * Closes the connection to the Oracle Database server. This handler
 * implements guac_dbshell_driver.disconnect_handler.
 *
 * @param session
 *     The session whose connection should be closed.
 */
static void guac_oracle_disconnect(guac_dbshell_session* session) {

    guac_oracle_data* data = (guac_oracle_data*) session->driver_data;
    if (data == NULL)
        return;

    OCILogoff(data->service, data->error);
    OCIHandleFree(data->error, OCI_HTYPE_ERROR);
    OCIHandleFree(data->cancel_error, OCI_HTYPE_ERROR);
    OCIHandleFree(data->environment, OCI_HTYPE_ENV);

    guac_mem_free(data);
    session->driver_data = NULL;

}

/**
 * Executes the given statement against the Oracle Database server,
 * rendering all results and errors to the session's terminal. This
 * handler implements guac_dbshell_driver.execute_handler.
 *
 * @param session
 *     The session on whose behalf the statement is being executed.
 *
 * @param statement_text
 *     The statement to execute.
 *
 * @return
 *     GUAC_DBSHELL_EXECUTE_OK if the session may continue, or
 *     GUAC_DBSHELL_EXECUTE_LOST if the connection has been lost.
 */
static int guac_oracle_execute(guac_dbshell_session* session,
        const char* statement_text) {

    guac_oracle_data* data = (guac_oracle_data*) session->driver_data;

    guac_timestamp started = guac_timestamp_current();

    /* Prepare the statement */
    OCIStmt* statement = NULL;
    if (OCIStmtPrepare2(data->service, &statement, data->error,
                (const OraText*) statement_text, strlen(statement_text),
                NULL, 0, OCI_NTV_SYNTAX, OCI_DEFAULT) != OCI_SUCCESS) {
        sb4 code = guac_oracle_print_error(session, data->error);
        return guac_oracle_is_connection_error(code)
            ? GUAC_DBSHELL_EXECUTE_LOST : GUAC_DBSHELL_EXECUTE_OK;
    }

    /* Determine whether the statement is a query */
    ub2 statement_type = 0;
    OCIAttrGet(statement, OCI_HTYPE_STMT, &statement_type, NULL,
            OCI_ATTR_STMT_TYPE, data->error);

    int result = GUAC_DBSHELL_EXECUTE_OK;

    /* Queries execute with no prefetched iterations, then fetch */
    if (statement_type == OCI_STMT_SELECT) {

        sword status = OCIStmtExecute(data->service, statement,
                data->error, 0, 0, NULL, NULL, OCI_DEFAULT);

        if (status != OCI_SUCCESS && status != OCI_SUCCESS_WITH_INFO) {
            sb4 code = guac_oracle_print_error(session, data->error);
            OCIReset(data->service, data->error);
            if (guac_oracle_is_connection_error(code))
                result = GUAC_DBSHELL_EXECUTE_LOST;
        }

        else {

            long rows = guac_oracle_render_rows(session, data, statement);
            if (rows >= 0)
                guac_dbshell_print_summary(session, (unsigned long) rows,
                        0, guac_timestamp_current() - started);

        }

    }

    /* All other statements execute exactly once and commit on success */
    else {

        sword status = OCIStmtExecute(data->service, statement,
                data->error, 1, 0, NULL, NULL, OCI_COMMIT_ON_SUCCESS);

        if (status != OCI_SUCCESS && status != OCI_SUCCESS_WITH_INFO) {
            sb4 code = guac_oracle_print_error(session, data->error);
            OCIReset(data->service, data->error);
            if (guac_oracle_is_connection_error(code))
                result = GUAC_DBSHELL_EXECUTE_LOST;
        }

        else {

            ub4 affected = 0;
            OCIAttrGet(statement, OCI_HTYPE_STMT, &affected, NULL,
                    OCI_ATTR_ROW_COUNT, data->error);

            guac_dbshell_print_summary(session, affected, 1,
                    guac_timestamp_current() - started);

        }

    }

    OCIStmtRelease(statement, data->error, NULL, 0, OCI_DEFAULT);

    guac_dbshell_println(session, "");
    return result;

}

/**
 * Interrupts the statement currently executing on the given session using
 * OCIBreak(), which is safe to invoke from a thread other than the one
 * executing the statement. This handler implements
 * guac_dbshell_driver.cancel_handler.
 *
 * @param session
 *     The session whose current statement should be interrupted.
 */
static void guac_oracle_cancel(guac_dbshell_session* session) {

    guac_oracle_data* data = (guac_oracle_data*) session->driver_data;
    if (data == NULL)
        return;

    OCIBreak(data->service, data->cancel_error);

}

const guac_dbshell_driver guac_oracle_driver = {

    .name = "oracle",
    .dialect = GUAC_DBSHELL_DIALECT_ORACLE,

    .connect_handler = guac_oracle_connect,
    .disconnect_handler = guac_oracle_disconnect,
    .execute_handler = guac_oracle_execute,
    .cancel_handler = guac_oracle_cancel

};
