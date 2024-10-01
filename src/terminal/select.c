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

#include "common/clipboard.h"
#include "terminal/buffer.h"
#include "terminal/display.h"
#include "terminal/select.h"
#include "terminal/selection-point.h"
#include "terminal/terminal.h"
#include "terminal/terminal-priv.h"
#include "terminal/types.h"

#include <guacamole/client.h>
#include <guacamole/socket.h>
#include <guacamole/unicode.h>

#include <stdbool.h>

void guac_terminal_select_redraw(guac_terminal* terminal) {

    /* Update the selected region of the display if text is currently
     * selected */
    if (terminal->text_selected) {

        int start_row = terminal->selection_start_row + terminal->scroll_offset;
        int start_column = terminal->selection_start_column;

        int end_row = terminal->selection_end_row + terminal->scroll_offset;
        int end_column = terminal->selection_end_column;

        guac_terminal_display_select(terminal->display, start_row,
                start_column, end_row, end_column, terminal->rectangle_selection);

    }

    /* Clear the display selection if no text is currently selected */
    else
        guac_terminal_display_clear_select(terminal->display);

}

/**
 * Locates the beginning of the character at the given row and column, updating
 * the column to the starting column of that character. The width, if available,
 * is returned. If the character has no defined width, 1 is returned.
 *
 * @param terminal
 *     The guac_terminal in which the character should be located.
 *
 * @param row
 *     The row number of the desired character, where the first (top-most) row
 *     in the terminal is row 0. Rows within the scrollback buffer (above the
 *     top-most row of the terminal) will be negative.
 *
 * @param column
 *     A pointer to an int containing the column number of the desired
 *     character, where 0 is the first (left-most) column within the row. If
 *     the character is a multi-column character, the value of this int will be
 *     adjusted as necessary such that it contains the column number of the
 *     first column containing the character.
 *
 * @return
 *     The width of the specified character, in columns, or 1 if the character
 *     has no defined width.
 */
static int guac_terminal_find_char(guac_terminal* terminal,
        int row, int* column) {

    guac_terminal_char* characters;
    int start_column = *column;

    int length = guac_terminal_buffer_get_columns(terminal->current_buffer, &characters, NULL, row);
    if (start_column >= 0 && start_column < length) {

        /* Find beginning of character */
        guac_terminal_char* start_char = &(characters[start_column]);
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

/**
 * Setup a selection point at the given row, column, and side. Making
 * sure to calculate the start and width of the character pointed to.
 *
 * @param terminal
 *     The guac_terminal in which the character should be located.
 *
 * @param row
 *     The row number of the desired point
 *
 * @param column
 *     The column number of the desired point
 *
 * @param side
 *     The column side of the desired point
 */
static void guac_terminal_selection_point_init(guac_terminal_selection_point *point,
    guac_terminal* terminal, int row, int column, guac_terminal_column_side side) {

    point->row                  = row;
    point->column               = column;
    point->side                 = side;
    point->char_width           = guac_terminal_find_char(terminal, row, &column);
    point->char_starting_column = column;

}

void guac_terminal_select_start(guac_terminal* terminal,
        int row, int column, guac_terminal_column_side side) {

    /* Selection start and end beging at same point */
    guac_terminal_selection_point_init(&terminal->selection_start,
        terminal, row, column, side);
    guac_terminal_selection_point_init(&terminal->selection_end,
        terminal, row, column, side);

    terminal->text_selected = false;
    terminal->selection_committed = false;
    guac_terminal_notify(terminal);

}

void guac_terminal_select_update(guac_terminal* terminal,
        int row, int column, guac_terminal_column_side side) {
    
    /* Only update if end point has changed */
    if (row != terminal->selection_end.row
        || column != terminal->selection_end.column
        || side != terminal->selection_end.side) {

        /* Update the end point */
        guac_terminal_selection_point_init(&terminal->selection_end,
            terminal, row, column, side);

        /* Normalize so that start point comes first when reading left to right */
        const guac_terminal_selection_point *start = &terminal->selection_start;
        const guac_terminal_selection_point *end = &terminal->selection_end;
        if (guac_terminal_selection_point_is_after(start, end)) {
            end = &terminal->selection_start;
            start = &terminal->selection_end;
        }

        if (guac_terminal_selection_points_enclose_text(start, end)) {
            int new_start_column = guac_terminal_selection_point_round_up(start);
            int new_end_column = guac_terminal_selection_point_round_down(end);

            if (terminal->rectangle_selection && new_start_column > new_end_column) {
                new_start_column = guac_terminal_selection_point_round_up(end);
                new_end_column = guac_terminal_selection_point_round_down(start);
            }

            /* Only notify terminal if actual selection has changed */
            if (terminal->selection_start_row != start->row ||
                terminal->selection_start_column != new_start_column ||
                terminal->selection_end_row != end->row ||
                terminal->selection_end_column != new_end_column ||
                !terminal->text_selected) {

                terminal->selection_start_row = start->row;
                terminal->selection_start_column = new_start_column;
                terminal->selection_end_row = end->row;
                terminal->selection_end_column = new_end_column;
                terminal->text_selected = true;
                guac_terminal_notify(terminal);
            }
        }

        /* If no text is selected, then clear the selection */
        else {

            terminal->text_selected = false;
            guac_terminal_notify(terminal);

        }

    }

}

void guac_terminal_select_resume(guac_terminal* terminal,
        int row, int column, guac_terminal_column_side side) {

    /* No need to test coordinates if no text is selected at all */
    if (!terminal->text_selected)
        return;

    guac_terminal_selection_point point;
    guac_terminal_selection_point_init(&point, terminal, row, column, side);

    /* Update the start point if it's after this resume point */
    if (guac_terminal_selection_point_is_after(&terminal->selection_start, &point)) {
        terminal->selection_start = point;

        /* Preserve the existing end point */
        row = terminal->selection_end.row;
        column = terminal->selection_end.column;
        side = terminal->selection_end.side;

        /* Clobbering the row here will make sure guac_terminal_select_update actually runs */
        terminal->selection_end.row = -1;
    }

    /* Selection is again in-progress */
    terminal->selection_committed = false;

    /* Update selection to contain given character */
    guac_terminal_select_update(terminal, row, column, side);

}

/**
 * Appends the text within the given array of terminal characters to the
 * clipboard. The provided coordinates are considered inclusively (the
 * characters at the start and end column are included in the copied text). Any
 * out-of-bounds coordinates will be automatically clipped within the bounds of
 * the given array.
 *
 * @param terminal
 *     The guac_terminal instance associated with the buffer containing the
 *     text being copied and the clipboard receiving the copied text.
 *
 * @param characters
 *     The array of characters copied into the clipboard.
 *
 * @param length
 *     The number of characters in the provided character array.
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
static void guac_terminal_clipboard_append_characters(guac_terminal* terminal,
        guac_terminal_char* characters, unsigned int length, int start, int end) {

    char buffer[1024];
    int eol;

    /* If selection is entirely outside the bounds of the row, then there is
     * nothing to append */
    if (start < 0 || end < 0 || start >= length)
        return;

    /* Ensure desired end column is within bounds */
    if (end >= length)
        end = length - 1;

    /* Get position of last not null char */
    for (eol = end; eol > start; eol--) {
        if (characters[eol].value != 0)
            break;
    }

    /* Repeatedly convert chunks of terminal buffer rows until entire specified
     * region has been appended to clipboard */
    for (int i = start; i <= end;) {

        int remaining = sizeof(buffer);
        char* current = buffer;

        /* Convert as many codepoints within the given range as possible */
        for (; i <= end; i++) {

            int codepoint = characters[i].value;

            /* Fill empty with spaces if not at end of line */
            if (codepoint == 0 && i < eol)
                codepoint = GUAC_CHAR_SPACE;

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

    /* If no text is selected, nothing to do */
    if (!terminal->text_selected)
        return;

    /* Selection is now committed */
    terminal->selection_committed = true;

    /* Reset current clipboard contents */
    guac_common_clipboard_reset(terminal->clipboard, "text/plain");

    int start_row = terminal->selection_start_row;
    int start_col = terminal->selection_start_column;
    int end_row = terminal->selection_end_row;
    int end_col = terminal->selection_end_column;

    guac_terminal_char* characters;
    bool last_row_was_wrapped = true;

    for (int row = start_row; row <= end_row; row++) {

        /* Add a newline only if the previous line was not wrapped */
        if (!last_row_was_wrapped || (terminal->rectangle_selection && row != start_row))
            guac_common_clipboard_append(terminal->clipboard, "\n", 1);

        /* Append next row from desired region, adjusting the start/end column
         * to account for selections that start or end in the middle of a row.
         * With the exception of the start and end rows, all other rows are
         * copied in their entirety. */
        int length = guac_terminal_buffer_get_columns(terminal->current_buffer, &characters, &last_row_was_wrapped, row);
        guac_terminal_clipboard_append_characters(terminal, characters, length,
            (row == start_row || terminal->rectangle_selection) ? start_col : 0,
            (row == end_row   || terminal->rectangle_selection) ? end_col   : length - 1);

    }

    /* Broadcast copied data to all connected users only if allowed */
    if (!terminal->disable_copy) {
        guac_common_clipboard_send(terminal->clipboard, client);
        guac_socket_flush(socket);
    }

    guac_terminal_notify(terminal);

}

bool guac_terminal_select_contains(guac_terminal* terminal,
        int start_row, int start_column, int end_row, int end_column) {

    /* No need to test coordinates if no text is selected at all */
    if (!terminal->text_selected)
        return false;

    /* If test range starts after highlight ends, does not intersect */
    if (start_row > terminal->selection_end_row)
        return false;

    if (start_row == terminal->selection_end_row && start_column > terminal->selection_end_column)
        return false;

    /* If test range ends before highlight starts, does not intersect */
    if (end_row < terminal->selection_start_row)
        return false;

    if (end_row == terminal->selection_start_row && end_column < terminal->selection_start_column)
        return false;

    /* Otherwise, does intersect */
    return true;

}

void guac_terminal_select_touch(guac_terminal* terminal,
        int start_row, int start_column, int end_row, int end_column) {

    /* Only clear selection if selection is committed */
    if (!terminal->selection_committed)
        return;

    /* Clear selection if it contains any characters within the given region */
    if (guac_terminal_select_contains(terminal, start_row, start_column,
                end_row, end_column)) {

        /* Text is no longer selected */
        terminal->text_selected = false;
        terminal->selection_committed = false;
        guac_terminal_notify(terminal);

    }

}

