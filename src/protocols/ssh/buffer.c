
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

#include <stdlib.h>
#include <string.h>

#include "buffer.h"
#include "common.h"

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

    int i;
    guac_terminal_char* current;

    /* Get and expand row */
    guac_terminal_buffer_row* buffer_row = guac_terminal_buffer_get_row(buffer, row, end_column+1);

    /* Set values */
    current = &(buffer_row->characters[start_column]);
    for (i=start_column; i<=end_column; i++)
        *(current++) = *character;

    /* Update length depending on row written */
    if (character->value != 0 && row >= buffer->length) 
        buffer->length = row+1;

}

