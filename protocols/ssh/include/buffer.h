
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

#ifndef _SSH_GUAC_BUFFER_H
#define _SSH_GUAC_BUFFER_H

#include "types.h"

/**
 * A single variable-length row of terminal data.
 */
typedef struct guac_terminal_buffer_row {

    /**
     * Array of guac_terminal_char representing the contents of the row.
     */
    guac_terminal_char* characters;

    /**
     * The length of this row in characters. This is the number of initialized
     * characters in the buffer, usually equal to the number of characters
     * in the screen width at the time this row was created.
     */
    int length;

    /**
     * The number of elements in the characters array. After the length
     * equals this value, the array must be resized.
     */
    int available;

} guac_terminal_buffer_row;

/**
 * A buffer containing a constant number of arbitrary-length rows.
 * New rows can be appended to the buffer, with the oldest row replaced with
 * the new row.
 */
typedef struct guac_terminal_buffer {

    /**
     * Array of buffer rows. This array functions as a ring buffer.
     * When a new row needs to be appended, the top reference is moved down
     * and the old top row is replaced.
     */
    guac_terminal_buffer_row* rows;

    /**
     * The row to replace when adding a new row to the buffer.
     */
    int top;

    /**
     * The number of rows currently stored in the buffer.
     */
    int length;

    /**
     * The number of rows in the buffer. This is the total capacity
     * of the buffer.
     */
    int available;

} guac_terminal_buffer;

/**
 * Allocates a new buffer having the given maximum number of rows.
 */
guac_terminal_buffer* guac_terminal_buffer_alloc(int rows);

/**
 * Frees the given buffer.
 */
void guac_terminal_buffer_free(guac_terminal_buffer* buffer);

/**
 * Returns the row at the given location.
 */
guac_terminal_buffer_row* guac_terminal_buffer_get_row(guac_terminal_buffer* buffer, int row);

/**
 * Ensures the given row has at least the given number of character spaces available. If new characters
 * must be added, they are initialized with the given fill character.
 */
void guac_terminal_buffer_prepare_row(guac_terminal_buffer_row* row, int width, guac_terminal_char* fill);

/**
 * Copies the given range of columns to a new location, offset from
 * the original by the given number of columns.
 */
void guac_terminal_buffer_copy_columns(guac_terminal_buffer* buffer, int row,
        int start_column, int end_column, int offset);

/**
 * Copies the given range of rows to a new location, offset from the
 * original by the given number of rows.
 */
void guac_terminal_buffer_copy_rows(guac_terminal_buffer* buffer,
        int start_row, int end_row, int offset);

/**
 * Sets the given range of columns within the given row to the given
 * character.
 */
void guac_terminal_buffer_set_columns(guac_terminal_buffer* buffer, int row,
        int start_column, int end_column, guac_terminal_char* character);

#endif

