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

#include "log.h"
#include "user.h"

#include <guacamole/client.h>
#include <guacamole/error.h>
#include <guacamole/parser.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/user.h>

#include <pthread.h>
#include <stdlib.h>

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

    /* Done */
    return 0;

}

