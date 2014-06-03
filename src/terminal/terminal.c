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

#include "buffer.h"
#include "blank.h"
#include "common.h"
#include "cursor.h"
#include "display.h"
#include "ibar.h"
#include "guac_clipboard.h"
#include "terminal.h"
#include "terminal_handlers.h"
#include "types.h"

#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <wchar.h>

#include <guacamole/client.h>
#include <guacamole/error.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>

/**
 * Sets the given range of columns to the given character.
 */
static void __guac_terminal_set_columns(guac_terminal* terminal, int row,
        int start_column, int end_column, guac_terminal_char* character) {

    guac_terminal_display_set_columns(terminal->display, row + terminal->scroll_offset,
            start_column, end_column, character);

    guac_terminal_buffer_set_columns(terminal->buffer, row,
            start_column, end_column, character);

}

/**
 * Enforces a character break at the given edge, ensuring that the left side
 * of the edge is the final column of a character, and the right side of the
 * edge is the initial column of a DIFFERENT character.
 *
 * For a character in a column N, the left edge number is N, and the right
 * edge is N+1.
 */
static void __guac_terminal_force_break(guac_terminal* terminal, int row, int edge) {

    guac_terminal_buffer_row* buffer_row = guac_terminal_buffer_get_row(terminal->buffer, row, 0);

    /* Ensure character to left of edge is unbroken */
    if (edge > 0) {

        int end_column = edge - 1;
        int start_column = end_column;

        guac_terminal_char* start_char = &(buffer_row->characters[start_column]);

        /* Determine start column */
        while (start_column > 0 && start_char->value == GUAC_CHAR_CONTINUATION) {
            start_char--;
            start_column--;
        }

        /* Advance to start of broken character if necessary */
        if (start_char->value != GUAC_CHAR_CONTINUATION && start_char->width < end_column - start_column + 1) {
            start_column += start_char->width;
            start_char += start_char->width;
        }

        /* Clear character if broken */
        if (start_char->value == GUAC_CHAR_CONTINUATION || start_char->width != end_column - start_column + 1) {

            guac_terminal_char cleared_char;
            cleared_char.value = ' ';
            cleared_char.attributes = start_char->attributes;
            cleared_char.width = 1;

            __guac_terminal_set_columns(terminal, row, start_column, end_column, &cleared_char);

        }

    }

    /* Ensure character to right of edge is unbroken */
    if (edge >= 0 && edge < buffer_row->length) {

        int start_column = edge;
        int end_column = start_column;

        guac_terminal_char* start_char = &(buffer_row->characters[start_column]);
        guac_terminal_char* end_char = &(buffer_row->characters[end_column]);

        /* Determine end column */
        while (end_column+1 < buffer_row->length && (end_char+1)->value == GUAC_CHAR_CONTINUATION) {
            end_char++;
            end_column++;
        }

        /* Advance to start of broken character if necessary */
        if (start_char->value != GUAC_CHAR_CONTINUATION && start_char->width < end_column - start_column + 1) {
            start_column += start_char->width;
            start_char += start_char->width;
        }

        /* Clear character if broken */
        if (start_char->value == GUAC_CHAR_CONTINUATION || start_char->width != end_column - start_column + 1) {

            guac_terminal_char cleared_char;
            cleared_char.value = ' ';
            cleared_char.attributes = start_char->attributes;
            cleared_char.width = 1;

            __guac_terminal_set_columns(terminal, row, start_column, end_column, &cleared_char);

        }

    }

}

void guac_terminal_reset(guac_terminal* term) {

    int row;

    /* Set current state */
    term->char_handler = guac_terminal_echo; 
    term->active_char_set = 0;
    term->char_mapping[0] =
    term->char_mapping[1] = NULL;

    /* Reset cursor location */
    term->cursor_row = term->visible_cursor_row = term->saved_cursor_row = 0;
    term->cursor_col = term->visible_cursor_col = term->saved_cursor_col = 0;

    /* Clear scrollback, buffer, and scoll region */
    term->buffer->top = 0;
    term->buffer->length = 0;
    term->scroll_start = 0;
    term->scroll_end = term->term_height - 1;
    term->scroll_offset = 0;

    /* Reset flags */
    term->text_selected = false;
    term->application_cursor_keys = false;
    term->automatic_carriage_return = false;
    term->insert_mode = false;

    /* Reset tabs */
    term->tab_interval = 8;
    memset(term->custom_tabs, 0, sizeof(term->custom_tabs));

    /* Clear terminal */
    for (row=0; row<term->term_height; row++)
        guac_terminal_set_columns(term, row, 0, term->term_width, &(term->default_char));

}

guac_terminal* guac_terminal_create(guac_client* client,
        const char* font_name, int font_size, int dpi,
        int width, int height) {

    guac_terminal_char default_char = {
        .value = 0,
        .attributes = {
            .foreground = 7,
            .background = 0,
            .bold       = false,
            .reverse    = false,
            .underscore = false
        },
        .width = 1};

    guac_terminal* term = malloc(sizeof(guac_terminal));
    term->client = client;
    term->upload_path_handler = NULL;
    term->file_download_handler = NULL;

    /* Init buffer */
    term->buffer = guac_terminal_buffer_alloc(1000, &default_char);

    /* Init display */
    term->display = guac_terminal_display_alloc(client,
            font_name, font_size, dpi,
            default_char.attributes.foreground,
            default_char.attributes.background);

    /* Fail if display init failed */
    if (term->display == NULL) {
        guac_client_log(client, GUAC_LOG_DEBUG, "Display initialization failed");
        free(term);
        return NULL;
    }

    /* Init terminal state */
    term->current_attributes = default_char.attributes;
    term->default_char = default_char;

    term->term_width   = width  / term->display->char_width;
    term->term_height  = height / term->display->char_height;

    /* Open STDOUT pipe */
    if (pipe(term->stdout_pipe_fd)) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Unable to open pipe for STDOUT";
        free(term);
        return NULL;
    }

    /* Open STDIN pipe */
    if (pipe(term->stdin_pipe_fd)) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Unable to open pipe for STDIN";
        free(term);
        return NULL;
    }

    /* Init terminal lock */
    pthread_mutex_init(&(term->lock), NULL);

    /* Size display */
    guac_protocol_send_size(term->display->client->socket,
            GUAC_DEFAULT_LAYER, width, height);
    guac_terminal_display_resize(term->display,
            term->term_width, term->term_height);

    /* Init terminal */
    guac_terminal_reset(term);

    term->mod_alt   =
    term->mod_ctrl  =
    term->mod_shift = 0;

    /* Set up mouse cursors */
    term->ibar_cursor = guac_terminal_create_ibar(client);
    term->blank_cursor = guac_terminal_create_blank(client);

    /* Initialize mouse cursor */
    term->current_cursor = term->blank_cursor;
    guac_terminal_set_cursor(term->client, term->current_cursor);

    /* Allocate clipboard */
    term->clipboard = guac_common_clipboard_alloc(GUAC_TERMINAL_CLIPBOARD_MAX_LENGTH);

    return term;

}

void guac_terminal_free(guac_terminal* term) {
    
    /* Close terminal output pipe */
    close(term->stdout_pipe_fd[1]);
    close(term->stdout_pipe_fd[0]);

    /* Close user input pipe */
    close(term->stdin_pipe_fd[1]);
    close(term->stdin_pipe_fd[0]);

    /* Free display */
    guac_terminal_display_free(term->display);

    /* Free buffer */
    guac_terminal_buffer_free(term->buffer);

    /* Free clipboard */
    guac_common_clipboard_free(term->clipboard);

    /* Free cursors */
    guac_terminal_cursor_free(term->client, term->ibar_cursor);
    guac_terminal_cursor_free(term->client, term->blank_cursor);

}

int guac_terminal_render_frame(guac_terminal* terminal) {

    guac_client* client = terminal->client;
    char buffer[8192];

    int ret_val;
    int fd = terminal->stdout_pipe_fd[0];
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

        guac_terminal_lock(terminal);

        /* Read data, write to terminal */
        if ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {

            if (guac_terminal_write(terminal, buffer, bytes_read)) {
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
        guac_terminal_flush(terminal);
        guac_terminal_unlock(terminal);

    }
    else if (ret_val < 0) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR, "Error waiting for data");
        return 1;
    }

    return 0;

}

int guac_terminal_read_stdin(guac_terminal* terminal, char* c, int size) {
    int stdin_fd = terminal->stdin_pipe_fd[0];
    return read(stdin_fd, c, size);
}

int guac_terminal_write_stdout(guac_terminal* terminal, const char* c, int size) {
    return guac_terminal_write_all(terminal->stdout_pipe_fd[1], c, size);
}

int guac_terminal_printf(guac_terminal* terminal, const char* format, ...) {

    int written;

    va_list ap;
    char buffer[1024];

    /* Print to buffer */
    va_start(ap, format);
    written = vsnprintf(buffer, sizeof(buffer)-1, format, ap);
    va_end(ap);

    if (written < 0)
        return written;

    /* Write to STDOUT */
    return guac_terminal_write_stdout(terminal, buffer, written);

}

void guac_terminal_prompt(guac_terminal* terminal, const char* title, char* str, int size, bool echo) {

    int pos;
    char in_byte;

    /* Print title */
    guac_terminal_printf(terminal, "%s", title);

    /* Make room for null terminator */
    size--;

    /* Read bytes until newline */
    pos = 0;
    while (pos < size && guac_terminal_read_stdin(terminal, &in_byte, 1) == 1) {

        /* Backspace */
        if (in_byte == 0x7F) {

            if (pos > 0) {
                guac_terminal_printf(terminal, "\b \b");
                pos--;
            }
        }

        /* CR (end of input */
        else if (in_byte == 0x0D) {
            guac_terminal_printf(terminal, "\r\n");
            break;
        }

        else {

            /* Store character, update buffers */
            str[pos++] = in_byte;

            /* Print character if echoing */
            if (echo)
                guac_terminal_printf(terminal, "%c", in_byte);
            else
                guac_terminal_printf(terminal, "*");

        }

    }

    /* Terminate string */
    str[pos] = 0;

}

int guac_terminal_set(guac_terminal* term, int row, int col, int codepoint) {

    int width;

    /* Build character with current attributes */
    guac_terminal_char guac_char;
    guac_char.value = codepoint;
    guac_char.attributes = term->current_attributes;

    width = wcwidth(codepoint);
    if (width < 0)
        width = 1;

    guac_char.width = width;

    guac_terminal_set_columns(term, row, col, col + width - 1, &guac_char);

    return 0;

}

void guac_terminal_commit_cursor(guac_terminal* term) {

    guac_terminal_char* guac_char;

    guac_terminal_buffer_row* old_row;
    guac_terminal_buffer_row* new_row;

    /* If no change, done */
    if (term->visible_cursor_row == term->cursor_row && term->visible_cursor_col == term->cursor_col)
        return;

    /* Get old and new rows with cursor */
    new_row = guac_terminal_buffer_get_row(term->buffer, term->cursor_row, term->cursor_col+1);
    old_row = guac_terminal_buffer_get_row(term->buffer, term->visible_cursor_row, term->visible_cursor_col+1);

    /* Clear cursor */
    guac_char = &(old_row->characters[term->visible_cursor_col]);
    guac_char->attributes.cursor = false;
    guac_terminal_display_set_columns(term->display, term->visible_cursor_row + term->scroll_offset,
            term->visible_cursor_col, term->visible_cursor_col, guac_char);

    /* Set cursor */
    guac_char = &(new_row->characters[term->cursor_col]);
    guac_char->attributes.cursor = true;
    guac_terminal_display_set_columns(term->display, term->cursor_row + term->scroll_offset,
            term->cursor_col, term->cursor_col, guac_char);

    term->visible_cursor_row = term->cursor_row;
    term->visible_cursor_col = term->cursor_col;

    return;

}

int guac_terminal_write(guac_terminal* term, const char* c, int size) {

    while (size > 0) {
        term->char_handler(term, *(c++));
        size--;
    }

    return 0;

}

int guac_terminal_scroll_up(guac_terminal* term,
        int start_row, int end_row, int amount) {

    /* If scrolling entire display, update scroll offset */
    if (start_row == 0 && end_row == term->term_height - 1) {

        /* Scroll up visibly */
        guac_terminal_display_copy_rows(term->display, start_row + amount, end_row, -amount);

        /* Advance by scroll amount */
        term->buffer->top += amount;
        if (term->buffer->top >= term->buffer->available)
            term->buffer->top -= term->buffer->available;

        term->buffer->length += amount;
        if (term->buffer->length > term->buffer->available)
            term->buffer->length = term->buffer->available;

        /* Update cursor location if within region */
        if (term->visible_cursor_row >= start_row &&
            term->visible_cursor_row <= end_row)
            term->visible_cursor_row -= amount;

    }

    /* Otherwise, just copy row data upwards */
    else
        guac_terminal_copy_rows(term, start_row + amount, end_row, -amount);

    /* Clear new area */
    guac_terminal_clear_range(term,
            end_row - amount + 1, 0,
            end_row, term->term_width - 1);

    return 0;
}

int guac_terminal_scroll_down(guac_terminal* term,
        int start_row, int end_row, int amount) {

    guac_terminal_copy_rows(term, start_row, end_row - amount, amount);

    /* Clear new area */
    guac_terminal_clear_range(term,
            start_row, 0,
            start_row + amount - 1, term->term_width - 1);

    return 0;
}

int guac_terminal_clear_columns(guac_terminal* term,
        int row, int start_col, int end_col) {

    /* Build space */
    guac_terminal_char blank;
    blank.value = 0;
    blank.attributes = term->current_attributes;
    blank.width = 1;

    /* Clear */
    guac_terminal_set_columns(term,
        row, start_col, end_col, &blank);

    return 0;

}

int guac_terminal_clear_range(guac_terminal* term,
        int start_row, int start_col,
        int end_row, int end_col) {

    /* If not at far left, must clear sub-region to far right */
    if (start_col > 0) {

        /* Clear from start_col to far right */
        guac_terminal_clear_columns(term,
            start_row, start_col, term->term_width - 1);

        /* One less row to clear */
        start_row++;
    }

    /* If not at far right, must clear sub-region to far left */
    if (end_col < term->term_width - 1) {

        /* Clear from far left to end_col */
        guac_terminal_clear_columns(term, end_row, 0, end_col);

        /* One less row to clear */
        end_row--;

    }

    /* Remaining region now guaranteed rectangular. Clear, if possible */
    if (start_row <= end_row) {

        int row;
        for (row=start_row; row<=end_row; row++) {

            /* Clear entire row */
            guac_terminal_clear_columns(term, row, 0, term->term_width - 1);

        }

    }

    return 0;

}

void guac_terminal_scroll_display_down(guac_terminal* terminal,
        int scroll_amount) {

    int start_row, end_row;
    int dest_row;
    int row, column;

    /* Limit scroll amount by size of scrollback buffer */
    if (scroll_amount > terminal->scroll_offset)
        scroll_amount = terminal->scroll_offset;

    /* If not scrolling at all, don't bother trying */
    if (scroll_amount <= 0)
        return;

    /* Shift screen up */
    if (terminal->term_height > scroll_amount)
        guac_terminal_display_copy_rows(terminal->display,
                scroll_amount, terminal->term_height - 1,
                -scroll_amount);

    /* Advance by scroll amount */
    terminal->scroll_offset -= scroll_amount;

    /* Get row range */
    end_row   = terminal->term_height - terminal->scroll_offset - 1;
    start_row = end_row - scroll_amount + 1;
    dest_row  = terminal->term_height - scroll_amount;

    /* Draw new rows from scrollback */
    for (row=start_row; row<=end_row; row++) {

        /* Get row from scrollback */
        guac_terminal_buffer_row* buffer_row =
            guac_terminal_buffer_get_row(terminal->buffer, row, 0);

        /* Clear row */
        guac_terminal_display_set_columns(terminal->display,
                dest_row, 0, terminal->display->width, &(terminal->default_char));

        /* Draw row */
        guac_terminal_char* current = buffer_row->characters;
        for (column=0; column<buffer_row->length; column++) {

            /* Only draw if not blank */
            if (guac_terminal_has_glyph(current->value))
                guac_terminal_display_set_columns(terminal->display, dest_row, column, column, current);

            current++;

        }

        /* Next row */
        dest_row++;

    }

    guac_terminal_display_flush(terminal->display);
    guac_protocol_send_sync(terminal->client->socket,
            terminal->client->last_sent_timestamp);
    guac_socket_flush(terminal->client->socket);

}

void guac_terminal_scroll_display_up(guac_terminal* terminal,
        int scroll_amount) {

    int start_row, end_row;
    int dest_row;
    int row, column;

    /* Limit scroll amount by size of scrollback buffer */
    if (terminal->scroll_offset + scroll_amount > terminal->buffer->length - terminal->term_height)
        scroll_amount = terminal->buffer->length - terminal->scroll_offset - terminal->term_height;

    /* If not scrolling at all, don't bother trying */
    if (scroll_amount <= 0)
        return;

    /* Shift screen down */
    if (terminal->term_height > scroll_amount)
        guac_terminal_display_copy_rows(terminal->display,
                0, terminal->term_height - scroll_amount - 1,
                scroll_amount);

    /* Advance by scroll amount */
    terminal->scroll_offset += scroll_amount;

    /* Get row range */
    start_row = -terminal->scroll_offset;
    end_row   = start_row + scroll_amount - 1;
    dest_row  = 0;

    /* Draw new rows from scrollback */
    for (row=start_row; row<=end_row; row++) {

        /* Get row from scrollback */
        guac_terminal_buffer_row* buffer_row = 
            guac_terminal_buffer_get_row(terminal->buffer, row, 0);

        /* Clear row */
        guac_terminal_display_set_columns(terminal->display,
                dest_row, 0, terminal->display->width, &(terminal->default_char));

        /* Draw row */
        guac_terminal_char* current = buffer_row->characters;
        for (column=0; column<buffer_row->length; column++) {

            /* Only draw if not blank */
            if (guac_terminal_has_glyph(current->value))
                guac_terminal_display_set_columns(terminal->display, dest_row, column, column, current);

            current++;

        }

        /* Next row */
        dest_row++;

    }

    guac_terminal_display_flush(terminal->display);
    guac_protocol_send_sync(terminal->client->socket,
            terminal->client->last_sent_timestamp);
    guac_socket_flush(terminal->client->socket);

}

void guac_terminal_select_redraw(guac_terminal* terminal) {

    int start_row = terminal->selection_start_row + terminal->scroll_offset;
    int start_column = terminal->selection_start_column;

    int end_row = terminal->selection_end_row + terminal->scroll_offset;
    int end_column = terminal->selection_end_column;

    /* Update start/end columns to include character width */
    if (start_row > end_row || (start_row == end_row && start_column > end_column))
        start_column += terminal->selection_start_width - 1;
    else
        end_column += terminal->selection_end_width - 1;

    guac_terminal_display_select(terminal->display, start_row, start_column, end_row, end_column);

}

/**
 * Locates the beginning of the character at the given row and column, updating
 * the column to the starting column of that character. The width, if available,
 * is returned. If the character has no defined width, 1 is returned.
 */
static int __guac_terminal_find_char(guac_terminal* terminal, int row, int* column) {

    int start_column = *column;

    guac_terminal_buffer_row* buffer_row = guac_terminal_buffer_get_row(terminal->buffer, row, 0);
    if (start_column < buffer_row->length) {

        /* Find beginning of character */
        guac_terminal_char* start_char = &(buffer_row->characters[start_column]);
        while (start_column > 0 && start_char->value == GUAC_CHAR_CONTINUATION) {
            start_char--;
            start_column--;
        }

        /* Use width, if available */
        if (start_char->value != GUAC_CHAR_CONTINUATION) {
            *column = start_column;
            return start_char->width;
        }

    }

    /* Default to one column wide */
    return 1;

}

void guac_terminal_select_start(guac_terminal* terminal, int row, int column) {

    int width = __guac_terminal_find_char(terminal, row, &column);

    terminal->selection_start_row = 
    terminal->selection_end_row   = row;

    terminal->selection_start_column = 
    terminal->selection_end_column   = column;

    terminal->selection_start_width = 
    terminal->selection_end_width   = width;

    terminal->text_selected = true;

    guac_terminal_select_redraw(terminal);

}

void guac_terminal_select_update(guac_terminal* terminal, int row, int column) {

    /* Only update if selection has changed */
    if (row != terminal->selection_end_row
        || column <  terminal->selection_end_column
        || column >= terminal->selection_end_column + terminal->selection_end_width) {

        int width = __guac_terminal_find_char(terminal, row, &column);

        terminal->selection_end_row = row;
        terminal->selection_end_column = column;
        terminal->selection_end_width = width;

        guac_terminal_select_redraw(terminal);
    }

}

int __guac_terminal_buffer_string(guac_terminal_buffer_row* row, int start, int end, char* string) {

    int length = 0;
    int i;
    for (i=start; i<=end; i++) {

        int codepoint = row->characters[i].value;

        /* If not null (blank), add to string */
        if (codepoint != 0 && codepoint != GUAC_CHAR_CONTINUATION) {
            int bytes = guac_terminal_encode_utf8(codepoint, string);
            string += bytes;
            length += bytes;
        }

    }

    return length;

}

void guac_terminal_select_end(guac_terminal* terminal, char* string) {

    /* Deselect */
    terminal->text_selected = false;
    guac_terminal_display_commit_select(terminal->display);

    guac_terminal_buffer_row* buffer_row;

    int row;

    int start_row, start_col;
    int end_row, end_col;

    /* Ensure proper ordering of start and end coords */
    if (terminal->selection_start_row < terminal->selection_end_row
        || (terminal->selection_start_row == terminal->selection_end_row
            && terminal->selection_start_column < terminal->selection_end_column)) {

        start_row = terminal->selection_start_row;
        start_col = terminal->selection_start_column;
        end_row   = terminal->selection_end_row;
        end_col   = terminal->selection_end_column + terminal->selection_end_width - 1;

    }
    else {
        end_row   = terminal->selection_start_row;
        end_col   = terminal->selection_start_column + terminal->selection_start_width - 1;
        start_row = terminal->selection_end_row;
        start_col = terminal->selection_end_column;
    }

    /* If only one row, simply copy */
    buffer_row = guac_terminal_buffer_get_row(terminal->buffer, start_row, 0);
    if (end_row == start_row) {
        if (buffer_row->length - 1 < end_col)
            end_col = buffer_row->length - 1;
        string += __guac_terminal_buffer_string(buffer_row, start_col, end_col, string);
    }

    /* Otherwise, copy multiple rows */
    else {

        /* Store first row */
        string += __guac_terminal_buffer_string(buffer_row, start_col, buffer_row->length - 1, string);

        /* Store all middle rows */
        for (row=start_row+1; row<end_row; row++) {

            buffer_row = guac_terminal_buffer_get_row(terminal->buffer, row, 0);

            *(string++) = '\n';
            string += __guac_terminal_buffer_string(buffer_row, 0, buffer_row->length - 1, string);

        }

        /* Store last row */
        buffer_row = guac_terminal_buffer_get_row(terminal->buffer, end_row, 0);
        if (buffer_row->length - 1 < end_col)
            end_col = buffer_row->length - 1;

        *(string++) = '\n';
        string += __guac_terminal_buffer_string(buffer_row, 0, end_col, string);

    }

    /* Null terminator */
    *string = 0;

}

void guac_terminal_copy_columns(guac_terminal* terminal, int row,
        int start_column, int end_column, int offset) {

    guac_terminal_display_copy_columns(terminal->display, row + terminal->scroll_offset,
            start_column, end_column, offset);

    guac_terminal_buffer_copy_columns(terminal->buffer, row,
            start_column, end_column, offset);

    /* Update cursor location if within region */
    if (row == terminal->visible_cursor_row &&
            terminal->visible_cursor_col >= start_column &&
            terminal->visible_cursor_col <= end_column)
        terminal->visible_cursor_col += offset;

    /* Force breaks around destination region */
    __guac_terminal_force_break(terminal, row, start_column + offset);
    __guac_terminal_force_break(terminal, row, end_column + offset + 1);

}

void guac_terminal_copy_rows(guac_terminal* terminal,
        int start_row, int end_row, int offset) {

    guac_terminal_display_copy_rows(terminal->display,
            start_row + terminal->scroll_offset, end_row + terminal->scroll_offset, offset);

    guac_terminal_buffer_copy_rows(terminal->buffer,
            start_row, end_row, offset);

    /* Update cursor location if within region */
    if (terminal->visible_cursor_row >= start_row &&
        terminal->visible_cursor_row <= end_row)
        terminal->visible_cursor_row += offset;

}

void guac_terminal_set_columns(guac_terminal* terminal, int row,
        int start_column, int end_column, guac_terminal_char* character) {

    __guac_terminal_set_columns(terminal, row, start_column, end_column, character);

    /* If visible cursor in current row, preserve state */
    if (row == terminal->visible_cursor_row
            && terminal->visible_cursor_col >= start_column
            && terminal->visible_cursor_col <= end_column) {

        /* Create copy of character with cursor attribute set */
        guac_terminal_char cursor_character = *character;
        cursor_character.attributes.cursor = true;

        __guac_terminal_set_columns(terminal, row,
                terminal->visible_cursor_col, terminal->visible_cursor_col, &cursor_character);

    }

    /* Force breaks around destination region */
    __guac_terminal_force_break(terminal, row, start_column);
    __guac_terminal_force_break(terminal, row, end_column + 1);

}

static void __guac_terminal_redraw_rect(guac_terminal* term, int start_row, int start_col, int end_row, int end_col) {

    int row, col;

    /* Redraw region */
    for (row=start_row; row<=end_row; row++) {

        guac_terminal_buffer_row* buffer_row =
            guac_terminal_buffer_get_row(term->buffer, row - term->scroll_offset, 0);

        /* Clear row */
        guac_terminal_display_set_columns(term->display,
                row, start_col, end_col, &(term->default_char));

        /* Copy characters */
        for (col=start_col; col <= end_col && col < buffer_row->length; col++) {

            /* Only redraw if not blank */
            guac_terminal_char* c = &(buffer_row->characters[col]);
            if (guac_terminal_has_glyph(c->value))
                guac_terminal_display_set_columns(term->display, row, col, col, c);

        }

    }

}

/**
 * Internal terminal resize routine. Accepts width/height in CHARACTERS
 * (not pixels like the public function).
 */
static void __guac_terminal_resize(guac_terminal* term, int width, int height) {

    /* If height is decreasing, shift display up */
    if (height < term->term_height) {

        int shift_amount;

        /* Get number of rows actually occupying terminal space */
        int used_height = term->buffer->length;
        if (used_height > term->term_height)
            used_height = term->term_height;

        shift_amount = used_height - height;

        /* If the new terminal bottom covers N rows, shift up N rows */
        if (shift_amount > 0) {

            guac_terminal_display_copy_rows(term->display,
                    shift_amount, term->display->height - 1, -shift_amount);

            /* Update buffer top and cursor row based on shift */
            term->buffer->top += shift_amount;
            term->cursor_row  -= shift_amount;
            term->visible_cursor_row  -= shift_amount;

            /* Redraw characters within old region */
            __guac_terminal_redraw_rect(term, height - shift_amount, 0, height-1, width-1);

        }

    }

    /* Resize display */
    guac_terminal_display_flush(term->display);
    guac_terminal_display_resize(term->display, width, height);

    /* Reraw any characters on right if widening */
    if (width > term->term_width)
        __guac_terminal_redraw_rect(term, 0, term->term_width-1, height-1, width-1);

    /* If height is increasing, shift display down */
    if (height > term->term_height) {

        /* If undisplayed rows exist in the buffer, shift them into view */
        if (term->term_height < term->buffer->length) {

            /* If the new terminal bottom reveals N rows, shift down N rows */
            int shift_amount = height - term->term_height;

            /* The maximum amount we can shift is the number of undisplayed rows */
            int max_shift = term->buffer->length - term->term_height;

            if (shift_amount > max_shift)
                shift_amount = max_shift;

            /* Update buffer top and cursor row based on shift */
            term->buffer->top -= shift_amount;
            term->cursor_row  += shift_amount;
            term->visible_cursor_row  += shift_amount;

            /* If scrolled enough, use scroll to fulfill entire resize */
            if (term->scroll_offset >= shift_amount) {

                term->scroll_offset -= shift_amount;

                /* Draw characters from scroll at bottom */
                __guac_terminal_redraw_rect(term, term->term_height, 0, term->term_height + shift_amount - 1, width-1);

            }

            /* Otherwise, fulfill with as much scroll as possible */
            else {

                /* Draw characters from scroll at bottom */
                __guac_terminal_redraw_rect(term, term->term_height, 0, term->term_height + term->scroll_offset - 1, width-1);

                /* Update shift_amount and scroll based on new rows */
                shift_amount -= term->scroll_offset;
                term->scroll_offset = 0;

                /* If anything remains, move screen as necessary */
                if (shift_amount > 0) {

                    guac_terminal_display_copy_rows(term->display,
                            0, term->display->height - shift_amount - 1, shift_amount);

                    /* Draw characters at top from scroll */
                    __guac_terminal_redraw_rect(term, 0, 0, shift_amount - 1, width-1);

                }

            }

        } /* end if undisplayed rows exist */

    }

    /* Keep cursor on screen */
    if (term->cursor_row < 0)       term->cursor_row = 0;
    if (term->cursor_row >= height) term->cursor_row = height-1;
    if (term->cursor_col < 0)       term->cursor_col = 0;
    if (term->cursor_col >= width)  term->cursor_col = width-1;

    /* Commit new dimensions */
    term->term_width = width;
    term->term_height = height;

}

int guac_terminal_resize(guac_terminal* terminal, int width, int height) {

    guac_terminal_display* display = terminal->display;
    guac_client* client = display->client;
    guac_socket* socket = client->socket;

    /* Calculate dimensions */
    int rows    = height / display->char_height;
    int columns = width  / display->char_width;

    /* Resize default layer to given pixel dimensions */
    guac_protocol_send_size(socket, GUAC_DEFAULT_LAYER, width, height);

    /* Resize terminal if row/column dimensions have changed */
    if (columns != terminal->term_width || rows != terminal->term_height) {

        guac_client_log(client, GUAC_LOG_DEBUG,
                "Resizing terminal to %ix%i", rows, columns);

        /* Resize terminal */
        __guac_terminal_resize(terminal, columns, rows);

        /* Reset scroll region */
        terminal->scroll_end = rows - 1;

        guac_terminal_flush(terminal);
    }

    /* If terminal size hasn't changed, still need to flush */
    else {
        guac_protocol_send_sync(socket, client->last_sent_timestamp);
        guac_socket_flush(socket);
    }

    return 0;

}

void guac_terminal_flush(guac_terminal* terminal) {
    guac_terminal_commit_cursor(terminal);
    guac_terminal_display_flush(terminal->display);
}

void guac_terminal_lock(guac_terminal* terminal) {
    pthread_mutex_lock(&(terminal->lock));
}

void guac_terminal_unlock(guac_terminal* terminal) {
    pthread_mutex_unlock(&(terminal->lock));
}

int guac_terminal_send_data(guac_terminal* term, const char* data, int length) {
    return guac_terminal_write_all(term->stdin_pipe_fd[1], data, length);
}

int guac_terminal_send_string(guac_terminal* term, const char* data) {
    return guac_terminal_write_all(term->stdin_pipe_fd[1], data, strlen(data));
}

static int __guac_terminal_send_key(guac_terminal* term, int keysym, int pressed) {

    /* Hide mouse cursor if not already hidden */
    if (term->current_cursor != term->blank_cursor) {
        term->current_cursor = term->blank_cursor;
        guac_terminal_set_cursor(term->client, term->blank_cursor);
        guac_socket_flush(term->client->socket);
    }

    /* Track modifiers */
    if (keysym == 0xFFE3)
        term->mod_ctrl = pressed;
    else if (keysym == 0xFFE9)
        term->mod_alt = pressed;
    else if (keysym == 0xFFE1)
        term->mod_shift = pressed;
        
    /* If key pressed */
    else if (pressed) {

        /* Ctrl+Shift+V shortcut for paste */
        if (keysym == 'V' && term->mod_ctrl)
            return guac_terminal_send_data(term, term->clipboard->buffer, term->clipboard->length);

        /* Shift+PgUp / Shift+PgDown shortcuts for scrolling */
        if (term->mod_shift) {

            /* Page up */
            if (keysym == 0xFF55) {
                guac_terminal_scroll_display_up(term, term->term_height);
                return 0;
            }

            /* Page down */
            if (keysym == 0xFF56) {
                guac_terminal_scroll_display_down(term, term->term_height);
                return 0;
            }

        }

        /* Reset scroll */
        if (term->scroll_offset != 0)
            guac_terminal_scroll_display_down(term, term->scroll_offset);

        /* If alt being held, also send escape character */
        if (term->mod_alt)
            return guac_terminal_send_string(term, "\x1B");

        /* Translate Ctrl+letter to control code */ 
        if (term->mod_ctrl) {

            char data;

            /* Keysyms for '@' through '_' are all conveniently in C0 order */
            if (keysym >= '@' && keysym <= '_')
                data = (char) (keysym - '@');

            /* Handle lowercase as well */
            else if (keysym >= 'a' && keysym <= 'z')
                data = (char) (keysym - 'a' + 1);

            /* Ctrl+? is DEL (0x7f) */
            else if (keysym == '?')
                data = 0x7F;

            /* Map Ctrl+2 to same result as Ctrl+@ */
            else if (keysym == '2')
                data = 0x00;

            /* Map Ctrl+3 through Ctrl-7 to the remaining C0 characters such that Ctrl+6 is the same as Ctrl+^ */
            else if (keysym >= '3' && keysym <= '7')
                data = (char) (keysym - '3' + 0x1B);

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
            guac_client_log(term->client, GUAC_LOG_DEBUG,
                    "Ignoring unknown keysym: 0x%X", keysym);
        }

    }

    return 0;

}

int guac_terminal_send_key(guac_terminal* term, int keysym, int pressed) {

    int result;

    guac_terminal_lock(term);
    result = __guac_terminal_send_key(term, keysym, pressed);
    guac_terminal_unlock(term);

    return result;

}

static int __guac_terminal_send_mouse(guac_terminal* term, int x, int y, int mask) {

    /* Determine which buttons were just released and pressed */
    int released_mask =  term->mouse_mask & ~mask;
    int pressed_mask  = ~term->mouse_mask &  mask;

    term->mouse_mask = mask;

    /* Show mouse cursor if not already shown */
    if (term->current_cursor != term->ibar_cursor) {
        term->current_cursor = term->ibar_cursor;
        guac_terminal_set_cursor(term->client, term->ibar_cursor);
        guac_socket_flush(term->client->socket);
    }

    /* Paste contents of clipboard on right or middle mouse button up */
    if ((released_mask & GUAC_CLIENT_MOUSE_RIGHT) || (released_mask & GUAC_CLIENT_MOUSE_MIDDLE))
        return guac_terminal_send_data(term, term->clipboard->buffer, term->clipboard->length);

    /* If text selected, change state based on left mouse mouse button */
    if (term->text_selected) {

        /* If mouse button released, stop selection */
        if (released_mask & GUAC_CLIENT_MOUSE_LEFT) {

            int selected_length;

            /* End selection and get selected text */
            int selectable_size = term->term_width * term->term_height * sizeof(char);
            char* string = malloc(selectable_size);
            guac_terminal_select_end(term, string);

            selected_length = strnlen(string, selectable_size);

            /* Store new data */
            guac_common_clipboard_reset(term->clipboard, "text/plain");
            guac_common_clipboard_append(term->clipboard, string, selected_length);
            free(string);

            /* Send data */
            guac_common_clipboard_send(term->clipboard, term->client);
            guac_socket_flush(term->client->socket);

        }

        /* Otherwise, just update */
        else
            guac_terminal_select_update(term,
                    y / term->display->char_height - term->scroll_offset,
                    x / term->display->char_width);

    }

    /* Otherwise, if mouse button pressed AND moved, start selection */
    else if (!(pressed_mask & GUAC_CLIENT_MOUSE_LEFT) &&
               mask         & GUAC_CLIENT_MOUSE_LEFT)
        guac_terminal_select_start(term,
                y / term->display->char_height - term->scroll_offset,
                x / term->display->char_width);

    /* Scroll up if wheel moved up */
    if (released_mask & GUAC_CLIENT_MOUSE_SCROLL_UP)
        guac_terminal_scroll_display_up(term, GUAC_TERMINAL_WHEEL_SCROLL_AMOUNT);

    /* Scroll down if wheel moved down */
    if (released_mask & GUAC_CLIENT_MOUSE_SCROLL_DOWN)
        guac_terminal_scroll_display_down(term, GUAC_TERMINAL_WHEEL_SCROLL_AMOUNT);

    return 0;

}

int guac_terminal_send_mouse(guac_terminal* term, int x, int y, int mask) {

    int result;

    guac_terminal_lock(term);
    result = __guac_terminal_send_mouse(term, x, y, mask);
    guac_terminal_unlock(term);

    return result;

}

void guac_terminal_clipboard_reset(guac_terminal* term, const char* mimetype) {
    guac_common_clipboard_reset(term->clipboard, mimetype);
}

void guac_terminal_clipboard_append(guac_terminal* term, const void* data, int length) {
    guac_common_clipboard_append(term->clipboard, data, length);
}

int guac_terminal_sendf(guac_terminal* term, const char* format, ...) {

    int written;

    va_list ap;
    char buffer[1024];

    /* Print to buffer */
    va_start(ap, format);
    written = vsnprintf(buffer, sizeof(buffer)-1, format, ap);
    va_end(ap);

    if (written < 0)
        return written;

    /* Write to STDIN */
    return guac_terminal_write_all(term->stdin_pipe_fd[1], buffer, written);

}

void guac_terminal_set_tab(guac_terminal* term, int column) {

    int i;

    /* Search for available space, set if available */
    for (i=0; i<GUAC_TERMINAL_MAX_TABS; i++) {

        /* Set tab if space free */
        if (term->custom_tabs[i] == 0) {
            term->custom_tabs[i] = column+1;
            break;
        }

    }

}

void guac_terminal_unset_tab(guac_terminal* term, int column) {

    int i;

    /* Search for given tab, unset if found */
    for (i=0; i<GUAC_TERMINAL_MAX_TABS; i++) {

        /* Unset tab if found */
        if (term->custom_tabs[i] == column+1) {
            term->custom_tabs[i] = 0;
            break;
        }

    }

}

void guac_terminal_clear_tabs(guac_terminal* term) {
    term->tab_interval = 0;
    memset(term->custom_tabs, 0, sizeof(term->custom_tabs));
}

int guac_terminal_next_tab(guac_terminal* term, int column) {

    int i;

    /* Determine tab stop from interval */
    int tabstop;
    if (term->tab_interval != 0)
        tabstop = (column / term->tab_interval + 1) * term->tab_interval;
    else
        tabstop = term->term_width - 1;

    /* Walk custom tabs, trying to find an earlier occurrence */
    for (i=0; i<GUAC_TERMINAL_MAX_TABS; i++) {

        int custom_tabstop = term->custom_tabs[i] - 1;
        if (custom_tabstop != -1 && custom_tabstop > column && custom_tabstop < tabstop)
            tabstop = custom_tabstop;

    }

    return tabstop;
}

