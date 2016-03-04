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

#include "clipboard.h"
#include "input.h"
#include "guac_display.h"
#include "guac_dot_cursor.h"
#include "guac_pointer_cursor.h"
#include "user.h"
#include "sftp.h"
#include "vnc.h"

#include <guacamole/audio.h>
#include <guacamole/client.h>
#include <guacamole/socket.h>
#include <guacamole/user.h>
#include <rfb/rfbclient.h>
#include <rfb/rfbproto.h>

#include <pthread.h>

int guac_vnc_user_join_handler(guac_user* user, int argc, char** argv) {

    guac_vnc_client* vnc_client = (guac_vnc_client*) user->client->data;

    /* Connect via VNC if owner */
    if (user->owner) {

        /* Parse arguments into client */
        guac_vnc_settings* settings = vnc_client->settings =
            guac_vnc_parse_args(user, argc, (const char**) argv);

        /* Fail if settings cannot be parsed */
        if (settings == NULL) {
            guac_user_log(user, GUAC_LOG_INFO,
                    "Badly formatted client arguments.");
            return 1;
        }

        /* Start client thread */
        if (pthread_create(&vnc_client->client_thread, NULL, guac_vnc_client_thread, user->client)) {
            guac_user_log(user, GUAC_LOG_ERROR, "Unable to start VNC client thread.");
            return 1;
        }

    }

    /* If not owner, synchronize with current state */
    else {

#ifdef ENABLE_PULSE
        /* Synchronize an audio stream */
        if (vnc_client->audio)
            guac_audio_stream_add_user(vnc_client->audio, user);
#endif

        /* Synchronize with current display */
        guac_common_display_dup(vnc_client->display, user, user->socket);
        guac_socket_flush(user->socket);

    }

    /* Only handle events if not read-only */
    if (vnc_client->settings->read_only == 0) {

        /* General mouse/keyboard/clipboard events */
        user->mouse_handler     = guac_vnc_user_mouse_handler;
        user->key_handler       = guac_vnc_user_key_handler;
        user->clipboard_handler = guac_vnc_clipboard_handler;

#ifdef ENABLE_COMMON_SSH
        /* Set generic (non-filesystem) file upload handler */
        user->file_handler = guac_vnc_sftp_file_handler;
#endif

    }

    return 0;

}

int guac_vnc_user_leave_handler(guac_user* user) {

    guac_vnc_client* vnc_client = (guac_vnc_client*) user->client->data;

    guac_common_cursor_remove_user(vnc_client->display->cursor, user);

    return 0;
}

