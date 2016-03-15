/*
 * Copyright (C) 2016 Glyptodon, Inc.
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
#include "settings.h"
#include "telnet.h"
#include "terminal.h"
#include "user.h"

#include <guacamole/client.h>
#include <guacamole/socket.h>
#include <guacamole/user.h>

#include <pthread.h>
#include <string.h>

int guac_telnet_user_join_handler(guac_user* user, int argc, char** argv) {

    guac_client* client = user->client;
    guac_telnet_client* telnet_client = (guac_telnet_client*) client->data;

    /* Connect via telnet if owner */
    if (user->owner) {

        /* Parse arguments into client */
        guac_telnet_settings* settings = telnet_client->settings =
            guac_telnet_parse_args(user, argc, (const char**) argv);

        /* Fail if settings cannot be parsed */
        if (settings == NULL) {
            guac_user_log(user, GUAC_LOG_INFO,
                    "Badly formatted client arguments.");
            return 1;
        }

        /* Start client thread */
        if (pthread_create(&(telnet_client->client_thread), NULL,
                    guac_telnet_client_thread, (void*) client)) {
            guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                    "Unable to start telnet client thread");
            return 1;
        }

    }

    /* If not owner, synchronize with current display */
    else {
        guac_terminal_dup(telnet_client->term, user, user->socket);
        guac_socket_flush(user->socket);
    }

    /* Set per-user event handlers */
    user->key_handler       = guac_telnet_user_key_handler;
    user->mouse_handler     = guac_telnet_user_mouse_handler;
    user->size_handler      = guac_telnet_user_size_handler;
    user->clipboard_handler = guac_telnet_clipboard_handler;

    return 0;

}

