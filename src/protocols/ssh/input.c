/*
 * Copyright (C) 2016 Glyptodon LLC
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

#include "guac_cursor.h"
#include "guac_display.h"
#include "ssh.h"
#include "terminal.h"

#include <guacamole/client.h>
#include <guacamole/user.h>
#include <libssh2.h>

#include <pthread.h>

int guac_ssh_user_mouse_handler(guac_user* user, int x, int y, int mask) {

    guac_client* client = user->client;
    guac_ssh_client* ssh_client = (guac_ssh_client*) client->data;
    guac_terminal* term = ssh_client->term;

    /* Skip if terminal not yet ready */
    if (term == NULL)
        return 0;

    /* Send mouse event */
    guac_terminal_send_mouse(term, user, x, y, mask);
    return 0;
}

int guac_ssh_user_key_handler(guac_user* user, int keysym, int pressed) {

    guac_client* client = user->client;
    guac_ssh_client* ssh_client = (guac_ssh_client*) client->data;
    guac_terminal* term = ssh_client->term;

    /* Skip if terminal not yet ready */
    if (term == NULL)
        return 0;

    /* Send key */
    guac_terminal_send_key(term, keysym, pressed);
    return 0;
}

int guac_ssh_user_size_handler(guac_user* user, int width, int height) {

    /* Get terminal */
    guac_client* client = user->client;
    guac_ssh_client* ssh_client = (guac_ssh_client*) client->data;
    guac_terminal* terminal = ssh_client->term;

    /* Skip if terminal not yet ready */
    if (terminal == NULL)
        return 0;

    /* Resize terminal */
    guac_terminal_resize(terminal, width, height);

    /* Update SSH pty size if connected */
    if (ssh_client->term_channel != NULL) {
        pthread_mutex_lock(&(ssh_client->term_channel_lock));
        libssh2_channel_request_pty_size(ssh_client->term_channel,
                terminal->term_width, terminal->term_height);
        pthread_mutex_unlock(&(ssh_client->term_channel_lock));
    }

    return 0;
}

