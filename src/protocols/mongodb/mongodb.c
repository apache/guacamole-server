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

#include "mongodb.h"

#include <dbshell/buffer.h>
#include <dbshell/client.h>
#include <dbshell/dbshell.h>
#include <dbshell/settings.h>
#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <guacamole/string.h>
#include <guacamole/timestamp.h>
#include <terminal/terminal.h>

#include <bson/bson.h>
#include <mongoc/mongoc.h>

#include <ctype.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/**
 * The name of the database used when neither the connection parameters nor
 * the \use meta-command have selected one.
 */
#define GUAC_MONGODB_DEFAULT_DATABASE "test"

/**
 * The number of spaces per indentation level when rendering JSON replies.
 */
#define GUAC_MONGODB_JSON_INDENT 2

/**
 * Guard ensuring that mongoc_init() is invoked exactly once within the
 * guacd process.
 */
static pthread_once_t guac_mongodb_init_once = PTHREAD_ONCE_INIT;

/**
 * Initializes the MongoDB C Driver. Invoked exactly once via
 * pthread_once().
 */
static void guac_mongodb_init(void) {
    mongoc_init();
}

/**
 * Writes the given JSON text to the session's terminal, re-indented for
 * readability: newlines and indentation are inserted after structural
 * braces, brackets, and commas which lie outside string literals.
 *
 * @param session
 *     The session to render to.
 *
 * @param json
 *     The null-terminated JSON text to render, as produced by libbson
 *     (all control characters within string literals are escaped).
 */
static void guac_mongodb_print_json(guac_dbshell_session* session,
        const char* json) {

    guac_dbshell_buffer output;
    guac_dbshell_buffer_init(&output);

    int depth = 0;
    bool in_string = false;

    for (const char* c = json; *c != '\0'; c++) {

        if (in_string) {

            guac_dbshell_buffer_append(&output, c, 1);

            /* Skip escaped characters within strings */
            if (*c == '\\' && *(c + 1) != '\0') {
                c++;
                guac_dbshell_buffer_append(&output, c, 1);
            }
            else if (*c == '"')
                in_string = false;

            continue;

        }

        switch (*c) {

            case '"':
                in_string = true;
                guac_dbshell_buffer_append(&output, c, 1);
                break;

            /* Open a new indentation level, unless the value is empty */
            case '{':
            case '[':

                guac_dbshell_buffer_append(&output, c, 1);

                if (*(c + 1) == '}' || *(c + 1) == ']') {
                    c++;
                    guac_dbshell_buffer_append(&output, c, 1);
                    break;
                }

                depth++;
                guac_dbshell_buffer_append_string(&output, "\r\n");
                guac_dbshell_buffer_append_repeat(&output, ' ',
                        depth * GUAC_MONGODB_JSON_INDENT);
                break;

            /* Close the current indentation level */
            case '}':
            case ']':

                if (depth > 0)
                    depth--;

                guac_dbshell_buffer_append_string(&output, "\r\n");
                guac_dbshell_buffer_append_repeat(&output, ' ',
                        depth * GUAC_MONGODB_JSON_INDENT);
                guac_dbshell_buffer_append(&output, c, 1);
                break;

            /* Break after each member */
            case ',':

                guac_dbshell_buffer_append(&output, c, 1);
                guac_dbshell_buffer_append_string(&output, "\r\n");
                guac_dbshell_buffer_append_repeat(&output, ' ',
                        depth * GUAC_MONGODB_JSON_INDENT);

                /* Swallow the space libbson places after commas */
                if (*(c + 1) == ' ')
                    c++;

                break;

            default:
                guac_dbshell_buffer_append(&output, c, 1);
                break;

        }

    }

    guac_dbshell_buffer_append_string(&output, "\r\n");
    guac_terminal_write(session->term, output.data, output.length);
    guac_dbshell_buffer_destroy(&output);

}

/**
 * Renders the given BSON reply document to the session's terminal as
 * indented relaxed Extended JSON.
 *
 * @param session
 *     The session to render to.
 *
 * @param reply
 *     The reply document to render.
 */
static void guac_mongodb_print_reply(guac_dbshell_session* session,
        const bson_t* reply) {

    char* json = bson_as_relaxed_extended_json(reply, NULL);
    if (json == NULL)
        return;

    guac_mongodb_print_json(session, json);
    bson_free(json);

}

/**
 * Returns whether the given libmongoc error denotes loss of connectivity
 * with the MongoDB server rather than a failure of the command itself.
 *
 * @param error
 *     The error to test.
 *
 * @return
 *     Non-zero if the error denotes loss of connectivity, zero otherwise.
 */
static int guac_mongodb_is_connection_error(const bson_error_t* error) {
    return error->domain == MONGOC_ERROR_STREAM
        || error->domain == MONGOC_ERROR_SERVER_SELECTION;
}

/**
 * Continues and renders the cursor within the given command reply, if any,
 * by repeatedly issuing getMore commands until the cursor is exhausted.
 *
 * @param session
 *     The session to render to.
 *
 * @param data
 *     The MongoDB connection data of the session.
 *
 * @param reply
 *     The reply of the originating command, which may contain a cursor
 *     document.
 *
 * @return
 *     Zero on success, non-zero if connectivity with the server was lost
 *     while iterating the cursor.
 */
static int guac_mongodb_continue_cursor(guac_dbshell_session* session,
        guac_mongodb_data* data, const bson_t* reply) {

    bson_iter_t iter;
    bson_iter_t cursor;

    /* Nothing to do unless the reply contains a cursor document */
    if (!bson_iter_init_find(&iter, reply, "cursor")
            || !BSON_ITER_HOLDS_DOCUMENT(&iter))
        return 0;

    /* Extract the cursor ID */
    int64_t cursor_id = 0;
    if (bson_iter_recurse(&iter, &cursor))
        while (bson_iter_next(&cursor)) {
            if (strcmp(bson_iter_key(&cursor), "id") == 0
                    && BSON_ITER_HOLDS_INT64(&cursor))
                cursor_id = bson_iter_int64(&cursor);
        }

    /* Extract the collection name from the cursor namespace ("db.coll") */
    char collection[128] = { 0 };

    if (bson_iter_init_find(&iter, reply, "cursor")
            && bson_iter_recurse(&iter, &cursor)
            && bson_iter_find(&cursor, "ns")
            && BSON_ITER_HOLDS_UTF8(&cursor)) {

        const char* ns = bson_iter_utf8(&cursor, NULL);
        const char* separator = strchr(ns, '.');
        if (separator != NULL)
            guac_strlcpy(collection, separator + 1, sizeof(collection));

    }

    /* Repeatedly fetch further batches until the cursor is exhausted */
    while (cursor_id != 0 && collection[0] != '\0') {

        bson_t get_more;
        bson_init(&get_more);
        BSON_APPEND_INT64(&get_more, "getMore", cursor_id);
        BSON_APPEND_UTF8(&get_more, "collection", collection);

        bson_t batch;
        bson_error_t error;
        bool success = mongoc_client_command_simple(data->client,
                data->database, &get_more, NULL, &batch, &error);
        bson_destroy(&get_more);

        if (!success) {

            guac_dbshell_println(session, "ERROR: %s", error.message);
            bson_destroy(&batch);

            if (guac_mongodb_is_connection_error(&error))
                return 1;

            return 0;

        }

        guac_mongodb_print_reply(session, &batch);

        /* Read the ID of the continued cursor */
        cursor_id = 0;
        if (bson_iter_init_find(&iter, &batch, "cursor")
                && bson_iter_recurse(&iter, &cursor)
                && bson_iter_find(&cursor, "id")
                && BSON_ITER_HOLDS_INT64(&cursor))
            cursor_id = bson_iter_int64(&cursor);

        bson_destroy(&batch);

    }

    return 0;

}

/**
 * Establishes the connection to the MongoDB server described by the
 * settings of the given session. This handler implements
 * guac_dbshell_driver.connect_handler.
 *
 * @param session
 *     The session on whose behalf the connection is being established.
 *
 * @return
 *     Zero on success, non-zero on failure.
 */
static int guac_mongodb_connect(guac_dbshell_session* session) {

    guac_dbshell_settings* settings =
        (guac_dbshell_settings*) session->settings;

    guac_dbshell_client* dbshell_client =
        (guac_dbshell_client*) session->client->data;
    guac_mongodb_extra_settings* extra =
        (guac_mongodb_extra_settings*) dbshell_client->extra_settings;

    pthread_once(&guac_mongodb_init_once, guac_mongodb_init);

    /* Unlike the SQL protocols, a missing username legitimately denotes
     * an unauthenticated connection; prompt only for a missing password
     * when a username was given */
    if (settings->username != NULL)
        guac_dbshell_prompt_credentials(session, false, true);

    mongoc_uri_t* uri = mongoc_uri_new_for_host_port(settings->hostname,
            settings->port);
    if (uri == NULL) {
        guac_dbshell_println(session, "ERROR: Invalid MongoDB server "
                "address.");
        return 1;
    }

    /* Apply credentials, if given */
    if (settings->username != NULL) {

        mongoc_uri_set_username(uri, settings->username);

        if (settings->password != NULL)
            mongoc_uri_set_password(uri, settings->password);

        if (extra->auth_database != NULL)
            mongoc_uri_set_auth_source(uri, extra->auth_database);

    }

    /* Bound connection establishment and server selection */
    mongoc_uri_set_option_as_int32(uri, MONGOC_URI_CONNECTTIMEOUTMS,
            settings->timeout * 1000);
    mongoc_uri_set_option_as_int32(uri, MONGOC_URI_SERVERSELECTIONTIMEOUTMS,
            settings->timeout * 1000);

    /* Apply TLS settings */
    if (extra->use_ssl) {

        mongoc_uri_set_option_as_bool(uri, MONGOC_URI_TLS, true);

        if (extra->ssl_ca_file != NULL)
            mongoc_uri_set_option_as_utf8(uri, MONGOC_URI_TLSCAFILE,
                    extra->ssl_ca_file);

    }

    mongoc_client_t* client = mongoc_client_new_from_uri(uri);
    mongoc_uri_destroy(uri);

    if (client == NULL) {
        guac_dbshell_println(session, "ERROR: Unable to create the "
                "MongoDB client.");
        return 1;
    }

    mongoc_client_set_appname(client, "guacd");

    /* Verify connectivity and authentication with a ping */
    bson_t ping;
    bson_init(&ping);
    BSON_APPEND_INT32(&ping, "ping", 1);

    bson_error_t error;
    bool success = mongoc_client_command_simple(client, "admin", &ping,
            NULL, NULL, &error);
    bson_destroy(&ping);

    if (!success) {
        guac_dbshell_println(session, "ERROR: %s", error.message);
        mongoc_client_destroy(client);
        return 1;
    }

    guac_mongodb_data* data = guac_mem_zalloc(sizeof(guac_mongodb_data));
    data->client = client;
    data->database = guac_strdup(settings->database != NULL
            ? settings->database : GUAC_MONGODB_DEFAULT_DATABASE);
    session->driver_data = data;

    guac_dbshell_println(session, "Connected to %s. Each statement is a "
            "MongoDB command document in Extended JSON, for example "
            "{\"find\": \"myCollection\"}. Type \\h for help.",
            settings->hostname);
    guac_dbshell_println(session, "");

    return 0;

}

/**
 * Closes the connection to the MongoDB server. This handler implements
 * guac_dbshell_driver.disconnect_handler.
 *
 * @param session
 *     The session whose connection should be closed.
 */
static void guac_mongodb_disconnect(guac_dbshell_session* session) {

    guac_mongodb_data* data = (guac_mongodb_data*) session->driver_data;
    if (data == NULL)
        return;

    mongoc_client_destroy(data->client);
    guac_mem_free(data->database);
    guac_mem_free(data);
    session->driver_data = NULL;

}

/**
 * Executes the given Extended JSON command document against the current
 * database, rendering the reply to the session's terminal. This handler
 * implements guac_dbshell_driver.execute_handler.
 *
 * @param session
 *     The session on whose behalf the command is being executed.
 *
 * @param statement
 *     The command document, as Extended JSON.
 *
 * @return
 *     GUAC_DBSHELL_EXECUTE_OK if the session may continue, or
 *     GUAC_DBSHELL_EXECUTE_LOST if the connection has been lost.
 */
static int guac_mongodb_execute(guac_dbshell_session* session,
        const char* statement) {

    guac_mongodb_data* data = (guac_mongodb_data*) session->driver_data;

    /* Parse the command document */
    bson_error_t error;
    bson_t* command = bson_new_from_json((const uint8_t*) statement, -1,
            &error);

    if (command == NULL) {
        guac_dbshell_println(session, "ERROR: %s", error.message);
        return GUAC_DBSHELL_EXECUTE_OK;
    }

    guac_timestamp started = guac_timestamp_current();

    /* Run the command against the current database */
    bson_t reply;
    bool success = mongoc_client_command_simple(data->client,
            data->database, command, NULL, &reply, &error);
    bson_destroy(command);

    if (!success) {

        guac_dbshell_println(session, "ERROR: %s", error.message);

        /* The reply document may carry further details */
        if (!bson_empty(&reply))
            guac_mongodb_print_reply(session, &reply);

        bson_destroy(&reply);

        if (guac_mongodb_is_connection_error(&error))
            return GUAC_DBSHELL_EXECUTE_LOST;

        return GUAC_DBSHELL_EXECUTE_OK;

    }

    guac_mongodb_print_reply(session, &reply);

    /* Follow any cursor within the reply to completion */
    int lost = guac_mongodb_continue_cursor(session, data, &reply);
    bson_destroy(&reply);

    if (lost)
        return GUAC_DBSHELL_EXECUTE_LOST;

    guac_dbshell_println(session, "Completed in %li ms.",
            (long) (guac_timestamp_current() - started));
    guac_dbshell_println(session, "");

    return GUAC_DBSHELL_EXECUTE_OK;

}

/**
 * Handles the MongoDB-specific meta-commands: "\use <db>" switches the
 * database commands are directed to, "\show dbs" lists databases, and
 * "\show collections" lists the collections of the current database. This
 * handler implements guac_dbshell_driver.meta_handler.
 *
 * @param session
 *     The session on whose behalf the meta-command is being handled.
 *
 * @param command
 *     The meta-command, without its leading backslash.
 *
 * @return
 *     Non-zero if the meta-command was recognized and handled, zero
 *     otherwise.
 */
static int guac_mongodb_meta(guac_dbshell_session* session,
        const char* command) {

    guac_mongodb_data* data = (guac_mongodb_data*) session->driver_data;

    /* "\use <db>" switches the current database */
    if (strncmp(command, "use", 3) == 0
            && isspace((unsigned char) command[3])) {

        const char* name = command + 4;
        while (isspace((unsigned char) *name))
            name++;

        /* Trim trailing whitespace */
        int length = strlen(name);
        while (length > 0 && isspace((unsigned char) name[length - 1]))
            length--;

        if (length == 0) {
            guac_dbshell_println(session, "Usage: \\use <database>");
            return 1;
        }

        guac_mem_free(data->database);
        data->database = guac_mem_alloc(length + 1);
        memcpy(data->database, name, length);
        data->database[length] = '\0';

        guac_dbshell_println(session, "Switched to database \"%s\".",
                data->database);
        return 1;

    }

    /* "\show dbs" and "\show collections" */
    if (strcmp(command, "show dbs") == 0
            || strcmp(command, "show databases") == 0) {
        guac_mongodb_execute(session,
                "{\"listDatabases\": 1, \"nameOnly\": true}");
        return 1;
    }

    if (strcmp(command, "show collections") == 0) {
        guac_mongodb_execute(session,
                "{\"listCollections\": 1, \"nameOnly\": true}");
        return 1;
    }

    return 0;

}

/**
 * Writes the MongoDB-specific help lines. This handler implements
 * guac_dbshell_driver.help_handler.
 *
 * @param session
 *     The session whose terminal should receive the help text.
 */
static void guac_mongodb_help(guac_dbshell_session* session) {

    guac_dbshell_println(session, "  \\use <db>          Switch to the "
            "given database.");
    guac_dbshell_println(session, "  \\show dbs          List "
            "databases.");
    guac_dbshell_println(session, "  \\show collections  List "
            "collections of the current database.");
    guac_dbshell_println(session, "Statements are MongoDB command "
            "documents in Extended JSON, executed against the current "
            "database.");

}

const guac_dbshell_driver guac_mongodb_driver = {

    .name = "mongodb",
    .dialect = GUAC_DBSHELL_DIALECT_JSON,

    .connect_handler = guac_mongodb_connect,
    .disconnect_handler = guac_mongodb_disconnect,
    .execute_handler = guac_mongodb_execute,
    .meta_handler = guac_mongodb_meta,
    .help_handler = guac_mongodb_help

};
