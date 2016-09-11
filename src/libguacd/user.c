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

#include "libguacd/log.h"
#include "libguacd/user.h"

#include <guacamole/client.h>
#include <guacamole/error.h>
#include <guacamole/parser.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/user.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

/**
 * Copies the given array of mimetypes (strings) into a newly-allocated NULL-
 * terminated array of strings. Both the array and the strings within the array
 * are newly-allocated and must be later freed via guacd_free_mimetypes().
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
static char** guacd_copy_mimetypes(char** mimetypes, int count) {

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
 * been allocated with guacd_copy_mimetypes().
 *
 * @param mimetypes
 *     The NULL-terminated array of mimetypes to free. This array MUST have
 *     been previously allocated with guacd_copy_mimetypes().
 */
static void guacd_free_mimetypes(char** mimetypes) {

    char** current_mimetype = mimetypes;

    /* Free all strings within NULL-terminated mimetype array */
    while (*current_mimetype != NULL) {
        free(*current_mimetype);
        current_mimetype++;
    }

    /* Free the array itself, now that its contents have been freed */
    free(mimetypes);

}

void* guacd_user_input_thread(void* data) {

    guacd_user_input_thread_params* params = (guacd_user_input_thread_params*) data;
    guac_user* user = params->user;
    guac_parser* parser = params->parser;
    guac_client* client = user->client;
    guac_socket* socket = user->socket;

    /* Guacamole user input loop */
    while (client->state == GUAC_CLIENT_RUNNING && user->active) {

        /* Read instruction, stop on error */
        if (guac_parser_read(parser, socket, GUACD_USEC_TIMEOUT)) {

            if (guac_error == GUAC_STATUS_TIMEOUT)
                guac_user_abort(user, GUAC_PROTOCOL_STATUS_CLIENT_TIMEOUT, "User is not responding.");

            else {
                if (guac_error != GUAC_STATUS_CLOSED)
                    guacd_client_log_guac_error(client, GUAC_LOG_WARNING,
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
            guacd_client_log_guac_error(client, GUAC_LOG_WARNING,
                    "User connection aborted");

            /* Log handler details */
            guac_user_log(user, GUAC_LOG_DEBUG, "Failing instruction handler in user was \"%s\"", parser->opcode);

            guac_user_stop(user);
            return NULL;
        }

    }

    return NULL;

}

int guacd_user_start(guac_parser* parser, guac_user* user) {

    guacd_user_input_thread_params params = {
        .parser = parser,
        .user = user
    };

    pthread_t input_thread;

    if (pthread_create(&input_thread, NULL, guacd_user_input_thread, (void*) &params)) {
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

int guacd_handle_user(guac_user* user) {

    guac_socket* socket = user->socket;
    guac_client* client = user->client;

    /* Send args */
    if (guac_protocol_send_args(socket, client->args)
            || guac_socket_flush(socket)) {

        /* Log error */
        guacd_client_log_handshake_failure(client);
        guacd_client_log_guac_error(client, GUAC_LOG_DEBUG,
                "Error sending \"args\" to new user");

        return 1;
    }

    guac_parser* parser = guac_parser_alloc();

    /* Get optimal screen size */
    if (guac_parser_expect(parser, socket, GUACD_USEC_TIMEOUT, "size")) {

        /* Log error */
        guacd_client_log_handshake_failure(client);
        guacd_client_log_guac_error(client, GUAC_LOG_DEBUG,
                "Error reading \"size\"");

        guac_parser_free(parser);
        return 1;
    }

    /* Validate content of size instruction */
    if (parser->argc < 2) {
        guac_client_log(client, GUAC_LOG_ERROR, "Received \"size\" "
                "instruction lacked required arguments.");
        guac_parser_free(parser);
        return 1;
    }

    /* Parse optimal screen dimensions from size instruction */
    user->info.optimal_width  = atoi(parser->argv[0]);
    user->info.optimal_height = atoi(parser->argv[1]);

    /* If DPI given, set the client resolution */
    if (parser->argc >= 3)
        user->info.optimal_resolution = atoi(parser->argv[2]);

    /* Otherwise, use a safe default for rough backwards compatibility */
    else
        user->info.optimal_resolution = 96;

    /* Get supported audio formats */
    if (guac_parser_expect(parser, socket, GUACD_USEC_TIMEOUT, "audio")) {

        /* Log error */
        guacd_client_log_handshake_failure(client);
        guacd_client_log_guac_error(client, GUAC_LOG_DEBUG,
                "Error reading \"audio\"");

        guac_parser_free(parser);
        return 1;
    }

    /* Store audio mimetypes */
    char** audio_mimetypes = guacd_copy_mimetypes(parser->argv, parser->argc);
    user->info.audio_mimetypes = (const char**) audio_mimetypes;

    /* Get supported video formats */
    if (guac_parser_expect(parser, socket, GUACD_USEC_TIMEOUT, "video")) {

        /* Log error */
        guacd_client_log_handshake_failure(client);
        guacd_client_log_guac_error(client, GUAC_LOG_DEBUG,
                "Error reading \"video\"");

        guac_parser_free(parser);
        return 1;
    }

    /* Store video mimetypes */
    char** video_mimetypes = guacd_copy_mimetypes(parser->argv, parser->argc);
    user->info.video_mimetypes = (const char**) video_mimetypes;

    /* Get supported image formats */
    if (guac_parser_expect(parser, socket, GUACD_USEC_TIMEOUT, "image")) {

        /* Log error */
        guacd_client_log_handshake_failure(client);
        guacd_client_log_guac_error(client, GUAC_LOG_DEBUG,
                "Error reading \"image\"");

        guac_parser_free(parser);
        return 1;
    }

    /* Store image mimetypes */
    char** image_mimetypes = guacd_copy_mimetypes(parser->argv, parser->argc);
    user->info.image_mimetypes = (const char**) image_mimetypes;

    /* Get args from connect instruction */
    if (guac_parser_expect(parser, socket, GUACD_USEC_TIMEOUT, "connect")) {

        /* Log error */
        guacd_client_log_handshake_failure(client);
        guacd_client_log_guac_error(client, GUAC_LOG_DEBUG,
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
        guacd_user_start(parser, user);

        /* Remove/free user */
        guac_client_remove_user(client, user);
        guac_client_log(client, GUAC_LOG_INFO, "User \"%s\" disconnected (%i "
                "users remain)", user->user_id, client->connected_users);

    }

    /* Free mimetype lists */
    guacd_free_mimetypes(audio_mimetypes);
    guacd_free_mimetypes(video_mimetypes);
    guacd_free_mimetypes(image_mimetypes);

    guac_parser_free(parser);

    /* Successful disconnect */
    return 0;

}

