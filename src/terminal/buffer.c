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
#include "common.h"

#include <stdlib.h>
#include <string.h>

guac_terminal_buffer* guac_terminal_buffer_alloc(int rows, guac_terminal_char* default_character) {

    /* Allocate scrollback */
    guac_terminal_buffer* buffer =
        malloc(sizeof(guac_terminal_buffer));

    int i;
    guac_terminal_buffer_row* row;

    /* Init scrollback data */
    buffer->default_character = *default_character;
    buffer->available = rows;
    buffer->top = 0;
    buffer->length = 0;
    buffer->rows = malloc(sizeof(guac_terminal_buffer_row) *
            buffer->available);

    /* Init scrollback rows */
    row = buffer->rows;
    for (i=0; i<rows; i++) {

        /* Allocate row  */
        row->available = 256;
        row->length = 0;
        row->characters = malloc(sizeof(guac_terminal_char) * row->available);

        /* Next row */
        row++;

    }

    return buffer;

}

void guac_terminal_buffer_free(guac_terminal_buffer* buffer) {

    int i;
    guac_terminal_buffer_row* row = buffer->rows;

    /* Free all rows */
    for (i=0; i<buffer->available; i++) {
        free(row->characters);
        row++;
    }

    /* Free actual buffer */
    free(buffer->rows);
    free(buffer);

}

guac_terminal_buffer_row* guac_terminal_buffer_get_row(guac_terminal_buffer* buffer, int row, int width) {

    int i;
    guac_terminal_char* first;
    guac_terminal_buffer_row* buffer_row;

    /* Calculate scrollback row index */
    int index = buffer->top + row;
    if (index < 0)
        index += buffer->available;
    else if (index >= buffer->available)
        index -= buffer->available;

    /* Get row */
    buffer_row = &(buffer->rows[index]);

    /* If resizing is needed */
    if (width >= buffer_row->length) {

        /* Expand if necessary */
        if (width > buffer_row->available) {
            buffer_row->available = width*2;
            buffer_row->characters = realloc(buffer_row->characters, sizeof(guac_terminal_char) * buffer_row->available);
        }

        /* Initialize new part of row */
        first = &(buffer_row->characters[buffer_row->length]);
        for (i=buffer_row->length; i<width; i++)
            *(first++) = buffer->default_character;

        buffer_row->length = width;

    }

    /* Return found row */
    return buffer_row;

}

void guac_terminal_buffer_copy_columns(guac_terminal_buffer* buffer, int row,
        int start_column, int end_column, int offset) {

    guac_terminal_char* src;
    guac_terminal_char* dst;

    /* Get row */
    guac_terminal_buffer_row* buffer_row = guac_terminal_buffer_get_row(buffer, row, end_column + offset + 1);

    /* Fit range within bounds */
    start_column = guac_terminal_fit_to_range(start_column,          0, buffer_row->length - 1);
    end_column   = guac_terminal_fit_to_range(end_column,            0, buffer_row->length - 1);
    start_column = guac_terminal_fit_to_range(start_column + offset, 0, buffer_row->length - 1) - offset;
    end_column   = guac_terminal_fit_to_range(end_column   + offset, 0, buffer_row->length - 1) - offset;

    /* Determine source and destination locations */
    src = &(buffer_row->characters[start_column]);
    dst = &(buffer_row->characters[start_column + offset]);

    /* Copy data */
    memmove(dst, src, sizeof(guac_terminal_char) * (end_column - start_column + 1));

}

void guac_terminal_buffer_copy_rows(guac_terminal_buffer* buffer,
        int start_row, int end_row, int offset) {

    int i, current_row;
    int step;

    /* If shifting down, copy in reverse */
    if (offset > 0) {
        current_row = end_row;
        step = -1;
    }

    /* Otherwise, copy forwards */
    else {
        current_row = start_row;
        step = 1;
    }

    /* Copy each current_row individually */
    for (i = start_row; i <= end_row; i++) {

        /* Get source and destination rows */
        guac_terminal_buffer_row* src_row = guac_terminal_buffer_get_row(buffer, current_row, 0);
        guac_terminal_buffer_row* dst_row = guac_terminal_buffer_get_row(buffer, current_row + offset, src_row->length);

        /* Copy data */
        memcpy(dst_row->characters, src_row->characters, sizeof(guac_terminal_char) * src_row->length);
        dst_row->length = src_row->length;

        /* Next current_row */
        current_row += step;

    }

}

void guac_terminal_buffer_set_columns(guac_terminal_buffer* buffer, int row,
        int start_column, int end_column, guac_terminal_char* character) {

    int i, j;
    guac_terminal_char* current;

    /* Build continuation char (for multicolumn characters) */
    guac_terminal_char continuation_char;
    continuation_char.value = GUAC_CHAR_CONTINUATION;
    continuation_char.attributes = character->attributes;
    continuation_char.width = 0; /* Not applicable for GUAC_CHAR_CONTINUATION */

    /* Get and expand row */
    guac_terminal_buffer_row* buffer_row = guac_terminal_buffer_get_row(buffer, row, end_column+1);

    /* Set values */
    current = &(buffer_row->characters[start_column]);
    for (i = start_column; i <= end_column; i += character->width) {

        *(current++) = *character;

        /* Store any required continuation characters */
        for (j=1; j < character->width; j++)
            *(current++) = continuation_char;

    }

    /* Update length depending on row written */
    if (character->value != 0 && row >= buffer->length) 
        buffer->length = row+1;

}

