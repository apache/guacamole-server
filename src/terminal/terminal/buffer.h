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


#ifndef _GUAC_TERMINAL_BUFFER_H
#define _GUAC_TERMINAL_BUFFER_H

#include "config.h"

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
     * The character to assign to newly-allocated cells.
     */
    guac_terminal_char default_character;

    /**
     * Array of buffer rows. This array functions as a ring buffer.
     * When a new row needs to be appended, the top reference is moved down
     * and the old top row is replaced.
     */
    guac_terminal_buffer_row* rows;

    /**
     * The index of the first row in the buffer (the row which represents row 0
     * with respect to the terminal display). This is also the index of the row
     * to replace when insufficient space remains in the buffer to add a new
     * row.
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
 * Allocates a new buffer having the given maximum number of rows. New character cells will
 * be initialized to the given character.
 */
guac_terminal_buffer* guac_terminal_buffer_alloc(int rows, guac_terminal_char* default_character);

/**
 * Frees the given buffer.
 */
void guac_terminal_buffer_free(guac_terminal_buffer* buffer);

/**
 * Returns the row at the given location. The row returned is guaranteed to be at least the given
 * width.
 */
guac_terminal_buffer_row* guac_terminal_buffer_get_row(guac_terminal_buffer* buffer, int row, int width);

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

