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

#include "input.h"
#include "guac_display.h"
#include "user.h"
#include "rdp.h"
#include "rdp_settings.h"
#include "rdp_stream.h"
#include "rdp_svc.h"

#ifdef ENABLE_COMMON_SSH
#include "sftp.h"
#endif

#include <guacamole/audio.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/user.h>

#include <pthread.h>

int guac_rdp_user_join_handler(guac_user* user, int argc, char** argv) {

    guac_rdp_client* rdp_client = (guac_rdp_client*) user->client->data;

    /* Connect via RDP if owner */
    if (user->owner) {

        /* Parse arguments into client */
        guac_rdp_settings* settings = rdp_client->settings =
            guac_rdp_parse_args(user, argc, (const char**) argv);

        /* Fail if settings cannot be parsed */
        if (settings == NULL) {
            guac_user_log(user, GUAC_LOG_INFO,
                    "Badly formatted client arguments.");
            return 1;
        }

        /* Start client thread */
        if (pthread_create(&rdp_client->client_thread, NULL,
                    guac_rdp_client_thread, user->client)) {
            guac_user_log(user, GUAC_LOG_ERROR,
                    "Unable to start VNC client thread.");
            return 1;
        }

    }

    /* If not owner, synchronize with current state */
    else {

        /* Synchronize any audio stream */
        if (rdp_client->audio)
            guac_audio_stream_add_user(rdp_client->audio, user);

        /* Bring user up to date with any registered static channels */
        guac_rdp_svc_send_pipes(user);

        /* Synchronize with current display */
        guac_common_display_dup(rdp_client->display, user, user->socket);
        guac_socket_flush(user->socket);

    }

    user->file_handler = guac_rdp_user_file_handler;
    user->mouse_handler = guac_rdp_user_mouse_handler;
    user->key_handler = guac_rdp_user_key_handler;
    user->size_handler = guac_rdp_user_size_handler;
    user->pipe_handler = guac_rdp_svc_pipe_handler;
    user->clipboard_handler = guac_rdp_clipboard_handler;

    return 0;

}

int guac_rdp_user_file_handler(guac_user* user, guac_stream* stream,
        char* mimetype, char* filename) {

    guac_rdp_client* rdp_client = (guac_rdp_client*) user->client->data;

#ifdef ENABLE_COMMON_SSH
    guac_rdp_settings* settings = rdp_client->settings;

    /* If SFTP is enabled, it should be used for default uploads only if RDPDR
     * is not enabled or its upload directory has been set */
    if (rdp_client->sftp_filesystem != NULL) {
        if (!settings->drive_enabled || settings->sftp_directory != NULL)
            return guac_rdp_sftp_file_handler(user, stream, mimetype, filename);
    }
#endif

    /* Default to using RDPDR uploads (if enabled) */
    if (rdp_client->filesystem != NULL)
        return guac_rdp_upload_file_handler(user, stream, mimetype, filename);

    /* File transfer not enabled */
    guac_protocol_send_ack(user->socket, stream, "File transfer disabled",
            GUAC_PROTOCOL_STATUS_UNSUPPORTED);
    guac_socket_flush(user->socket);

    return 0;
}

int guac_rdp_user_leave_handler(guac_user* user) {

    guac_rdp_client* rdp_client = (guac_rdp_client*) user->client->data;

    guac_common_cursor_remove_user(rdp_client->display->cursor, user);

    return 0;
}

