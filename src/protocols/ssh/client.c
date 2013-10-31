
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

#define GUAC_SSH_DEFAULT_FONT_NAME "monospace" 
#define GUAC_SSH_DEFAULT_FONT_SIZE 12
#define GUAC_SSH_DEFAULT_PORT      22

/* Client plugin arguments */
const char* GUAC_CLIENT_ARGS[] = {
    "hostname",
    "port",
    "username",
    "password",
    "font-name",
    "font-size",
    "enable-sftp",
#ifdef ENABLE_SSH_PUBLIC_KEY
    "private-key",
    "passphrase",
#endif
    NULL
};

enum __SSH_ARGS_IDX {

    /**
     * The hostname to connect to. Required.
     */
    IDX_HOSTNAME,

    /**
     * The port to connect to. Optional.
     */
    IDX_PORT,

    /**
     * The name of the user to login as. Optional.
     */
    IDX_USERNAME,

    /**
     * The password to use when logging in. Optional.
     */
    IDX_PASSWORD,

    /**
     * The name of the font to use within the terminal.
     */
    IDX_FONT_NAME,

    /**
     * The size of the font to use within the terminal, in points.
     */
    IDX_FONT_SIZE,

    /**
     * Whether SFTP should be enabled.
     */
    IDX_ENABLE_SFTP,

#ifdef ENABLE_SSH_PUBLIC_KEY
    /**
     * The private key to use for authentication, if any.
     */
    IDX_PRIVATE_KEY,

    /**
     * The passphrase required to decrypt the private key, if any.
     */
    IDX_PASSPHRASE,
#endif

    SSH_ARGS_COUNT
};

int guac_client_init(guac_client* client, int argc, char** argv) {

    guac_socket* socket = client->socket;

    ssh_guac_client_data* client_data = malloc(sizeof(ssh_guac_client_data));

    /* Init client data */
    client->data = client_data;
    client_data->mod_alt   =
    client_data->mod_ctrl  =
    client_data->mod_shift = 0;
    client_data->clipboard_data = NULL;
    client_data->term_channel = NULL;

    if (argc != SSH_ARGS_COUNT) {
        guac_client_log_error(client, "Wrong number of arguments");
        return -1;
    }

    /* Read parameters */
    strcpy(client_data->hostname,  argv[IDX_HOSTNAME]);
    strcpy(client_data->username,  argv[IDX_USERNAME]);
    strcpy(client_data->password,  argv[IDX_PASSWORD]);

#ifdef ENABLE_SSH_PUBLIC_KEY
    /* Init public key auth information */
    client_data->key = NULL;
    strcpy(client_data->key_base64,     argv[IDX_PRIVATE_KEY]);
    strcpy(client_data->key_passphrase, argv[IDX_PASSPHRASE]);
#endif

    /* Read font name */
    if (argv[IDX_FONT_NAME][0] != 0)
        strcpy(client_data->font_name, argv[IDX_FONT_NAME]);
    else
        strcpy(client_data->font_name, GUAC_SSH_DEFAULT_FONT_NAME );

    /* Read font size */
    if (argv[IDX_FONT_SIZE][0] != 0)
        client_data->font_size = atoi(argv[IDX_FONT_SIZE]);
    else
        client_data->font_size = GUAC_SSH_DEFAULT_FONT_SIZE;

    /* Parse SFTP enable */
    client_data->enable_sftp = strcmp(argv[IDX_ENABLE_SFTP], "true") == 0;
    client_data->sftp_session = NULL;
    client_data->sftp_ssh_session = NULL;
    strcpy(client_data->sftp_upload_path, ".");

    /* Read port */
    if (argv[IDX_PORT][0] != 0)
        client_data->port = atoi(argv[IDX_PORT]);
    else
        client_data->port = GUAC_SSH_DEFAULT_PORT;

    /* Create terminal */
    client_data->term = guac_terminal_create(client,
            client_data->font_name, client_data->font_size,
            client->info.optimal_width, client->info.optimal_height);

    /* Fail if terminal init failed */
    if (client_data->term == NULL) {
        guac_error = GUAC_STATUS_BAD_STATE;
        guac_error_message = "Terminal initialization failed";
        return -1;
    }

    /* Set up I-bar pointer */
    client_data->ibar_cursor = guac_ssh_create_ibar(client);

    /* Set up blank pointer */
    client_data->blank_cursor = guac_ssh_create_blank(client);

    /* Send initial name */
    guac_protocol_send_name(socket, client_data->hostname);

    /* Initialize pointer */
    client_data->current_cursor = client_data->blank_cursor;
    guac_ssh_set_cursor(client, client_data->current_cursor);

    guac_socket_flush(socket);

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

