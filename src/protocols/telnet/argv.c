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
#include "argv.h"
#include "telnet.h"
#include "terminal/terminal.h"

#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/user.h>

#include <stdlib.h>
#include <string.h>

/**
 * The value or current status of a connection parameter received over an
 * "argv" stream.
 */
typedef struct guac_telnet_argv {

    /**
     * Buffer space for containing the received argument value.
     */
    char buffer[16384];

    /**
     * The number of bytes received so far.
     */
    int length;

} guac_telnet_argv;

/**
 * Handler for "blob" instructions which appends the data from received blobs
 * to the end of the in-progress argument value buffer.
 *
 * @see guac_user_blob_handler
 */
static int guac_telnet_argv_blob_handler(guac_user* user,
        guac_stream* stream, void* data, int length) {

    guac_telnet_argv* argv = (guac_telnet_argv*) stream->data;

    /* Calculate buffer size remaining, including space for null terminator,
     * adjusting received length accordingly */
    int remaining = sizeof(argv->buffer) - argv->length - 1;
    if (length > remaining)
        length = remaining;

    /* Append received data to end of buffer */
    memcpy(argv->buffer + argv->length, data, length);
    argv->length += length;

    return 0;

}

/**
 * Handler for "end" instructions which applies the changes specified by the
 * argument value buffer associated with the stream.
 *
 * @see guac_user_end_handler
 */
static int guac_telnet_argv_end_handler(guac_user* user,
        guac_stream* stream) {

    guac_client* client = user->client;
    guac_telnet_client* telnet_client = (guac_telnet_client*) client->data;
    guac_terminal* terminal = telnet_client->term;

    /* Append null terminator to value */
    guac_telnet_argv* argv = (guac_telnet_argv*) stream->data;
    argv->buffer[argv->length] = '\0';

    /* Update color scheme */
    guac_terminal_apply_color_scheme(terminal, argv->buffer);
    free(argv);
    return 0;

}

int guac_telnet_argv_handler(guac_user* user, guac_stream* stream,
        char* mimetype, char* name) {

    /* Allow users to update the color scheme */
    if (strcmp(name, "color-scheme") == 0) {

        guac_telnet_argv* argv = malloc(sizeof(guac_telnet_argv));
        argv->length = 0;

        /* Prepare stream to receive argument value */
        stream->blob_handler = guac_telnet_argv_blob_handler;
        stream->end_handler = guac_telnet_argv_end_handler;
        stream->data = argv;

        /* Signal stream is ready */
        guac_protocol_send_ack(user->socket, stream, "Ready for color "
                "scheme.", GUAC_PROTOCOL_STATUS_SUCCESS);
        guac_socket_flush(user->socket);
        return 0;

    }

    /* No other connection parameters may be updated */
    guac_protocol_send_ack(user->socket, stream, "Not allowed.",
            GUAC_PROTOCOL_STATUS_CLIENT_FORBIDDEN);
    guac_socket_flush(user->socket);
    return 0;

}

