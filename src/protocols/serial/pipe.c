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
#include "pipe.h"
#include "serial.h"
#include "stream.h"
#include "terminal/terminal.h"

#include <guacamole/mem.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/user.h>

#include <ctype.h>
#include <string.h>

/**
 * The maximum length, in bytes, of a single control command token.
 */
#define GUAC_SERIAL_CONTROL_TOKEN_LENGTH 64

/**
 * Per-inbound-stream reassembly state for the control pipe. Control commands
 * are whitespace-delimited plain-text tokens which may span multiple blobs.
 */
typedef struct guac_serial_control_stream {

    /**
     * Buffer accumulating the current (possibly partial) command token.
     */
    char token[GUAC_SERIAL_CONTROL_TOKEN_LENGTH];

    /**
     * The number of bytes currently held in the token buffer.
     */
    int length;

} guac_serial_control_stream;

/**
 * Executes a single, complete control command token.
 *
 * @param user
 *     The user which sent the command.
 *
 * @param token
 *     The null-terminated command token.
 */
static void guac_serial_control_dispatch(guac_user* user, const char* token) {

    guac_client* client = user->client;
    guac_serial_client* serial_client = (guac_serial_client*) client->data;

    /* Send a serial break */
    if (strcmp(token, "break") == 0) {
        if (serial_client->stream != NULL)
            guac_serial_stream_send_break(serial_client->stream);
    }

    /* Ignore unrecognized commands */
    else
        guac_client_log(client, GUAC_LOG_DEBUG, "Ignoring unknown serial "
                "control command: \"%s\".", token);

}

/**
 * Blob handler for the inbound control pipe: reassembles whitespace-delimited
 * plain-text tokens and dispatches each complete token.
 */
static int guac_serial_control_blob_handler(guac_user* user,
        guac_stream* stream, void* data, int length) {

    guac_serial_control_stream* cs =
        (guac_serial_control_stream*) stream->data;
    const char* in = (const char*) data;

    for (int i = 0; i < length; i++) {

        char c = in[i];

        /* A whitespace character completes the current token */
        if (isspace((unsigned char) c)) {
            if (cs->length > 0) {
                cs->token[cs->length] = '\0';
                guac_serial_control_dispatch(user, cs->token);
                cs->length = 0;
            }
        }

        /* Accumulate non-whitespace characters into the current token */
        else if (cs->length < GUAC_SERIAL_CONTROL_TOKEN_LENGTH - 1)
            cs->token[cs->length++] = c;

        /* Oversized token; drop it to resynchronize on the next whitespace */
        else
            cs->length = 0;

    }

    guac_protocol_send_ack(user->socket, stream, "OK",
            GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(user->socket);
    return 0;

}

/**
 * End handler for the inbound control pipe. Dispatches any final unterminated
 * token and releases the reassembly state.
 */
static int guac_serial_control_end_handler(guac_user* user,
        guac_stream* stream) {

    guac_serial_control_stream* cs =
        (guac_serial_control_stream*) stream->data;

    /* Dispatch any trailing token not followed by whitespace */
    if (cs != NULL && cs->length > 0) {
        cs->token[cs->length] = '\0';
        guac_serial_control_dispatch(user, cs->token);
    }

    guac_mem_free(stream->data);
    stream->data = NULL;
    return 0;

}

/**
 * Sets up an inbound "serial-control" pipe stream just opened by the given
 * user, installing the blob/end handlers that parse and dispatch control
 * commands.
 *
 * @param user
 *     The user which opened the control pipe.
 *
 * @param stream
 *     The inbound pipe stream.
 */
static void guac_serial_control_open(guac_user* user, guac_stream* stream) {

    guac_serial_control_stream* cs =
        guac_mem_zalloc(sizeof(guac_serial_control_stream));

    stream->data = cs;
    stream->blob_handler = guac_serial_control_blob_handler;
    stream->end_handler = guac_serial_control_end_handler;

    guac_protocol_send_ack(user->socket, stream, "OK",
            GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(user->socket);

}

int guac_serial_pipe_handler(guac_user* user, guac_stream* stream,
        char* mimetype, char* name) {

    guac_client* client = user->client;
    guac_serial_client* serial_client = (guac_serial_client*) client->data;

    /* Redirect STDIN if pipe has required name */
    if (strcmp(name, GUAC_SERIAL_STDIN_PIPE_NAME) == 0) {
        guac_terminal_send_stream(serial_client->term, user, stream);
        return 0;
    }

    /* Out-of-band control channel */
    if (strcmp(name, GUAC_SERIAL_CONTROL_PIPE_NAME) == 0) {
        guac_serial_control_open(user, stream);
        return 0;
    }

    /* No other inbound pipe streams are supported */
    guac_protocol_send_ack(user->socket, stream, "No such input stream.",
            GUAC_PROTOCOL_STATUS_RESOURCE_NOT_FOUND);
    guac_socket_flush(user->socket);
    return 0;

}
