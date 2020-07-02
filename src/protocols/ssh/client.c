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
#include "common/clipboard.h"
#include "common/recording.h"
#include "common-ssh/sftp.h"
#include "ssh.h"
#include "terminal/terminal.h"
#include "user.h"

#include <langinfo.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include <guacamole/argv.h>
#include <guacamole/client.h>

int guac_client_init(guac_client* client) {

    /* Set client args */
    client->args = GUAC_SSH_CLIENT_ARGS;

    /* Allocate client instance data */
    guac_ssh_client* ssh_client = calloc(1, sizeof(guac_ssh_client));
    client->data = ssh_client;

    /* Init clipboard */
    ssh_client->clipboard = guac_common_clipboard_alloc(GUAC_SSH_CLIPBOARD_MAX_LENGTH);

    /* Set handlers */
    client->join_handler = guac_ssh_user_join_handler;
    client->free_handler = guac_ssh_client_free_handler;

    /* Register handlers for argument values that may be sent after the handshake */
    guac_argv_register(GUAC_SSH_ARGV_COLOR_SCHEME, guac_ssh_argv_callback, NULL, GUAC_ARGV_OPTION_ECHO);
    guac_argv_register(GUAC_SSH_ARGV_FONT_NAME, guac_ssh_argv_callback, NULL, GUAC_ARGV_OPTION_ECHO);
    guac_argv_register(GUAC_SSH_ARGV_FONT_SIZE, guac_ssh_argv_callback, NULL, GUAC_ARGV_OPTION_ECHO);

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

int guac_ssh_client_free_handler(guac_client* client) {

    guac_ssh_client* ssh_client = (guac_ssh_client*) client->data;

    /* Close SSH channel */
    if (ssh_client->term_channel != NULL) {
        libssh2_channel_send_eof(ssh_client->term_channel);
        libssh2_channel_close(ssh_client->term_channel);
    }

    /* Free terminal (which may still be using term_channel) */
    if (ssh_client->term != NULL) {
        /* Stop the terminal to unblock any pending reads/writes */
        guac_terminal_stop(ssh_client->term);

        /* Wait ssh_client_thread to finish before freeing the terminal */
        pthread_join(ssh_client->client_thread, NULL);
        guac_terminal_free(ssh_client->term);
    }

    /* Free terminal channel now that the terminal is finished */
    if (ssh_client->term_channel != NULL)
        libssh2_channel_free(ssh_client->term_channel);

    /* Clean up the SFTP filesystem object and session */
    if (ssh_client->sftp_filesystem) {
        guac_common_ssh_destroy_sftp_filesystem(ssh_client->sftp_filesystem);
        guac_common_ssh_destroy_session(ssh_client->sftp_session);
    }

    /* Clean up recording, if in progress */
    if (ssh_client->recording != NULL)
        guac_common_recording_free(ssh_client->recording);

    /* Free interactive SSH session */
    if (ssh_client->session != NULL)
        guac_common_ssh_destroy_session(ssh_client->session);

    /* Free SSH client credentials */
    if (ssh_client->user != NULL)
        guac_common_ssh_destroy_user(ssh_client->user);

    /* Free parsed settings */
    if (ssh_client->settings != NULL)
        guac_ssh_settings_free(ssh_client->settings);

    /* Free client structure */
    guac_common_clipboard_free(ssh_client->clipboard);
    free(ssh_client);

    guac_common_ssh_uninit();
    return 0;
}

