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
#include "clipboard.h"
#include "common.h"
#include "cursor.h"
#include "guac_handlers.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/stat.h>

#include <cairo/cairo.h>
#include <guacamole/socket.h>
#include <guacamole/protocol.h>
#include <guacamole/client.h>
#include <libssh2.h>
#include <pango/pangocairo.h>

int ssh_guac_client_handle_messages(guac_client* client) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    char buffer[8192];

    int ret_val;
    int fd = client_data->term->stdout_pipe_fd[0];
    struct timeval timeout;
    fd_set fds;

    /* Build fd_set */
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    /* Time to wait */
    timeout.tv_sec =  1;
    timeout.tv_usec = 0;

    /* Wait for data to be available */
    ret_val = select(fd+1, &fds, NULL, NULL, &timeout);
    if (ret_val > 0) {

        int bytes_read = 0;

        guac_terminal_lock(client_data->term);

        /* Read data, write to terminal */
        if ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {

            if (guac_terminal_write(client_data->term, buffer, bytes_read)) {
                guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR, "Error writing data");
                return 1;
            }

        }

        /* Notify on error */
        if (bytes_read < 0) {
            guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR, "Error reading data");
            return 1;
        }

        /* Flush terminal */
        guac_terminal_flush(client_data->term);
        guac_terminal_unlock(client_data->term);

    }
    else if (ret_val < 0) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR, "Error waiting for data");
        return 1;
    }

    return 0;

}

int ssh_guac_client_mouse_handler(guac_client* client, int x, int y, int mask) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    guac_terminal* term = client_data->term;

    /* Send mouse event */
    guac_terminal_lock(term);
    guac_terminal_send_mouse(term, x, y, mask);
    guac_terminal_unlock(term);

    return 0;

}

int ssh_guac_client_key_handler(guac_client* client, int keysym, int pressed) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    guac_terminal* term = client_data->term;

    /* Send key */
    guac_terminal_lock(term);
    guac_terminal_send_key(term, keysym, pressed);
    guac_terminal_unlock(term);

    return 0;

}

int ssh_guac_client_size_handler(guac_client* client, int width, int height) {

    /* Get terminal */
    ssh_guac_client_data* guac_client_data = (ssh_guac_client_data*) client->data;
    guac_terminal* terminal = guac_client_data->term;

    /* Resize terminal */
    guac_terminal_lock(terminal);
    guac_terminal_resize(terminal, width, height);
    guac_terminal_unlock(terminal);

    /* Update SSH pty size if connected */
    if (guac_client_data->term_channel != NULL)
        libssh2_channel_request_pty_size(guac_client_data->term_channel,
                terminal->term_width, terminal->term_height);

    return 0;
}

int ssh_guac_client_free_handler(guac_client* client) {

    ssh_guac_client_data* guac_client_data = (ssh_guac_client_data*) client->data;

    /* Close SSH channel */
    if (guac_client_data->term_channel != NULL) {
        libssh2_channel_send_eof(guac_client_data->term_channel);
        libssh2_channel_close(guac_client_data->term_channel);
    }

    /* Free terminal */
    guac_terminal_free(guac_client_data->term);
    pthread_join(guac_client_data->client_thread, NULL);

    /* Free channels */
    libssh2_channel_free(guac_client_data->term_channel);

    /* Clean up SFTP */
    if (guac_client_data->sftp_session)
        libssh2_sftp_shutdown(guac_client_data->sftp_session);

    if (guac_client_data->sftp_ssh_session) {
        libssh2_session_disconnect(guac_client_data->sftp_ssh_session, "Bye");
        libssh2_session_free(guac_client_data->sftp_ssh_session);
    }

    /* Free session */
    if (guac_client_data->session != NULL)
        libssh2_session_free(guac_client_data->session);

    /* Free auth key */
    if (guac_client_data->key != NULL)
        ssh_key_free(guac_client_data->key);

    /* Free generic data struct */
    free(client->data);

    return 0;
}

