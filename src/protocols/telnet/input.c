/*
 * Copyright (C) 2013 Glyptodon LLC
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
#include "terminal.h"
#include "telnet.h"

#include <guacamole/client.h>
#include <guacamole/user.h>
#include <libtelnet.h>

#include <regex.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int guac_telnet_user_mouse_handler(guac_user* user, int x, int y, int mask) {

    guac_client* client = user->client;
    guac_telnet_client* telnet_client = (guac_telnet_client*) client->data;
    guac_telnet_settings* settings = telnet_client->settings;
    guac_terminal* term = telnet_client->term;

    /* Send mouse if not searching for password or username */
    if (settings->password_regex == NULL && settings->username_regex == NULL)
        guac_terminal_send_mouse(term, user, x, y, mask);

    return 0;

}

int guac_telnet_user_key_handler(guac_user* user, int keysym, int pressed) {

    guac_client* client = user->client;
    guac_telnet_client* telnet_client = (guac_telnet_client*) client->data;
    guac_telnet_settings* settings = telnet_client->settings;
    guac_terminal* term = telnet_client->term;

    /* Stop searching for password */
    if (settings->password_regex != NULL) {

        guac_client_log(client, GUAC_LOG_DEBUG,
                "Stopping password prompt search due to user input.");

        regfree(settings->password_regex);
        free(settings->password_regex);
        settings->password_regex = NULL;

    }

    /* Stop searching for username */
    if (settings->username_regex != NULL) {

        guac_client_log(client, GUAC_LOG_DEBUG,
                "Stopping username prompt search due to user input.");

        regfree(settings->username_regex);
        free(settings->username_regex);
        settings->username_regex = NULL;

    }

    /* Intercept and handle Pause / Break / Ctrl+0 as "IAC BRK" */
    if (pressed && (
                keysym == 0xFF13                  /* Pause */
             || keysym == 0xFF6B                  /* Break */
             || (term->mod_ctrl && keysym == '0') /* Ctrl + 0 */
       )) {

        /* Send IAC BRK */
        telnet_iac(telnet_client->telnet, TELNET_BREAK);

        return 0;
    }

    /* Send key */
    guac_terminal_send_key(term, keysym, pressed);

    return 0;

}

int guac_telnet_user_size_handler(guac_user* user, int width, int height) {

    /* Get terminal */
    guac_client* client = user->client;
    guac_telnet_client* telnet_client = (guac_telnet_client*) client->data;
    guac_terminal* terminal = telnet_client->term;

    /* Resize terminal */
    guac_terminal_resize(terminal, width, height);

    /* Update terminal window size if connected */
    if (telnet_client->telnet != NULL && telnet_client->naws_enabled)
        guac_telnet_send_naws(telnet_client->telnet, terminal->term_width,
                terminal->term_height);

    return 0;
}

