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

#include "terminal/buffer.h"
#include "terminal/common.h"

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

    /* Normalize row index into a scrollback buffer index */
    int index = (buffer->top + row) % buffer->available;
    if (index < 0)
        index += buffer->available;

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

    /* Do nothing if glyph is empty */
    if (character->width == 0)
        return;

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

