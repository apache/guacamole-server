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
#include "common/cursor.h"
#include "terminal/buffer.h"
#include "terminal/common.h"
#include "terminal/display.h"
#include "terminal/palette.h"
#include "terminal/terminal.h"
#include "terminal/terminal_handlers.h"
#include "terminal/types.h"
#include "terminal/typescript.h"
#include "terminal/xparsecolor.h"

#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <wchar.h>

#include <guacamole/client.h>
#include <guacamole/error.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/timestamp.h>

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

