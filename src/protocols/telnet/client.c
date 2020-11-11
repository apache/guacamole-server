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
#include "client.h"
#include "common/recording.h"
#include "settings.h"
#include "telnet.h"
#include "terminal/terminal.h"
#include "user.h"

#include <langinfo.h>
#include <locale.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <guacamole/argv.h>
#include <guacamole/client.h>

int guac_client_init(guac_client* client) {

    /* Set client args */
    client->args = GUAC_TELNET_CLIENT_ARGS;

    /* Allocate client instance data */
    guac_telnet_client* telnet_client = calloc(1, sizeof(guac_telnet_client));
    client->data = telnet_client;

    /* Init clipboard */
    telnet_client->clipboard = guac_common_clipboard_alloc(GUAC_TELNET_CLIPBOARD_MAX_LENGTH);

    /* Init telnet client */
    telnet_client->socket_fd = -1;
    telnet_client->naws_enabled = 0;
    telnet_client->echo_enabled = 1;

    /* Set handlers */
    client->join_handler = guac_telnet_user_join_handler;
    client->free_handler = guac_telnet_client_free_handler;

    /* Register handlers for argument values that may be sent after the handshake */
    guac_argv_register(GUAC_TELNET_ARGV_COLOR_SCHEME, guac_telnet_argv_callback, NULL, GUAC_ARGV_OPTION_ECHO);
    guac_argv_register(GUAC_TELNET_ARGV_FONT_NAME, guac_telnet_argv_callback, NULL, GUAC_ARGV_OPTION_ECHO);
    guac_argv_register(GUAC_TELNET_ARGV_FONT_SIZE, guac_telnet_argv_callback, NULL, GUAC_ARGV_OPTION_ECHO);

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

    /* Clean up recording, if in progress */
    if (telnet_client->recording != NULL)
        guac_common_recording_free(telnet_client->recording);

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

    guac_common_clipboard_free(telnet_client->clipboard);
    free(telnet_client);
    return 0;

}

