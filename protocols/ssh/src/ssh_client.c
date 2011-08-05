
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

#include <libssh/libssh.h>

#include <guacamole/log.h>
#include <guacamole/guacio.h>
#include <guacamole/protocol.h>
#include <guacamole/client.h>

#include "ssh_client.h"
#include "ssh_handlers.h"
#include "ssh_terminal.h"

#define SSH_TERM_STATE_NULL 0
#define SSH_TERM_STATE_ECHO 1
#define SSH_TERM_STATE_ESC  2
#define SSH_TERM_STATE_CSI  3
#define SSH_TERM_STATE_OSC  4
#define SSH_TERM_STATE_CHARSET 5

/* Client plugin arguments */
const char* GUAC_CLIENT_ARGS[] = {
    "hostname",
    "user",
    "password",
    NULL
};

int ssh_guac_client_password_key_handler(guac_client* client, int keysym, int pressed) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;

    /* If key pressed */
    if (pressed) {

        /* If simple ASCII key */
        if (keysym >= 0x00 && keysym <= 0xFF) {
            /* Add to password */
            client_data->password[client_data->password_length++] = keysym;
            ssh_guac_terminal_write(client_data->term, "*", 1);
            guac_flush(client->io);
        }
        else if (keysym == 0xFF08) {

            if (client_data->password_length > 0) {
                client_data->password_length--;

                /* Backspace */
                ssh_guac_terminal_write(client_data->term, "\x08\x1B[K", 4);
                guac_flush(client->io);
            }

        }
        else if (keysym == 0xFF0D) {

            /* Finish password */
            client_data->password[client_data->password_length] = '\0';

            /* Clear screen */
            ssh_guac_terminal_write(client_data->term, "\x1B[2J\x1B[1;1H", 10);
            guac_flush(client->io);

            return ssh_guac_client_auth(client, client_data->password);

        }

    }

    return 0;

}

int guac_client_init(guac_client* client, int argc, char** argv) {

    GUACIO* io = client->io;

    ssh_guac_client_data* client_data = malloc(sizeof(ssh_guac_client_data));
    ssh_guac_terminal* term = ssh_guac_terminal_create(client);

    /* Init client data */
    client->data = client_data;
    client_data->term = term;

    /* Send name and dimensions */
    guac_send_name(io, "SSH TEST");
    guac_send_size(io,
            term->char_width  * term->term_width,
            term->char_height * term->term_height);

    guac_flush(io);

    /* Open SSH session */
    client_data->session = ssh_new();
    if (client_data->session == NULL) {
        guac_send_error(io, "Unable to create SSH session.");
        guac_flush(io);
        return 1;
    }

    /* Set session options */
    ssh_options_set(client_data->session, SSH_OPTIONS_HOST, argv[0]);
    ssh_options_set(client_data->session, SSH_OPTIONS_USER, argv[1]);

    /* Connect */
    if (ssh_connect(client_data->session) != SSH_OK) {
        guac_send_error(io, "Unable to connect via SSH.");
        guac_flush(io);
        return 1;
    }

    /* If password provided, authenticate now */
    if (argv[2][0] != '\0')
        return ssh_guac_client_auth(client, argv[2]);

    /* Otherwise, prompt for password */
    else {
        
        client_data->password_length = 0;
        ssh_guac_terminal_write(client_data->term, "Password: ", 10);
        guac_flush(client->io);

        client->key_handler = ssh_guac_client_password_key_handler;

    }

    /* Success */
    return 0;

}

int ssh_guac_client_auth(guac_client* client, const char* password) {

    GUACIO* io = client->io;
    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    ssh_guac_terminal* term = client_data->term;

    /* Authenticate */
    if (ssh_userauth_password(client_data->session, NULL, password) != SSH_AUTH_SUCCESS) {
        guac_send_error(io, "SSH auth failed.");
        guac_flush(io);
        return 1;
    }

    /* Open channel for terminal */
    client_data->term_channel = channel_new(client_data->session);
    if (client_data->term_channel == NULL) {
        guac_send_error(io, "Unable to open channel.");
        guac_flush(io);
        return 1;
    }

    /* Open session for channel */
    if (channel_open_session(client_data->term_channel) != SSH_OK) {
        guac_send_error(io, "Unable to open channel session.");
        guac_flush(io);
        return 1;
    }

    /* Request PTY */
    if (channel_request_pty(client_data->term_channel) != SSH_OK) {
        guac_send_error(io, "Unable to allocate PTY for channel.");
        guac_flush(io);
        return 1;
    }

    /* Request PTY size */
    if (channel_change_pty_size(client_data->term_channel, term->term_width, term->term_height) != SSH_OK) {
        guac_send_error(io, "Unable to change PTY size.");
        guac_flush(io);
        return 1;
    }

    /* Request shell */
    if (channel_request_shell(client_data->term_channel) != SSH_OK) {
        guac_send_error(io, "Unable to associate shell with PTY.");
        guac_flush(io);
        return 1;
    }

    guac_log_info("SSH connection successful.");

    /* Set handlers */
    client->handle_messages = ssh_guac_client_handle_messages;
    client->key_handler = ssh_guac_client_key_handler;

    /* Success */
    return 0;

}



