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


#include <sys/select.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdlib.h>
#include <string.h>

#include <cairo/cairo.h>
#include <pango/pangocairo.h>

#include <guacamole/socket.h>
#include <guacamole/protocol.h>
#include <guacamole/client.h>
#include <guacamole/error.h>

#include <libssh2.h>

#include "guac_handlers.h"
#include "client.h"
#include "common.h"
#include "cursor.h"

int ssh_guac_client_handle_messages(guac_client* client) {

    guac_socket* socket = client->socket;
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

        /* Lock terminal access */
        pthread_mutex_lock(&(client_data->term->lock));

        /* Read data, write to terminal */
        if ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {

            if (guac_terminal_write(client_data->term, buffer, bytes_read))
                return 1;

        }

        /* Notify on error */
        if (bytes_read < 0) {
            guac_protocol_send_error(socket, "Error reading data.",
                GUAC_PROTOCOL_STATUS_INTERNAL_ERROR);
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

    /* Paste contents of clipboard on right or middle mouse button up */
    if ((released_mask & GUAC_CLIENT_MOUSE_RIGHT) || (released_mask & GUAC_CLIENT_MOUSE_MIDDLE)) {
        if (client_data->clipboard_data != NULL)
            return guac_terminal_send_string(term, client_data->clipboard_data);
        else
            return 0;
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

    /* Hide mouse cursor if not already hidden */
    if (client_data->current_cursor != client_data->blank_cursor) {
        pthread_mutex_lock(&(term->lock));

        client_data->current_cursor = client_data->blank_cursor;
        guac_ssh_set_cursor(client, client_data->blank_cursor);
        guac_socket_flush(client->socket);

        pthread_mutex_unlock(&(term->lock));
    }

    /* Track modifiers */
    if (keysym == 0xFFE3)
        client_data->mod_ctrl = pressed;
    else if (keysym == 0xFFE9)
        client_data->mod_alt = pressed;
    else if (keysym == 0xFFE1)
        client_data->mod_shift = pressed;
        
    /* If key pressed */
    else if (pressed) {

        /* Ctrl+Shift+V shortcut for paste */
        if (keysym == 'V' && client_data->mod_ctrl) {
            if (client_data->clipboard_data != NULL)
                return guac_terminal_send_string(term, client_data->clipboard_data);
            else
                return 0;
        }

        /* Shift+PgUp / Shift+PgDown shortcuts for scrolling */
        if (client_data->mod_shift) {

            /* Page up */
            if (keysym == 0xFF55) {
                pthread_mutex_lock(&(term->lock));
                guac_terminal_scroll_display_up(term, term->term_height);
                pthread_mutex_unlock(&(term->lock));
                return 0;
            }

            /* Page down */
            if (keysym == 0xFF56) {
                pthread_mutex_lock(&(term->lock));
                guac_terminal_scroll_display_down(term, term->term_height);
                pthread_mutex_unlock(&(term->lock));
                return 0;
            }

        }

        /* Reset scroll */
        if (term->scroll_offset != 0) {
            pthread_mutex_lock(&(term->lock));
            guac_terminal_scroll_display_down(term, term->scroll_offset);
            pthread_mutex_unlock(&(term->lock));
        }

        /* If alt being held, also send escape character */
        if (client_data->mod_alt)
            return guac_terminal_send_string(term, "\x1B");

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

            return guac_terminal_send_data(term, &data, 1);

        }

        /* Translate Unicode to UTF-8 */
        else if ((keysym >= 0x00 && keysym <= 0xFF) || ((keysym & 0xFFFF0000) == 0x01000000)) {

            int length;
            char data[5];

            length = guac_terminal_encode_utf8(keysym & 0xFFFF, data);
            return guac_terminal_send_data(term, data, length);

        }

        /* Non-printable keys */
        else {

            if (keysym == 0xFF08) return guac_terminal_send_string(term, "\x7F"); /* Backspace */
            if (keysym == 0xFF09) return guac_terminal_send_string(term, "\x09"); /* Tab */
            if (keysym == 0xFF0D) return guac_terminal_send_string(term, "\x0D"); /* Enter */
            if (keysym == 0xFF1B) return guac_terminal_send_string(term, "\x1B"); /* Esc */

            if (keysym == 0xFF50) return guac_terminal_send_string(term, "\x1B[1~"); /* Home */

            /* Arrow keys w/ application cursor */
            if (term->application_cursor_keys) {
                if (keysym == 0xFF51) return guac_terminal_send_string(term, "\x1BOD"); /* Left */
                if (keysym == 0xFF52) return guac_terminal_send_string(term, "\x1BOA"); /* Up */
                if (keysym == 0xFF53) return guac_terminal_send_string(term, "\x1BOC"); /* Right */
                if (keysym == 0xFF54) return guac_terminal_send_string(term, "\x1BOB"); /* Down */
            }
            else {
                if (keysym == 0xFF51) return guac_terminal_send_string(term, "\x1B[D"); /* Left */
                if (keysym == 0xFF52) return guac_terminal_send_string(term, "\x1B[A"); /* Up */
                if (keysym == 0xFF53) return guac_terminal_send_string(term, "\x1B[C"); /* Right */
                if (keysym == 0xFF54) return guac_terminal_send_string(term, "\x1B[B"); /* Down */
            }

            if (keysym == 0xFF55) return guac_terminal_send_string(term, "\x1B[5~"); /* Page up */
            if (keysym == 0xFF56) return guac_terminal_send_string(term, "\x1B[6~"); /* Page down */
            if (keysym == 0xFF57) return guac_terminal_send_string(term, "\x1B[4~"); /* End */

            if (keysym == 0xFF63) return guac_terminal_send_string(term, "\x1B[2~"); /* Insert */

            if (keysym == 0xFFBE) return guac_terminal_send_string(term, "\x1B[[A"); /* F1  */
            if (keysym == 0xFFBF) return guac_terminal_send_string(term, "\x1B[[B"); /* F2  */
            if (keysym == 0xFFC0) return guac_terminal_send_string(term, "\x1B[[C"); /* F3  */
            if (keysym == 0xFFC1) return guac_terminal_send_string(term, "\x1B[[D"); /* F4  */
            if (keysym == 0xFFC2) return guac_terminal_send_string(term, "\x1B[[E"); /* F5  */

            if (keysym == 0xFFC3) return guac_terminal_send_string(term, "\x1B[17~"); /* F6  */
            if (keysym == 0xFFC4) return guac_terminal_send_string(term, "\x1B[18~"); /* F7  */
            if (keysym == 0xFFC5) return guac_terminal_send_string(term, "\x1B[19~"); /* F8  */
            if (keysym == 0xFFC6) return guac_terminal_send_string(term, "\x1B[20~"); /* F9  */
            if (keysym == 0xFFC7) return guac_terminal_send_string(term, "\x1B[21~"); /* F10 */
            if (keysym == 0xFFC8) return guac_terminal_send_string(term, "\x1B[22~"); /* F11 */
            if (keysym == 0xFFC9) return guac_terminal_send_string(term, "\x1B[23~"); /* F12 */

            if (keysym == 0xFFFF) return guac_terminal_send_string(term, "\x1B[3~"); /* Delete */

            /* Ignore unknown keys */
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

        /* Update SSH pty size if connected */
        if (guac_client_data->term_channel != NULL)
            libssh2_channel_request_pty_size(guac_client_data->term_channel,
                    terminal->term_width, terminal->term_height);

        /* Reset scroll region */
        terminal->scroll_end = rows - 1;

        guac_terminal_display_flush(terminal->display);
        guac_protocol_send_sync(terminal->client->socket,
                client->last_sent_timestamp);
        guac_socket_flush(terminal->client->socket);
    }

    pthread_mutex_unlock(&(terminal->lock));

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

    /* Free clipboard data */
    free(guac_client_data->clipboard_data);

    /* Free cursors */
    guac_ssh_cursor_free(client, guac_client_data->ibar_cursor);
    guac_ssh_cursor_free(client, guac_client_data->blank_cursor);

    /* Free generic data struct */
    free(client->data);

    return 0;
}

