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
#include "settings.h"
#include "telnet.h"
#include "terminal.h"
#include "user.h"

#include <langinfo.h>
#include <locale.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <guacamole/client.h>

int guac_client_init(guac_client* client) {

    /* Set client args */
    client->args = GUAC_TELNET_CLIENT_ARGS;

    /* Allocate client instance data */
    guac_telnet_client* telnet_client = calloc(1, sizeof(guac_telnet_client));
    client->data = telnet_client;

    /* Init telnet client */
    telnet_client->socket_fd = -1;
    telnet_client->naws_enabled = 0;
    telnet_client->echo_enabled = 1;

    /* Set handlers */
    client->join_handler = guac_telnet_user_join_handler;
    client->free_handler = guac_telnet_client_free_handler;

    /* Set locale and warn if not UTF-8 */
    setlocale(LC_CTYPE, "");
    if (strcmp(nl_langinfo(CODESET), "UTF-8") != 0) {
        guac_client_log(client, GUAC_LOG_INFO,
                "Current locale does not use UTF-8. Some characters may "
                "not render correctly.");
    }

    /* Success */
    return 0;

}

int guac_telnet_client_free_handler(guac_client* client) {

    guac_telnet_client* telnet_client = (guac_telnet_client*) client->data;

    /* Close telnet connection */
    if (telnet_client->socket_fd != -1)
        close(telnet_client->socket_fd);

    /* Kill terminal */
    guac_terminal_free(telnet_client->term);

    /* Wait for and free telnet session, if connected */
    if (telnet_client->telnet != NULL) {
        pthread_join(telnet_client->client_thread, NULL);
        telnet_free(telnet_client->telnet);
    }

    /* Free settings */
    if (telnet_client->settings != NULL)
        guac_telnet_settings_free(telnet_client->settings);

    free(telnet_client);
    return 0;

}

