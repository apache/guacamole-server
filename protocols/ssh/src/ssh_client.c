
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

#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>

#include "client.h"
#include "common.h"

/**
 * Reads a single line from STDIN.
 */
static char* prompt(guac_client* client, const char* title, char* str, int size, bool echo) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;

    int pos;
    char in_byte;

    /* Get STDIN and STDOUT */
    int stdin_fd  = client_data->term->stdin_pipe_fd[0];
    int stdout_fd = client_data->term->stdout_pipe_fd[1];

    /* Print title */
    guac_terminal_write_all(stdout_fd, title, strlen(title));

    /* Make room for null terminator */
    size--;

    /* Read bytes until newline */
    pos = 0;
    while (pos < size && read(stdin_fd, &in_byte, 1) == 1) {

        /* Backspace */
        if (in_byte == 0x08) {

            if (pos > 0) {
                guac_terminal_write_all(stdout_fd, "\b \b", 3);
                pos--;
            }
        }

        /* Newline (end of input */
        else if (in_byte == 0x0A) {
            guac_terminal_write_all(stdout_fd, "\r\n", 2);
            break;
        }

        else {

            /* Store character, update buffers */
            str[pos++] = in_byte;

            /* Print character if echoing */
            if (echo)
                guac_terminal_write_all(stdout_fd, &in_byte, 1);
            else
                guac_terminal_write_all(stdout_fd, "*", 1);

        }

    }

    str[pos] = 0;
    return str;

}

void* ssh_input_thread(void* data) {

    guac_client* client = (guac_client*) data;
    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;

    char buffer[8192];
    int bytes_read;

    int stdin_fd = client_data->term->stdin_pipe_fd[0];

    /* Write all data read */
    while ((bytes_read = read(stdin_fd, buffer, sizeof(buffer))) > 0)
        channel_write(client_data->term_channel, buffer, bytes_read);

    return NULL;

}

void* ssh_client_thread(void* data) {

    guac_client* client = (guac_client*) data;
    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;

    guac_socket* socket = client->socket;
    char buffer[8192];
    int bytes_read = -1234;

    int stdout_fd = client_data->term->stdout_pipe_fd[1];

    pthread_t input_thread;

    /* Get username */
    if (client_data->username[0] == 0 &&
            prompt(client, "Login as: ", client_data->username, sizeof(client_data->username), true) == NULL)
        return NULL;

    /* Get password */
    if (client_data->password[0] == 0 &&
            prompt(client, "Password: ", client_data->password, sizeof(client_data->password), false) == NULL)
        return NULL;

    /* Clear screen */
    guac_terminal_write_all(stdout_fd, "\x1B[H\x1B[J", 6);

    /* Open SSH session */
    client_data->session = ssh_new();
    if (client_data->session == NULL) {
        guac_protocol_send_error(socket, "Unable to create SSH session.");
        guac_socket_flush(socket);
        return NULL;
    }

    /* Set session options */
    ssh_options_set(client_data->session, SSH_OPTIONS_HOST, client_data->hostname);
    ssh_options_set(client_data->session, SSH_OPTIONS_USER, client_data->username);

    /* Connect */
    if (ssh_connect(client_data->session) != SSH_OK) {
        guac_protocol_send_error(socket, "Unable to connect via SSH.");
        guac_socket_flush(socket);
        return NULL;
    }

    /* Authenticate */
    if (ssh_userauth_password(client_data->session, NULL, client_data->password) != SSH_AUTH_SUCCESS) {
        guac_protocol_send_error(socket, "SSH auth failed.");
        guac_socket_flush(socket);
        return NULL;
    }

    /* Open channel for terminal */
    client_data->term_channel = channel_new(client_data->session);
    if (client_data->term_channel == NULL) {
        guac_protocol_send_error(socket, "Unable to open channel.");
        guac_socket_flush(socket);
        return NULL;
    }

    /* Open session for channel */
    if (channel_open_session(client_data->term_channel) != SSH_OK) {
        guac_protocol_send_error(socket, "Unable to open channel session.");
        guac_socket_flush(socket);
        return NULL;
    }

    /* Request PTY */
    if (channel_request_pty(client_data->term_channel) != SSH_OK) {
        guac_protocol_send_error(socket, "Unable to allocate PTY for channel.");
        guac_socket_flush(socket);
        return NULL;
    }

    /* Request PTY size */
    if (channel_change_pty_size(client_data->term_channel,
                client_data->term->term_width, client_data->term->term_height) != SSH_OK) {
        guac_protocol_send_error(socket, "Unable to change PTY size.");
        guac_socket_flush(socket);
        return NULL;
    }

    /* Request shell */
    if (channel_request_shell(client_data->term_channel) != SSH_OK) {
        guac_protocol_send_error(socket, "Unable to associate shell with PTY.");
        guac_socket_flush(socket);
        return NULL;
    }

    /* Logged in */
    guac_client_log_info(client, "SSH connection successful.");

    /* Start input thread */
    if (pthread_create(&(input_thread), NULL, ssh_input_thread, (void*) client)) {
        guac_client_log_error(client, "Unable to start SSH input thread");
        return NULL;
    }

    /* While data available, write to terminal */
    while (channel_is_open(client_data->term_channel)
            && !channel_is_eof(client_data->term_channel)) {

        /* Repeat read if necessary */
        if ((bytes_read = channel_read(client_data->term_channel, buffer, sizeof(buffer), 0)) == SSH_AGAIN)
            continue;

        if (bytes_read > 0)
            guac_terminal_write_all(stdout_fd, buffer, bytes_read);

    }

    /* Notify on error */
    if (bytes_read < 0) {
        guac_protocol_send_error(socket, "Error reading data.");
        guac_socket_flush(socket);
        return NULL;
    }

    /* Wait for input thread to die */
    pthread_join(input_thread, NULL);

    guac_client_log_info(client, "SSH connection ended.");
    return NULL;

}

