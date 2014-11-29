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
#include "client.h"
#include "guac_handlers.h"
#include "terminal.h"
#include "telnet_client.h"

#include <guacamole/client.h>
#include <libtelnet.h>

#include <pthread.h>
#include <regex.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

int guac_telnet_client_handle_messages(guac_client* client) {

    guac_telnet_client_data* client_data = (guac_telnet_client_data*) client->data;
    return guac_terminal_render_frame(client_data->term);

}

int guac_telnet_client_mouse_handler(guac_client* client, int x, int y, int mask) {

    guac_telnet_client_data* client_data = (guac_telnet_client_data*) client->data;
    guac_terminal* term = client_data->term;

    /* Send mouse if not searching for password or username */
    if (client_data->password_regex == NULL && client_data->username_regex == NULL)
        guac_terminal_send_mouse(term, x, y, mask);

    return 0;

}

int guac_telnet_client_key_handler(guac_client* client, int keysym, int pressed) {

    guac_telnet_client_data* client_data = (guac_telnet_client_data*) client->data;
    guac_terminal* term = client_data->term;

    /* Stop searching for password */
    if (client_data->password_regex != NULL) {

        guac_client_log(client, GUAC_LOG_DEBUG,
                "Stopping password prompt search due to user input.");

        regfree(client_data->password_regex);
        free(client_data->password_regex);
        client_data->password_regex = NULL;

    }

    /* Stop searching for username */
    if (client_data->username_regex != NULL) {

        guac_client_log(client, GUAC_LOG_DEBUG,
                "Stopping username prompt search due to user input.");

        regfree(client_data->username_regex);
        free(client_data->username_regex);
        client_data->username_regex = NULL;

    }

    /* Send key */
    guac_terminal_send_key(term, keysym, pressed);

    return 0;

}

int guac_telnet_client_size_handler(guac_client* client, int width, int height) {

    /* Get terminal */
    guac_telnet_client_data* guac_client_data = (guac_telnet_client_data*) client->data;
    guac_terminal* terminal = guac_client_data->term;

    /* Resize terminal */
    guac_terminal_resize(terminal, width, height);

    /* Update terminal window size if connected */
    if (guac_client_data->telnet != NULL && guac_client_data->naws_enabled)
        guac_telnet_send_naws(guac_client_data->telnet, terminal->term_width, terminal->term_height);

    return 0;
}

int guac_telnet_client_free_handler(guac_client* client) {

    guac_telnet_client_data* guac_client_data = (guac_telnet_client_data*) client->data;

    /* Close telnet connection */
    if (guac_client_data->socket_fd != -1)
        close(guac_client_data->socket_fd);

    /* Kill terminal */
    guac_terminal_free(guac_client_data->term);

    /* Wait for and free telnet session, if connected */
    if (guac_client_data->telnet != NULL) {
        pthread_join(guac_client_data->client_thread, NULL);
        telnet_free(guac_client_data->telnet);
    }

    /* Free password regex */
    if (guac_client_data->password_regex != NULL) {
        regfree(guac_client_data->password_regex);
        free(guac_client_data->password_regex);
    }

    free(client->data);
    return 0;

}

