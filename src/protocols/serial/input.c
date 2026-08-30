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
#include "serial.h"
#include "stream.h"
#include "terminal/terminal.h"

#include <guacamole/client.h>
#include <guacamole/recording.h>
#include <guacamole/user.h>

#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int guac_serial_user_mouse_handler(guac_user* user, int x, int y, int mask) {

    guac_client* client = user->client;
    guac_serial_client* serial_client = (guac_serial_client*) client->data;
    guac_terminal* term = serial_client->term;

    /* Skip if terminal not yet ready */
    if (term == NULL)
        return 0;

    /* Report mouse position within recording */
    if (serial_client->recording != NULL)
        guac_recording_report_mouse(serial_client->recording, x, y, mask);

    /* Send mouse event to terminal */
    guac_terminal_send_mouse(term, user, x, y, mask);

    return 0;

}

int guac_serial_user_key_handler(guac_user* user, int keysym, int pressed) {

    guac_client* client = user->client;
    guac_serial_client* serial_client = (guac_serial_client*) client->data;
    guac_terminal* term = serial_client->term;

    /* Report key state within recording */
    if (serial_client->recording != NULL)
        guac_recording_report_key(serial_client->recording, keysym, pressed);

    /* Skip if terminal not yet ready */
    if (term == NULL)
        return 0;

    /* Intercept and handle Pause / Break / Ctrl+0 as a serial break */
    if (pressed && (
                keysym == 0xFF13                  /* Pause */
             || keysym == 0xFF6B                  /* Break */
             || (
                    guac_terminal_get_mod_ctrl(term)
                    && keysym == '0'
                )                                 /* Ctrl + 0 */
       )) {

        /* Send a serial break if the connection is established */
        if (serial_client->stream != NULL)
            guac_serial_stream_send_break(serial_client->stream);

        return 0;
    }

    /* Send key */
    guac_terminal_send_key(term, keysym, pressed);

    return 0;

}

int guac_serial_user_size_handler(guac_user* user, int width, int height) {

    /* Get terminal */
    guac_client* client = user->client;
    guac_serial_client* serial_client = (guac_serial_client*) client->data;
    guac_terminal* terminal = serial_client->term;

    /* Skip if terminal not yet ready */
    if (terminal == NULL)
        return 0;

    /* Resize terminal */
    guac_terminal_resize(terminal, width, height);

    return 0;
}
