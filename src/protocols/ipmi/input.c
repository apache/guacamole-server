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
#include "ipmi.h"
#include "menu.h"
#include "terminal/terminal.h"

#include <guacamole/client.h>
#include <guacamole/recording.h>
#include <guacamole/user.h>

#include <stdlib.h>
#include <sys/types.h>

int guac_ipmi_user_mouse_handler(guac_user* user, int x, int y, int mask) {

    guac_client* client = user->client;
    guac_ipmi_client* ipmi_client = (guac_ipmi_client*) client->data;
    guac_terminal* term = ipmi_client->term;

    /* Skip if terminal not yet ready */
    if (term == NULL)
        return 0;

    /* Report mouse position within recording */
    if (ipmi_client->recording != NULL)
        guac_recording_report_mouse(ipmi_client->recording, x, y, mask);

    /* Send mouse event to terminal */
    guac_terminal_send_mouse(term, user, x, y, mask);

    return 0;

}

int guac_ipmi_user_key_handler(guac_user* user, int keysym, int pressed) {

    guac_client* client = user->client;
    guac_ipmi_client* ipmi_client = (guac_ipmi_client*) client->data;
    guac_terminal* term = ipmi_client->term;

    /* Report key state within recording */
    if (ipmi_client->recording != NULL)
        guac_recording_report_key(ipmi_client->recording, keysym, pressed);

    /* Skip if terminal not yet ready */
    if (term == NULL)
        return 0;

    /* While the control menu is open, interpret key presses as menu commands
     * rather than forwarding them to the serial console. Key releases are
     * ignored. */
    if (ipmi_client->menu_open) {
        if (pressed)
            guac_ipmi_menu_handle_key(client, keysym);
        return 0;
    }

    /* Open the control menu when the menu key (Ctrl + ']') is pressed */
    if (pressed && keysym == GUAC_IPMI_MENU_KEYSYM
            && guac_terminal_get_mod_ctrl(term)) {
        guac_ipmi_menu_open(client);
        return 0;
    }

    /* Send key to terminal */
    guac_terminal_send_key(term, keysym, pressed);

    return 0;

}

int guac_ipmi_user_size_handler(guac_user* user, int width, int height) {

    /* Get terminal */
    guac_client* client = user->client;
    guac_ipmi_client* ipmi_client = (guac_ipmi_client*) client->data;
    guac_terminal* terminal = ipmi_client->term;

    /* Skip if terminal not yet ready */
    if (terminal == NULL)
        return 0;

    /* Resize terminal. The IPMI SOL protocol provides no mechanism to inform
     * the remote serial console of a window size change, so only the local
     * terminal is resized. */
    guac_terminal_resize(terminal, width, height);

    return 0;

}
