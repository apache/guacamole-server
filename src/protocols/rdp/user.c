/*
 * Copyright (C) 2014 Glyptodon LLC
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

