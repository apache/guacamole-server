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

#include "client.h"
#include "error.h"
#include "parser.h"
#include "protocol.h"
#include "socket.h"
#include "user.h"

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

/**
 * Parameters required by the user input thread.
 */
typedef struct guac_user_input_thread_params {

    /**
     * The parser which will be used throughout the user's session.
     */
    guac_parser* parser;

    /**
     * A reference to the connected user.
     */
    guac_user* user;

    /**
     * The number of microseconds to wait for instructions from a connected
     * user before closing the connection with an error.
     */
    int usec_timeout;

} guac_user_input_thread_params;

/**
 * Prints an error message using the logging facilities of the given user,
 * automatically including any information present in guac_error.
 *
 * @param user
 *     The guac_user associated with the error that occurred.
 *
 * @param level
 *     The level at which to log this message.
 *
 * @param message
 *     The message to log.
 */
static void guac_user_log_guac_error(guac_user* user,
        guac_client_log_level level, const char* message) {

    if (guac_error != GUAC_STATUS_SUCCESS) {

        /* If error message provided, include in log */
        if (guac_error_message != NULL)
            guac_user_log(user, level, "%s: %s", message,
                    guac_error_message);

        /* Otherwise just log with standard status string */
        else
            guac_user_log(user, level, "%s: %s", message,
                    guac_status_string(guac_error));

    }

    /* Just log message if no status code */
    else
        guac_user_log(user, level, "%s", message);

}

/**
 * Logs a reasonable explanatory message regarding handshake failure based on
 * the current value of guac_error.
 *
 * @param user
 *     The guac_user associated with the failed Guacamole protocol handshake.
 */
static void guac_user_log_handshake_failure(guac_user* user) {

    if (guac_error == GUAC_STATUS_CLOSED)
        guac_user_log(user, GUAC_LOG_INFO,
                "Guacamole connection closed during handshake");
    else if (guac_error == GUAC_STATUS_PROTOCOL_ERROR)
        guac_user_log(user, GUAC_LOG_ERROR,
                "Guacamole protocol violation. Perhaps the version of "
                "guacamole-client is incompatible with this version of "
                "libguac?");
    else
        guac_user_log(user, GUAC_LOG_WARNING,
                "Guacamole handshake failed: %s",
                guac_status_string(guac_error));

}

/**
 * Copies the given array of mimetypes (strings) into a newly-allocated NULL-
 * terminated array of strings. Both the array and the strings within the array
 * are newly-allocated and must be later freed via guac_free_mimetypes().
 *
 * @param mimetypes
 *     The array of mimetypes to copy.
 *
 * @param count
 *     The number of mimetypes in the given array.
 *
 * @return
 *     A newly-allocated, NULL-terminated array containing newly-allocated
 *     copies of each of the mimetypes provided in the original mimetypes
 *     array.
 */
static char** guac_copy_mimetypes(char** mimetypes, int count) {

    int i;

    /* Allocate sufficient space for NULL-terminated array of mimetypes */
    char** mimetypes_copy = malloc(sizeof(char*) * (count+1));

    /* Copy each provided mimetype */
    for (i = 0; i < count; i++)
        mimetypes_copy[i] = strdup(mimetypes[i]);

    /* Terminate with NULL */
    mimetypes_copy[count] = NULL;

    return mimetypes_copy;

}

/**
 * Frees the given array of mimetypes, including the space allocated to each
 * mimetype string within the array. The provided array of mimetypes MUST have
 * been allocated with guac_copy_mimetypes().
 *
 * @param mimetypes
 *     The NULL-terminated array of mimetypes to free. This array MUST have
 *     been previously allocated with guac_copy_mimetypes().
 */
static void guac_free_mimetypes(char** mimetypes) {

    char** current_mimetype = mimetypes;

    /* Free all strings within NULL-terminated mimetype array */
    while (*current_mimetype != NULL) {
        free(*current_mimetype);
        current_mimetype++;
    }

    /* Free the array itself, now that its contents have been freed */
    free(mimetypes);

}

/**
 * The thread which handles all user input, calling event handlers for received
 * instructions.
 *
 * @param data
 *     A pointer to a guac_user_input_thread_params structure describing the
 *     user whose input is being handled and the guac_parser with which to
 *     handle it.
 *
 * @return
 *     Always NULL.
 */
static void* guac_user_input_thread(void* data) {

    guac_user_input_thread_params* params =
        (guac_user_input_thread_params*) data;

    int usec_timeout = params->usec_timeout;
    guac_user* user = params->user;
    guac_parser* parser = params->parser;
    guac_client* client = user->client;
    guac_socket* socket = user->socket;

    /* Guacamole user input loop */
    while (client->state == GUAC_CLIENT_RUNNING && user->active) {

        /* Read instruction, stop on error */
        if (guac_parser_read(parser, socket, usec_timeout)) {

            if (guac_error == GUAC_STATUS_TIMEOUT)
                guac_user_abort(user, GUAC_PROTOCOL_STATUS_CLIENT_TIMEOUT, "User is not responding.");

            else {
                if (guac_error != GUAC_STATUS_CLOSED)
                    guac_user_log_guac_error(user, GUAC_LOG_WARNING,
                            "Guacamole connection failure");
                guac_user_stop(user);
            }

            return NULL;
        }

        /* Reset guac_error and guac_error_message (user/client handlers are not
         * guaranteed to set these) */
        guac_error = GUAC_STATUS_SUCCESS;
        guac_error_message = NULL;

        /* Call handler, stop on error */
        if (guac_user_handle_instruction(user, parser->opcode, parser->argc, parser->argv) < 0) {

            /* Log error */
            guac_user_log_guac_error(user, GUAC_LOG_WARNING,
                    "User connection aborted");

            /* Log handler details */
            guac_user_log(user, GUAC_LOG_DEBUG, "Failing instruction handler in user was \"%s\"", parser->opcode);

            guac_user_stop(user);
            return NULL;
        }

    }

    return NULL;

}

/**
 * Starts the input/output threads of a new user. This function will block
 * until the user disconnects. If an error prevents the input/output threads
 * from starting, guac_user_stop() will be invoked on the given user.
 *
 * @param parser
 *     The guac_parser to use to handle all input from the given user.
 *
 * @param user
 *     The user whose associated I/O transfer threads should be started.
 *
 * @param usec_timeout
 *     The number of microseconds to wait for instructions from the given
 *     user before closing the connection with an error.
 *
 * @return
 *     Zero if the I/O threads started successfully and user has disconnected,
 *     or non-zero if the I/O threads could not be started.
 */
static int guac_user_start(guac_parser* parser, guac_user* user,
        int usec_timeout) {

    guac_user_input_thread_params params = {
        .parser = parser,
        .user = user,
        .usec_timeout = usec_timeout
    };

    pthread_t input_thread;

    if (pthread_create(&input_thread, NULL, guac_user_input_thread, (void*) &params)) {
        guac_user_log(user, GUAC_LOG_ERROR, "Unable to start input thread");
        guac_user_stop(user);
        return -1;
    }

    /* Wait for I/O threads */
    pthread_join(input_thread, NULL);

    /* Explicitly signal disconnect */
    guac_protocol_send_disconnect(user->socket);
    guac_socket_flush(user->socket);

    /* Done */
    return 0;

}

int guac_user_handle_connection(guac_user* user, int usec_timeout) {

    guac_socket* socket = user->socket;
    guac_client* client = user->client;

    /* Send args */
    if (guac_protocol_send_args(socket, client->args)
            || guac_socket_flush(socket)) {

        /* Log error */
        guac_user_log_handshake_failure(user);
        guac_user_log_guac_error(user, GUAC_LOG_DEBUG,
                "Error sending \"args\" to new user");

        return 1;
    }

    guac_parser* parser = guac_parser_alloc();

    /* Get optimal screen size */
    if (guac_parser_expect(parser, socket, usec_timeout, "size")) {

        /* Log error */
        guac_user_log_handshake_failure(user);
        guac_user_log_guac_error(user, GUAC_LOG_DEBUG,
                "Error reading \"size\"");

        guac_parser_free(parser);
        return 1;
    }

    /* Validate content of size instruction */
    if (parser->argc < 2) {
        guac_user_log(user, GUAC_LOG_ERROR, "Received \"size\" "
                "instruction lacked required arguments.");
        guac_parser_free(parser);
        return 1;
    }

    /* Parse optimal screen dimensions from size instruction */
    user->info.optimal_width  = atoi(parser->argv[0]);
    user->info.optimal_height = atoi(parser->argv[1]);

    /* If DPI given, set the user resolution */
    if (parser->argc >= 3)
        user->info.optimal_resolution = atoi(parser->argv[2]);

    /* Otherwise, use a safe default for rough backwards compatibility */
    else
        user->info.optimal_resolution = 96;

    /* Get supported audio formats */
    if (guac_parser_expect(parser, socket, usec_timeout, "audio")) {

        /* Log error */
        guac_user_log_handshake_failure(user);
        guac_user_log_guac_error(user, GUAC_LOG_DEBUG,
                "Error reading \"audio\"");

        guac_parser_free(parser);
        return 1;
    }

    /* Store audio mimetypes */
    char** audio_mimetypes = guac_copy_mimetypes(parser->argv, parser->argc);
    user->info.audio_mimetypes = (const char**) audio_mimetypes;

    /* Get supported video formats */
    if (guac_parser_expect(parser, socket, usec_timeout, "video")) {

        /* Log error */
        guac_user_log_handshake_failure(user);
        guac_user_log_guac_error(user, GUAC_LOG_DEBUG,
                "Error reading \"video\"");

        guac_parser_free(parser);
        return 1;
    }

    /* Store video mimetypes */
    char** video_mimetypes = guac_copy_mimetypes(parser->argv, parser->argc);
    user->info.video_mimetypes = (const char**) video_mimetypes;

    /* Get supported image formats */
    if (guac_parser_expect(parser, socket, usec_timeout, "image")) {

        /* Log error */
        guac_user_log_handshake_failure(user);
        guac_user_log_guac_error(user, GUAC_LOG_DEBUG,
                "Error reading \"image\"");

        guac_parser_free(parser);
        return 1;
    }

    /* Store image mimetypes */
    char** image_mimetypes = guac_copy_mimetypes(parser->argv, parser->argc);
    user->info.image_mimetypes = (const char**) image_mimetypes;

    /* Get args from connect instruction */
    if (guac_parser_expect(parser, socket, usec_timeout, "connect")) {

        /* Log error */
        guac_user_log_handshake_failure(user);
        guac_user_log_guac_error(user, GUAC_LOG_DEBUG,
                "Error reading \"connect\"");

        guac_parser_free(parser);
        return 1;
    }

    /* Acknowledge connection availability */
    guac_protocol_send_ready(socket, client->connection_id);
    guac_socket_flush(socket);

    /* Attempt join */
    if (guac_client_add_user(client, user, parser->argc, parser->argv))
        guac_client_log(client, GUAC_LOG_ERROR, "User \"%s\" could NOT "
                "join connection \"%s\"", user->user_id, client->connection_id);

    /* Begin user connection if join successful */
    else {

        guac_client_log(client, GUAC_LOG_INFO, "User \"%s\" joined connection "
                "\"%s\" (%i users now present)", user->user_id,
                client->connection_id, client->connected_users);

        /* Handle user I/O, wait for connection to terminate */
        guac_user_start(parser, user, usec_timeout);

        /* Remove/free user */
        guac_client_remove_user(client, user);
        guac_client_log(client, GUAC_LOG_INFO, "User \"%s\" disconnected (%i "
                "users remain)", user->user_id, client->connected_users);

    }

    /* Free mimetype lists */
    guac_free_mimetypes(audio_mimetypes);
    guac_free_mimetypes(video_mimetypes);
    guac_free_mimetypes(image_mimetypes);

    guac_parser_free(parser);

    /* Successful disconnect */
    return 0;

}

