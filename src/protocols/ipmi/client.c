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
#include "ipmi.h"
#include "settings.h"
#include "terminal/terminal.h"
#include "user.h"

#include <langinfo.h>
#include <locale.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <guacamole/argv.h>
#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <guacamole/recording.h>
#include <guacamole/socket.h>

#include <ipmiconsole.h>

/**
 * A pending join handler implementation that will synchronize the connection
 * state for all pending users prior to them being promoted to full user.
 *
 * @param client
 *     The client whose pending users are about to be promoted.
 *
 * @return
 *     Always zero.
 */
static int guac_ipmi_join_pending_handler(guac_client* client) {

    guac_ipmi_client* ipmi_client = (guac_ipmi_client*) client->data;

    /* Synchronize the terminal state to all pending users */
    if (ipmi_client->term != NULL) {
        guac_socket* broadcast_socket = client->pending_socket;
        guac_terminal_sync_users(ipmi_client->term, client, broadcast_socket);
        guac_ipmi_send_current_argv_batch(client, broadcast_socket);
        guac_socket_flush(broadcast_socket);
    }

    return 0;

}

int guac_client_init(guac_client* client) {

    /* Set client args */
    client->args = GUAC_IPMI_CLIENT_ARGS;

    /* Allocate client instance data */
    guac_ipmi_client* ipmi_client = guac_mem_zalloc(sizeof(guac_ipmi_client));
    client->data = ipmi_client;

    /* Init IPMI client */
    ipmi_client->console_fd = -1;
    ipmi_client->menu_open = false;
    ipmi_client->menu_pending_action = GUAC_IPMI_POWER_NONE;

    /* Set handlers */
    client->join_handler = guac_ipmi_user_join_handler;
    client->join_pending_handler = guac_ipmi_join_pending_handler;
    client->free_handler = guac_ipmi_client_free_handler;
    client->leave_handler = guac_ipmi_user_leave_handler;

    /* Register handlers for argument values that may be sent after the handshake */
    guac_argv_register(GUAC_IPMI_ARGV_COLOR_SCHEME, guac_ipmi_argv_callback, NULL, GUAC_ARGV_OPTION_ECHO);
    guac_argv_register(GUAC_IPMI_ARGV_FONT_NAME, guac_ipmi_argv_callback, NULL, GUAC_ARGV_OPTION_ECHO);
    guac_argv_register(GUAC_IPMI_ARGV_FONT_SIZE, guac_ipmi_argv_callback, NULL, GUAC_ARGV_OPTION_ECHO);

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

int guac_ipmi_client_free_handler(guac_client* client) {

    guac_ipmi_client* ipmi_client = (guac_ipmi_client*) client->data;

    /* Close the SOL file descriptor, unblocking the client thread's read loop.
     * As the IPMICONSOLE_ENGINE_CLOSE_FD flag is not used, the descriptor is
     * owned by us and must be closed here. */
    if (ipmi_client->console_fd != -1)
        close(ipmi_client->console_fd);

    /* Clean up recording, if in progress */
    if (ipmi_client->recording != NULL)
        guac_recording_free(ipmi_client->recording);

    /* Kill terminal, unblocking the input thread */
    guac_terminal_free(ipmi_client->term);

    /* Wait for and clean up the SOL session, if established */
    if (ipmi_client->console_ctx != NULL) {
        pthread_join(ipmi_client->client_thread, NULL);
        ipmiconsole_ctx_destroy(ipmi_client->console_ctx);
        guac_ipmi_engine_unref();
    }

    /* Free settings */
    if (ipmi_client->settings != NULL)
        guac_ipmi_settings_free(ipmi_client->settings);

    guac_mem_free(ipmi_client);
    return 0;

}
