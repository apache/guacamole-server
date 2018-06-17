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

#include "common/clipboard.h"
#include "terminal/buffer.h"
#include "terminal/display.h"
#include "terminal/select.h"
#include "terminal/terminal.h"
#include "terminal/types.h"

#include <guacamole/client.h>
#include <guacamole/socket.h>
#include <guacamole/unicode.h>

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

/**
 * Appends the text within the given subsection of a terminal row to the
 * clipboard. The provided coordinates are considered inclusiveley (the
 * characters at the start and end column are included in the copied
 * text). Any out-of-bounds coordinates will be automatically clipped within
 * the bounds of the given row.
 *
 * @param terminal
 *     The guac_terminal instance associated with the buffer containing the
 *     text being copied and the clipboard receiving the copied text.
 *
 * @param row
 *     The row number of the text within the terminal to be copied into the
 *     clipboard, where the first (top-most) row in the terminal is row 0. Rows
 *     within the scrollback buffer (above the top-most row of the terminal)
 *     will be negative.
 *
 * @param start
 *     The first column of the text to be copied from the given row into the
 *     clipboard associated with the given terminal, where 0 is the first
 *     (left-most) column within the row.
 *
 * @param end
 *     The last column of the text to be copied from the given row into the
 *     clipboard associated with the given terminal, where 0 is the first
 *     (left-most) column within the row.
 */
static void guac_terminal_clipboard_append_row(guac_terminal* terminal,
        int row, int start, int end) {

    char buffer[1024];
    int i = start;

    guac_terminal_buffer_row* buffer_row =
        guac_terminal_buffer_get_row(terminal->buffer, row, 0);

    /* If selection is entirely outside the bounds of the row, then there is
     * nothing to append */
    if (start > buffer_row->length - 1)
        return;

    /* Clip given range to actual bounds of row */
    if (end == -1 || end > buffer_row->length - 1)
        end = buffer_row->length - 1;

    /* Repeatedly convert chunks of terminal buffer rows until entire specified
     * region has been appended to clipboard */
    while (i <= end) {

        int remaining = sizeof(buffer);
        char* current = buffer;

        /* Convert as many codepoints within the given range as possible */
        for (i = start; i <= end; i++) {

            int codepoint = buffer_row->characters[i].value;

            /* Ignore null (blank) characters */
            if (codepoint == 0 || codepoint == GUAC_CHAR_CONTINUATION)
                continue;

            /* Encode current codepoint as UTF-8 */
            int bytes = guac_utf8_write(codepoint, current, remaining);
            if (bytes == 0)
                break;

            current += bytes;
            remaining -= bytes;

        }

        /* Append converted buffer to clipboard */
        guac_common_clipboard_append(terminal->clipboard, buffer, current - buffer);

    }

}

void guac_terminal_select_end(guac_terminal* terminal) {

    guac_client* client = terminal->client;
    guac_socket* socket = client->socket;

    /* Reset current clipboard contents */
    guac_common_clipboard_reset(terminal->clipboard, "text/plain");

    /* Deselect */
    terminal->text_selected = false;
    guac_terminal_display_commit_select(terminal->display);

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
    if (end_row == start_row)
        guac_terminal_clipboard_append_row(terminal, start_row, start_col, end_col);

    /* Otherwise, copy multiple rows */
    else {

        /* Store first row */
        guac_terminal_clipboard_append_row(terminal, start_row, start_col, -1);

        /* Store all middle rows */
        for (int row = start_row + 1; row < end_row; row++) {
            guac_common_clipboard_append(terminal->clipboard, "\n", 1);
            guac_terminal_clipboard_append_row(terminal, row, 0, -1);
        }

        /* Store last row */
        guac_common_clipboard_append(terminal->clipboard, "\n", 1);
        guac_terminal_clipboard_append_row(terminal, end_row, 0, end_col);

    }

    /* Send data */
    guac_common_clipboard_send(terminal->clipboard, client);
    guac_socket_flush(socket);

}

