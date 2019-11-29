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
#include "ssh.h"
#include "terminal/terminal.h"

#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/user.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

/**
 * All SSH connection settings which may be updated by unprivileged users
 * through "argv" streams.
 */
typedef enum guac_ssh_argv_setting {

    /**
     * The color scheme of the terminal.
     */
    GUAC_SSH_ARGV_SETTING_COLOR_SCHEME,

    /**
     * The name of the font family used by the terminal.
     */
    GUAC_SSH_ARGV_SETTING_FONT_NAME,

    /**
     * The size of the font used by the terminal, in points.
     */
    GUAC_SSH_ARGV_SETTING_FONT_SIZE

} guac_ssh_argv_setting;

/**
 * The value or current status of a connection parameter received over an
 * "argv" stream.
 */
typedef struct guac_ssh_argv {

    /**
     * The specific setting being updated.
     */
    guac_ssh_argv_setting setting;

    /**
     * Buffer space for containing the received argument value.
     */
    char buffer[GUAC_SSH_ARGV_MAX_LENGTH];

    /**
     * The number of bytes received so far.
     */
    int length;

} guac_ssh_argv;

/**
 * Handler for "blob" instructions which appends the data from received blobs
 * to the end of the in-progress argument value buffer.
 *
 * @see guac_user_blob_handler
 */
static int guac_ssh_argv_blob_handler(guac_user* user,
        guac_stream* stream, void* data, int length) {

    guac_ssh_argv* argv = (guac_ssh_argv*) stream->data;

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
static int guac_ssh_argv_end_handler(guac_user* user,
        guac_stream* stream) {

    int size;

    guac_client* client = user->client;
    guac_ssh_client* ssh_client = (guac_ssh_client*) client->data;
    guac_terminal* terminal = ssh_client->term;

    /* Append null terminator to value */
    guac_ssh_argv* argv = (guac_ssh_argv*) stream->data;
    argv->buffer[argv->length] = '\0';

    /* Apply changes to chosen setting */
    switch (argv->setting) {

        /* Update color scheme */
        case GUAC_SSH_ARGV_SETTING_COLOR_SCHEME:
            guac_terminal_apply_color_scheme(terminal, argv->buffer);
            guac_client_stream_argv(client, client->socket, "text/plain",
                    "color-scheme", argv->buffer);
            break;

        /* Update font name */
        case GUAC_SSH_ARGV_SETTING_FONT_NAME:
            guac_terminal_apply_font(terminal, argv->buffer, -1, 0);
            guac_client_stream_argv(client, client->socket, "text/plain",
                    "font-name", argv->buffer);
            break;

        /* Update font size */
        case GUAC_SSH_ARGV_SETTING_FONT_SIZE:

            /* Update only if font size is sane */
            size = atoi(argv->buffer);
            if (size > 0) {
                guac_terminal_apply_font(terminal, NULL, size,
                        ssh_client->settings->resolution);
                guac_client_stream_argv(client, client->socket, "text/plain",
                        "font-size", argv->buffer);
            }

            break;

    }

    /* Update SSH pty size if connected */
    if (ssh_client->term_channel != NULL) {
        pthread_mutex_lock(&(ssh_client->term_channel_lock));
        libssh2_channel_request_pty_size(ssh_client->term_channel,
                terminal->term_width, terminal->term_height);
        pthread_mutex_unlock(&(ssh_client->term_channel_lock));
    }

    free(argv);
    return 0;

}

int guac_ssh_argv_handler(guac_user* user, guac_stream* stream,
        char* mimetype, char* name) {

    guac_ssh_argv_setting setting;

    /* Allow users to update the color scheme and font details */
    if (strcmp(name, "color-scheme") == 0)
        setting = GUAC_SSH_ARGV_SETTING_COLOR_SCHEME;
    else if (strcmp(name, "font-name") == 0)
        setting = GUAC_SSH_ARGV_SETTING_FONT_NAME;
    else if (strcmp(name, "font-size") == 0)
        setting = GUAC_SSH_ARGV_SETTING_FONT_SIZE;

    /* No other connection parameters may be updated */
    else {
        guac_protocol_send_ack(user->socket, stream, "Not allowed.",
                GUAC_PROTOCOL_STATUS_CLIENT_FORBIDDEN);
        guac_socket_flush(user->socket);
        return 0;
    }

    guac_ssh_argv* argv = malloc(sizeof(guac_ssh_argv));
    argv->setting = setting;
    argv->length = 0;

    /* Prepare stream to receive argument value */
    stream->blob_handler = guac_ssh_argv_blob_handler;
    stream->end_handler = guac_ssh_argv_end_handler;
    stream->data = argv;

    /* Signal stream is ready */
    guac_protocol_send_ack(user->socket, stream, "Ready for updated "
            "parameter.", GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(user->socket);
    return 0;

}

void* guac_ssh_send_current_argv(guac_user* user, void* data) {

    guac_ssh_client* ssh_client = (guac_ssh_client*) data;
    guac_terminal* terminal = ssh_client->term;

    /* Send current color scheme */
    guac_user_stream_argv(user, user->socket, "text/plain", "color-scheme",
            terminal->color_scheme);

    /* Send current font name */
    guac_user_stream_argv(user, user->socket, "text/plain", "font-name",
            terminal->font_name);

    /* Send current font size */
    char font_size[64];
    sprintf(font_size, "%i", terminal->font_size);
    guac_user_stream_argv(user, user->socket, "text/plain", "font-size",
            font_size);

    return NULL;

}

