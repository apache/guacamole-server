/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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

