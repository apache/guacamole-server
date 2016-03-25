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

#include "clipboard.h"
#include "input.h"
#include "guac_display.h"
#include "user.h"
#include "sftp.h"
#include "ssh.h"
#include "settings.h"

#include <guacamole/client.h>
#include <guacamole/socket.h>
#include <guacamole/user.h>

#include <pthread.h>
#include <string.h>

int guac_ssh_user_join_handler(guac_user* user, int argc, char** argv) {

    guac_client* client = user->client;
    guac_ssh_client* ssh_client = (guac_ssh_client*) client->data;

    /* Connect via SSH if owner */
    if (user->owner) {

        /* Parse arguments into client */
        guac_ssh_settings* settings = ssh_client->settings =
            guac_ssh_parse_args(user, argc, (const char**) argv);

        /* Fail if settings cannot be parsed */
        if (settings == NULL) {
            guac_user_log(user, GUAC_LOG_INFO,
                    "Badly formatted client arguments.");
            return 1;
        }

        /* Start client thread */
        if (pthread_create(&(ssh_client->client_thread), NULL,
                    ssh_client_thread, (void*) client)) {
            guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                    "Unable to start SSH client thread");
            return 1;
        }

    }

    /* If not owner, synchronize with current display */
    else {
        guac_terminal_dup(ssh_client->term, user, user->socket);
        guac_socket_flush(user->socket);
    }

    /* Set per-user event handlers */
    user->key_handler       = guac_ssh_user_key_handler;
    user->mouse_handler     = guac_ssh_user_mouse_handler;
    user->size_handler      = guac_ssh_user_size_handler;
    user->clipboard_handler = guac_ssh_clipboard_handler;

    /* Set generic (non-filesystem) file upload handler */
    user->file_handler = guac_sftp_file_handler;

    return 0;

}

