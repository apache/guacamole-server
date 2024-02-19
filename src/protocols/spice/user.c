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

#include "channels/audio.h"
#include "channels/clipboard.h"
#include "input.h"
#include "common/display.h"
#include "common/dot_cursor.h"
#include "common/pointer_cursor.h"
#include "user.h"
#include "sftp.h"
#include "spice.h"

#include <guacamole/argv.h>
#include <guacamole/audio.h>
#include <guacamole/client.h>
#include <guacamole/socket.h>
#include <guacamole/user.h>

#include <pthread.h>

int guac_spice_user_join_handler(guac_user* user, int argc, char** argv) {

    guac_spice_client* spice_client = (guac_spice_client*) user->client->data;

    /* Parse provided arguments */
    guac_spice_settings* settings = guac_spice_parse_args(user,
            argc, (const char**) argv);

    /* Fail if settings cannot be parsed */
    if (settings == NULL) {
        guac_user_log(user, GUAC_LOG_INFO,
                "Badly formatted client arguments.");
        return 1;
    }

    /* Store settings at user level */
    user->data = settings;

    /* Connect via Spice if owner */
    if (user->owner) {

        /* Store owner's settings at client level */
        spice_client->settings = settings;

        /* Start client thread */
        if (pthread_create(&spice_client->client_thread, NULL, guac_spice_client_thread, user->client)) {
            guac_user_log(user, GUAC_LOG_ERROR, "Unable to start SPICE client thread.");
            return 1;
        }

        /* Handle inbound audio streams if audio input is enabled */
        if (settings->audio_input_enabled)
            user->audio_handler = guac_spice_client_audio_record_handler;

    }

    /* If not owner, synchronize with current state */
    else {

        /* Synchronize with current display */
        guac_common_display_dup(spice_client->display, user->client, user->socket);
        guac_socket_flush(user->socket);

    }

    /* Only handle events if not read-only */
    if (!settings->read_only) {

        /* General mouse/keyboard events */
        user->mouse_handler = guac_spice_user_mouse_handler;
        user->key_handler = guac_spice_user_key_handler;

        /* Inbound (client to server) clipboard transfer */
        if (!settings->disable_paste)
            user->clipboard_handler = guac_spice_clipboard_handler;
        
        /* Updates to connection parameters if we own the connection */
        if (user->owner)
            user->argv_handler = guac_argv_handler;

#ifdef ENABLE_COMMON_SSH
        /* Set generic (non-filesystem) file upload handler */
        if (settings->enable_sftp && !settings->sftp_disable_upload)
            user->file_handler = guac_spice_sftp_file_handler;
#endif

    }

    return 0;

}

int guac_spice_user_leave_handler(guac_user* user) {

    guac_spice_client* spice_client = (guac_spice_client*) user->client->data;

    if (spice_client->display) {
        /* Update shared cursor state */
        guac_common_cursor_remove_user(spice_client->display->cursor, user);
    }

    /* Free settings if not owner (owner settings will be freed with client) */
    if (!user->owner) {
        guac_spice_settings* settings = (guac_spice_settings*) user->data;
        guac_spice_settings_free(settings);
    }

    return 0;
}

