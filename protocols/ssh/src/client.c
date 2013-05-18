
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is libguac-client-ssh.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * James Muehlner <dagger10k@users.sourceforge.net>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <guacamole/socket.h>
#include <guacamole/protocol.h>
#include <guacamole/client.h>
#include <guacamole/error.h>

#include "client.h"
#include "guac_handlers.h"
#include "terminal.h"
#include "blank.h"
#include "ibar.h"
#include "ssh_client.h"

/* Client plugin arguments */
const char* GUAC_CLIENT_ARGS[] = {
    "hostname",
    "username",
    "password",
    NULL
};

int guac_client_init(guac_client* client, int argc, char** argv) {

    guac_socket* socket = client->socket;

    ssh_guac_client_data* client_data = malloc(sizeof(ssh_guac_client_data));
    guac_terminal* term = guac_terminal_create(client,
            client->info.optimal_width, client->info.optimal_height);

    /* Init client data */
    client->data = client_data;
    client_data->term = term;
    client_data->mod_ctrl = 0;
    client_data->clipboard_data = NULL;

    /* Set up I-bar pointer */
    client_data->ibar_cursor = guac_ssh_create_ibar(client);

    /* Set up blank pointer */
    client_data->blank_cursor = guac_ssh_create_blank(client);

    /* Send name and dimensions */
    guac_protocol_send_name(socket, "Terminal");

    /* Initialize pointer */
    client_data->current_cursor = client_data->blank_cursor;
    guac_ssh_set_cursor(client, client_data->current_cursor);

    guac_socket_flush(socket);

    /* Open STDOUT pipe */
    if (pipe(client_data->stdout_pipe_fd)) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Unable to open pipe for STDOUT";
        return 1;
    }

    /* Redirect STDOUT to pipe */
    if (dup2(client_data->stdout_pipe_fd[1], STDOUT_FILENO) < 0) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Unable redirect STDOUT";
        return 1;
    }

    /* Open STDIN pipe */
    if (pipe(client_data->stdin_pipe_fd)) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Unable to open pipe for STDIN";
        return 1;
    }

    /* Redirect STDIN to pipe */
    if (dup2(client_data->stdin_pipe_fd[0], STDIN_FILENO) < 0) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Unable redirect STDIN";
        return 1;
    }

    /* Set basic handlers */
    client->handle_messages   = ssh_guac_client_handle_messages;
    client->clipboard_handler = ssh_guac_client_clipboard_handler;
    client->key_handler       = ssh_guac_client_key_handler;
    client->mouse_handler     = ssh_guac_client_mouse_handler;
    client->size_handler      = ssh_guac_client_size_handler;
    client->free_handler      = ssh_guac_client_free_handler;

    /* Start client thread */
    if (pthread_create(&(client_data->client_thread), NULL, ssh_client_thread, (void*) client)) {
        guac_client_log_error(client, "Unable to SSH client thread");
        return -1;
    }

    /* Success */
    return 0;

}

