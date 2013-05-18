
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

#include <sys/select.h>

#include <stdlib.h>
#include <string.h>

#include <cairo/cairo.h>
#include <pango/pangocairo.h>

#include <guacamole/socket.h>
#include <guacamole/protocol.h>
#include <guacamole/client.h>
#include <guacamole/error.h>

#include "guac_handlers.h"
#include "client.h"
#include "common.h"
#include "cursor.h"

int ssh_guac_client_handle_messages(guac_client* client) {

    guac_socket* socket = client->socket;
    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    char buffer[8192];

    int ret_val;
    int fd = client_data->stdout_pipe_fd[0];
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

        /* Lock terminal access */
        pthread_mutex_lock(&(client_data->term->lock));

        /* Read data, write to terminal */
        if ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {

            if (guac_terminal_write(client_data->term, buffer, bytes_read))
                return 1;

        }

        /* Notify on error */
        if (bytes_read < 0) {
            guac_protocol_send_error(socket, "Error reading data.");
            guac_socket_flush(socket);
            return 1;
        }

        /* Update cursor */
        guac_terminal_commit_cursor(client_data->term);

        /* Flush terminal display */
        guac_terminal_display_flush(client_data->term->display);

        /* Unlock terminal access */
        pthread_mutex_unlock(&(client_data->term->lock));

    }
    else if (ret_val < 0) {
        guac_error_message = "Error waiting for pipe";
        guac_error = GUAC_STATUS_SEE_ERRNO;
        return 1;
    }

    return 0;

}

int ssh_guac_client_clipboard_handler(guac_client* client, char* data) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;

    free(client_data->clipboard_data);

    client_data->clipboard_data = strdup(data);

    return 0;
}



int ssh_guac_client_mouse_handler(guac_client* client, int x, int y, int mask) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    guac_terminal* term = client_data->term;

    /* Determine which buttons were just released and pressed */
    int released_mask =  client_data->mouse_mask & ~mask;
    int pressed_mask  = ~client_data->mouse_mask &  mask;

    client_data->mouse_mask = mask;

    /* Show mouse cursor if not already shown */
    if (client_data->current_cursor != client_data->ibar_cursor) {
        pthread_mutex_lock(&(term->lock));

        client_data->current_cursor = client_data->ibar_cursor;
        guac_ssh_set_cursor(client, client_data->ibar_cursor);
        guac_socket_flush(client->socket);

        pthread_mutex_unlock(&(term->lock));
    }

    /* Paste contents of clipboard on right mouse button up */
    if ((released_mask & GUAC_CLIENT_MOUSE_RIGHT)
            && client_data->clipboard_data != NULL) {

        int length = strlen(client_data->clipboard_data);
        if (length)
            return write(client_data->stdin_pipe_fd[1],
                    client_data->clipboard_data, length);

    }

    /* If text selected, change state based on left mouse mouse button */
    if (term->text_selected) {
        pthread_mutex_lock(&(term->lock));

        /* If mouse button released, stop selection */
        if (released_mask & GUAC_CLIENT_MOUSE_LEFT) {

            /* End selection and get selected text */
            char* string = malloc(term->term_width * term->term_height * sizeof(char));
            guac_terminal_select_end(term, string);

            /* Store new data */
            free(client_data->clipboard_data);
            client_data->clipboard_data = string;

            /* Send data */
            guac_protocol_send_clipboard(client->socket, string);
            guac_socket_flush(client->socket);

        }

        /* Otherwise, just update */
        else
            guac_terminal_select_update(term,
                    y / term->display->char_height - term->scroll_offset,
                    x / term->display->char_width);

        pthread_mutex_unlock(&(term->lock));
    }

    /* Otherwise, if mouse button pressed AND moved, start selection */
    else if (!(pressed_mask & GUAC_CLIENT_MOUSE_LEFT) &&
               mask         & GUAC_CLIENT_MOUSE_LEFT) {
        pthread_mutex_lock(&(term->lock));

        guac_terminal_select_start(term,
                y / term->display->char_height - term->scroll_offset,
                x / term->display->char_width);

        pthread_mutex_unlock(&(term->lock));
    }

    /* Scroll up if wheel moved up */
    if (released_mask & GUAC_CLIENT_MOUSE_SCROLL_UP) {
        pthread_mutex_lock(&(term->lock));
        guac_terminal_scroll_display_up(term, GUAC_SSH_WHEEL_SCROLL_AMOUNT);
        pthread_mutex_unlock(&(term->lock));
    }

    /* Scroll down if wheel moved down */
    if (released_mask & GUAC_CLIENT_MOUSE_SCROLL_DOWN) {
        pthread_mutex_lock(&(term->lock));
        guac_terminal_scroll_display_down(term, GUAC_SSH_WHEEL_SCROLL_AMOUNT);
        pthread_mutex_unlock(&(term->lock));
    }

    return 0;

}

int ssh_guac_client_key_handler(guac_client* client, int keysym, int pressed) {

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    guac_terminal* term = client_data->term;

    /* Get write end of STDIN pipe */
    int fd = client_data->stdin_pipe_fd[1];

    /* Hide mouse cursor if not already hidden */
    if (client_data->current_cursor != client_data->blank_cursor) {
        pthread_mutex_lock(&(term->lock));

        client_data->current_cursor = client_data->blank_cursor;
        guac_ssh_set_cursor(client, client_data->blank_cursor);
        guac_socket_flush(client->socket);

        pthread_mutex_unlock(&(term->lock));
    }

    /* Track modifiers */
    if (keysym == 0xFFE3) {
        client_data->mod_ctrl = pressed;
    }
        
    /* If key pressed */
    else if (pressed) {

        /* Reset scroll */
        if (term->scroll_offset != 0) {
            pthread_mutex_lock(&(term->lock));
            guac_terminal_scroll_display_down(term, term->scroll_offset);
            pthread_mutex_unlock(&(term->lock));
        }

        /* Translate Ctrl+letter to control code */ 
        if (client_data->mod_ctrl) {

            char data;

            /* If valid control code, send it */
            if (keysym >= 'A' && keysym <= 'Z')
                data = (char) (keysym - 'A' + 1);
            else if (keysym >= 'a' && keysym <= 'z')
                data = (char) (keysym - 'a' + 1);

            /* Otherwise ignore */
            else
                return 0;

            return write(fd, &data, 1);

        }

        /* Translate Unicode to UTF-8 */
        else if ((keysym >= 0x00 && keysym <= 0xFF) || ((keysym & 0xFFFF0000) == 0x01000000)) {

            int length;
            char data[5];

            length = guac_terminal_encode_utf8(keysym & 0xFFFF, data);
            return write(fd, data, length);

        }

        else {

            int length = 0;
            const char* data = NULL;

                 if (keysym == 0xFF08) { data = "\x08"; length = 1; }
            else if (keysym == 0xFF09) { data = "\x09"; length = 1; }
            else if (keysym == 0xFF0D) { data = "\x0D"; length = 1; }
            else if (keysym == 0xFF1B) { data = "\x1B"; length = 1; }

            /* Arrow keys */
            else if (keysym == 0xFF52) { data = "\x1B\x5B""A"; length = 3; }
            else if (keysym == 0xFF54) { data = "\x1B\x5B""B"; length = 3; }
            else if (keysym == 0xFF53) { data = "\x1B\x5B""C"; length = 3; }
            else if (keysym == 0xFF51) { data = "\x1B\x5B""D"; length = 3; }

            /* Ignore other keys */
            else return 0;

            return write(fd, data, length);
        }

    }

    return 0;

}

int ssh_guac_client_size_handler(guac_client* client, int width, int height) {

    /* Get terminal */
    ssh_guac_client_data* guac_client_data = (ssh_guac_client_data*) client->data;
    guac_terminal* terminal = guac_client_data->term;

    /* Calculate dimensions */
    int rows    = height / terminal->display->char_height;
    int columns = width  / terminal->display->char_width;

    pthread_mutex_lock(&(terminal->lock));

    /* If size has changed */
    if (columns != terminal->term_width || rows != terminal->term_height) {

        /* Resize terminal */
        guac_terminal_resize(terminal, columns, rows);

        /* FIXME: Make resize call to SSH thread */

        /* Reset scroll region */
        terminal->scroll_end = rows - 1;

    }

    pthread_mutex_unlock(&(terminal->lock));

    return 0;
}

int ssh_guac_client_free_handler(guac_client* client) {

    ssh_guac_client_data* guac_client_data = (ssh_guac_client_data*) client->data;

    /* Close SSH client */
    close(STDOUT_FILENO);
    close(STDIN_FILENO);
    pthread_join(guac_client_data->client_thread, NULL);

    /* Free terminal */
    guac_terminal_free(guac_client_data->term);

    /* Free clipboard data */
    free(guac_client_data->clipboard_data);

    /* Free cursors */
    guac_ssh_cursor_free(client, guac_client_data->ibar_cursor);
    guac_ssh_cursor_free(client, guac_client_data->blank_cursor);

    /* Free generic data struct */
    free(client->data);

    return 0;
}

