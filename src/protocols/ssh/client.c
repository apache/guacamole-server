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

#include "client.h"
#include "guac_sftp.h"
#include "ssh.h"
#include "terminal.h"
#include "user.h"

#include <langinfo.h>
#include <locale.h>
#include <stdlib.h>
#include <string.h>

#include <guacamole/client.h>

int guac_client_init(guac_client* client) {

    /* Set client args */
    client->args = GUAC_SSH_CLIENT_ARGS;

    /* Allocate client instance data */
    guac_ssh_client* ssh_client = calloc(1, sizeof(guac_ssh_client));
    client->data = ssh_client;

    /* Set handlers */
    client->join_handler = guac_ssh_user_join_handler;
    client->free_handler = guac_ssh_client_free_handler;

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
        guac_terminal_free(ssh_client->term);
        pthread_join(ssh_client->client_thread, NULL);
    }

    /* Free terminal channel now that the terminal is finished */
    if (ssh_client->term_channel != NULL)
        libssh2_channel_free(ssh_client->term_channel);

    /* Clean up the SFTP filesystem object and session */
    if (ssh_client->sftp_filesystem) {
        guac_common_ssh_destroy_sftp_filesystem(ssh_client->sftp_filesystem);
        guac_common_ssh_destroy_session(ssh_client->sftp_session);
    }

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
    free(ssh_client);

    guac_common_ssh_uninit();
    return 0;
}

