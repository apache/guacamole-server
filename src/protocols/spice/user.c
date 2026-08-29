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
#include "audio.h"
#include "channels/file-upload.h"
#include "clipboard.h"
#include "common/clipboard.h"
#include "input.h"
#include "settings.h"
#include "spice.h"
#include "user.h"

#ifdef ENABLE_COMMON_SSH
#include "sftp.h"
#endif

#include <guacamole/argv.h>
#include <guacamole/client.h>
#include <guacamole/display.h>
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

    /* Connect to SPICE server if owner */
    if (user->owner) {

        /* Store owner's settings at client level */
        spice_client->settings = settings;

        /* Init clipboard */
        spice_client->clipboard =
            guac_common_clipboard_alloc(settings->clipboard_buffer_size);

        /* Start client thread */
        if (pthread_create(&spice_client->client_thread, NULL,
                    guac_spice_client_thread, user->client)) {
            guac_user_log(user, GUAC_LOG_ERROR,
                    "Unable to start SPICE client thread.");
            return 1;
        }

    }

    /* Only handle events if not read-only */
    if (!settings->read_only) {

        /* General mouse/keyboard events */
        user->mouse_handler = guac_spice_user_mouse_handler;
        user->key_handler = guac_spice_user_key_handler;

        /* Dynamic display resize (client resolution -> guest), unless disabled */
        if (!settings->disable_display_resize)
            user->size_handler = guac_spice_user_size_handler;

        /* Inbound (client-to-server) clipboard transfer */
        if (!settings->disable_paste && !settings->disable_clipboard)
            user->clipboard_handler = guac_spice_clipboard_handler;

        /* Inbound (client-to-server) audio, if enabled */
        if (settings->audio_input_enabled)
            user->audio_handler = guac_spice_client_audio_record_handler;

#ifdef ENABLE_COMMON_SSH
        /* Set generic (non-filesystem) file upload handler */
        if (settings->enable_sftp && !settings->sftp_disable_upload)
            user->file_handler = guac_spice_sftp_file_handler;
#endif

        /* Route the generic (non-filesystem) upload gesture according to the
         * file-transfer mode, taking precedence over SFTP. In "agent"/"both"
         * dropped files are pushed into the guest via the SPICE agent; in
         * "drive" they are written to the shared folder. The exposed filesystem
         * object still handles its own get/put (download/ls/upload) requests
         * for the drive browser regardless of this routing. */
        if (!settings->disable_upload) {
            switch (settings->file_transfer_mode) {

                case GUAC_SPICE_FILE_TRANSFER_AGENT:
                case GUAC_SPICE_FILE_TRANSFER_BOTH:
                    user->file_handler = guac_spice_file_upload_agent_handler;
                    break;

                case GUAC_SPICE_FILE_TRANSFER_DRIVE:
                    user->file_handler = guac_spice_file_upload_file_handler;
                    break;

                case GUAC_SPICE_FILE_TRANSFER_NONE:
                    break;

            }
        }

    }

    /* Update connection parameters (e.g. password) if we own the connection */
    if (user->owner)
        user->argv_handler = guac_argv_handler;

    return 0;

}

int guac_spice_user_leave_handler(guac_user* user) {

    guac_spice_client* spice_client = (guac_spice_client*) user->client->data;

    if (spice_client->display)
        guac_display_notify_user_left(spice_client->display, user);

    /* Free settings if not owner (owner settings are freed with the client) */
    if (!user->owner) {
        guac_spice_settings* settings = (guac_spice_settings*) user->data;
        guac_spice_settings_free(settings);
    }

    return 0;

}
