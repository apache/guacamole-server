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
#include "common-ssh/ssh-constants.h"
#include "terminal/terminal.h"

#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/user.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

int guac_ssh_argv_callback(guac_user* user, const char* mimetype,
        const char* name, const char* value, void* data) {

    guac_client* client = user->client;
    guac_ssh_client* ssh_client = (guac_ssh_client*) client->data;
    guac_terminal* terminal = ssh_client->term;
    guac_ssh_settings* settings = ssh_client->settings;

    /* Update color scheme */
    if (strcmp(name, GUAC_SSH_ARGV_COLOR_SCHEME) == 0)
        guac_terminal_apply_color_scheme(terminal, value);

<<<<<<< HEAD
    /* Update font name */
    else if (strcmp(name, GUAC_SSH_ARGV_FONT_NAME) == 0)
        guac_terminal_apply_font(terminal, value, -1, 0);
=======
        /* Update username */
        case GUAC_SSH_ARGV_SETTING_USERNAME:
            free(settings->username);
            settings->username = strndup(argv->buffer, argv->length);
            
            pthread_cond_broadcast(&(ssh_client->ssh_credential_cond));
            break;
        
        /* Update password */
        case GUAC_SSH_ARGV_SETTING_PASSWORD:
            
            /* Update password in settings */
            free(settings->password);
            settings->password = strndup(argv->buffer, argv->length);
            
            /* Update password in ssh user */
            guac_common_ssh_user* ssh_user = (guac_common_ssh_user*) ssh_client->user;
            if (ssh_user != NULL)
                guac_common_ssh_user_set_password(ssh_user, argv->buffer);
            
            pthread_cond_broadcast(&(ssh_client->ssh_credential_cond));
            break;
        
        /* Update private key passphrase */
        case GUAC_SSH_ARGV_SETTING_PASSPHRASE:
            free(settings->key_passphrase);
            settings->key_passphrase = strndup(argv->buffer, argv->length);
            
            pthread_cond_broadcast(&(ssh_client->ssh_credential_cond));
            break;
        
        /* Update color scheme */
        case GUAC_SSH_ARGV_SETTING_COLOR_SCHEME:
            guac_terminal_apply_color_scheme(terminal, argv->buffer);
            guac_client_stream_argv(client, client->socket, "text/plain",
                    GUAC_SSH_PARAMETER_NAME_COLOR_SCHEME, argv->buffer);
            break;

        /* Update font name */
        case GUAC_SSH_ARGV_SETTING_FONT_NAME:
            guac_terminal_apply_font(terminal, argv->buffer, -1, 0);
            guac_client_stream_argv(client, client->socket, "text/plain",
                    GUAC_SSH_PARAMETER_NAME_FONT_NAME, argv->buffer);
            break;

        /* Update font size */
        case GUAC_SSH_ARGV_SETTING_FONT_SIZE:

            /* Update only if font size is sane */
            size = atoi(argv->buffer);
            if (size > 0) {
                guac_terminal_apply_font(terminal, NULL, size,
                        ssh_client->settings->resolution);
                guac_client_stream_argv(client, client->socket, "text/plain",
                        GUAC_SSH_PARAMETER_NAME_FONT_SIZE, argv->buffer);
            }

            break;
>>>>>>> GUACAMOLE-221: Use constants for parameters updated via argv or required instructions.

    /* Update only if font size is sane */
    else if (strcmp(name, GUAC_SSH_ARGV_FONT_SIZE) == 0) {
        int size = atoi(value);
        if (size > 0)
            guac_terminal_apply_font(terminal, NULL, size,
                    ssh_client->settings->resolution);
    }

    /* Update SSH pty size if connected */
    if (ssh_client->term_channel != NULL) {
        pthread_mutex_lock(&(ssh_client->term_channel_lock));
        libssh2_channel_request_pty_size(ssh_client->term_channel,
                terminal->term_width, terminal->term_height);
        pthread_mutex_unlock(&(ssh_client->term_channel_lock));
    }

    return 0;

}

void* guac_ssh_send_current_argv(guac_user* user, void* data) {

    guac_ssh_client* ssh_client = (guac_ssh_client*) data;
    guac_terminal* terminal = ssh_client->term;

    /* Send current color scheme */
    guac_user_stream_argv(user, user->socket, "text/plain",
            GUAC_SSH_ARGV_COLOR_SCHEME, terminal->color_scheme);

    /* Send current font name */
    guac_user_stream_argv(user, user->socket, "text/plain",
            GUAC_SSH_ARGV_FONT_NAME, terminal->font_name);

    /* Send current font size */
    char font_size[64];
    sprintf(font_size, "%i", terminal->font_size);
    guac_user_stream_argv(user, user->socket, "text/plain",
            GUAC_SSH_ARGV_FONT_SIZE, font_size);

    return NULL;

}

