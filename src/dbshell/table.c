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

#include "dbshell/buffer.h"
#include "dbshell/line-editor.h"
#include "dbshell/table.h"

#include <guacamole/mem.h>
#include <guacamole/string.h>
#include <terminal/terminal.h>

#include <stdbool.h>
#include <string.h>

/**
 * The text rendered in place of database NULL values.
 */
#define GUAC_DBSHELL_TABLE_NULL_TEXT "NULL"

/**
 * The UTF-8 encoding of the marker appended to truncated cell values
 * (HORIZONTAL ELLIPSIS).
 */
#define GUAC_DBSHELL_TABLE_ELLIPSIS "\xE2\x80\xA6"

struct guac_dbshell_table {

    /**
     * The terminal the table is rendered to.
     */
    guac_terminal* term;

    /**
     * The number of columns of the result set.
     */
    int num_columns;

    /**
     * The sanitized column names. Elements are NULL until assigned.
     */
    char** headers;

    /**
     * Whether each column holds numeric values and should be
     * right-aligned.
     */
    bool* numeric;

    /**
     * The display width of each column, in terminal columns, excluding
     * padding. Widths grow while rows are being buffered and are fixed
     * once streaming begins.
     */
    int* widths;

    /**
     * The buffered leading rows, used to compute column widths before any
     * output is produced. Each row is an array of num_columns sanitized
     * strings, where NULL denotes the database NULL value.
     */
    char*** rows;

    /**
     * The number of buffered rows.
     */
    int num_buffered;

    /**
     * Whether the buffered window has been flushed and subsequent rows are
     * being streamed directly to the terminal.
     */
    bool streaming;

    /**
     * The total number of body rows added to the table.
     */
    unsigned long total_rows;

} ;

char* guac_dbshell_table_sanitize(const char* value) {

    char* sanitized = guac_strdup(value);

    /* Replace all control characters, preserving multi-byte UTF-8
     * characters (whose bytes all have the high bit set) */
    for (char* c = sanitized; *c != '\0'; c++) {
        unsigned char byte = (unsigned char) *c;
        if (byte < 0x20 || byte == 0x7F)
            *c = ' ';
    }

    return sanitized;

}

guac_dbshell_table* guac_dbshell_table_begin(guac_terminal* term,
        int num_columns) {

    if (num_columns <= 0)
        return NULL;

    guac_dbshell_table* table = guac_mem_zalloc(sizeof(guac_dbshell_table));

    table->term = term;
    table->num_columns = num_columns;

    table->headers = guac_mem_zalloc(guac_mem_ckd_mul_or_die(num_columns,
                sizeof(char*)));
    table->numeric = guac_mem_zalloc(guac_mem_ckd_mul_or_die(num_columns,
                sizeof(bool)));
    table->widths = guac_mem_zalloc(guac_mem_ckd_mul_or_die(num_columns,
                sizeof(int)));
    table->rows = guac_mem_zalloc(guac_mem_ckd_mul_or_die(
                GUAC_DBSHELL_TABLE_SIZING_ROWS, sizeof(char**)));

    return table;

}

/**
 * Returns the display width which the given cell value contributes to its
 * column, capped at the maximum column width.
 *
 * @param value
 *     The sanitized cell value, or NULL for the database NULL value.
 *
 * @return
 *     The capped display width of the value.
 */
static int guac_dbshell_table_value_width(const char* value) {

    if (value == NULL)
        value = GUAC_DBSHELL_TABLE_NULL_TEXT;

    int width = guac_dbshell_display_width(value, strlen(value));
    if (width > GUAC_DBSHELL_TABLE_MAX_COL_WIDTH)
        width = GUAC_DBSHELL_TABLE_MAX_COL_WIDTH;

    return width;

}

void guac_dbshell_table_set_header(guac_dbshell_table* table, int column,
        const char* name, bool numeric) {

    if (column < 0 || column >= table->num_columns)
        return;

    if (name == NULL)
        name = "";

    guac_mem_free(table->headers[column]);
    table->headers[column] = guac_dbshell_table_sanitize(name);
    table->numeric[column] = numeric;

    /* Columns are always at least as wide as their headers */
    int width = guac_dbshell_table_value_width(table->headers[column]);
    if (width > table->widths[column])
        table->widths[column] = width;

}

/**
 * Appends a horizontal border row ("+-----+-----+") for the given table to
 * the given output buffer.
 *
 * @param table
 *     The table whose border should be appended.
 *
 * @param output
 *     The output buffer to append to.
 */
static void guac_dbshell_table_append_border(guac_dbshell_table* table,
        guac_dbshell_buffer* output) {

    for (int i = 0; i < table->num_columns; i++) {
        guac_dbshell_buffer_append(output, "+", 1);
        guac_dbshell_buffer_append_repeat(output, '-',
                table->widths[i] + 2);
    }

    guac_dbshell_buffer_append_string(output, "+\r\n");

}

/**
 * Appends the given cell value to the given output buffer, padded or
 * truncated to exactly the width of the given column and surrounded by
 * single spaces.
 *
 * @param table
 *     The table containing the cell.
 *
 * @param output
 *     The output buffer to append to.
 *
 * @param column
 *     The index of the column containing the cell.
 *
 * @param value
 *     The sanitized value of the cell, or NULL for the database NULL
 *     value.
 */
static void guac_dbshell_table_append_cell(guac_dbshell_table* table,
        guac_dbshell_buffer* output, int column, const char* value) {

    if (value == NULL)
        value = GUAC_DBSHELL_TABLE_NULL_TEXT;

    int col_width = table->widths[column];
    int value_length = strlen(value);
    int value_width = guac_dbshell_display_width(value, value_length);

    guac_dbshell_buffer_append(output, " ", 1);

    /* Truncate values wider than the column, reserving one column for the
     * truncation marker */
    if (value_width > col_width) {

        int written = 0;
        int position = 0;

        while (position < value_length) {

            int next = position;
            int width = guac_dbshell_display_next(value, value_length,
                    &next);

            if (written + width > col_width - 1)
                break;

            guac_dbshell_buffer_append(output, value + position,
                    next - position);
            written += width;
            position = next;

        }

        guac_dbshell_buffer_append_string(output,
                GUAC_DBSHELL_TABLE_ELLIPSIS);
        written++;

        /* Pad to the exact column width (a wide character may have stopped
         * short of the boundary) */
        guac_dbshell_buffer_append_repeat(output, ' ',
                col_width - written);

    }

    /* Pad values narrower than the column, right-aligning numeric
     * columns */
    else {

        int padding = col_width - value_width;

        if (table->numeric[column]) {
            guac_dbshell_buffer_append_repeat(output, ' ', padding);
            guac_dbshell_buffer_append(output, value, value_length);
        }
        else {
            guac_dbshell_buffer_append(output, value, value_length);
            guac_dbshell_buffer_append_repeat(output, ' ', padding);
        }

    }

    guac_dbshell_buffer_append_string(output, " |");

}

/**
 * Appends a complete body or header row to the given output buffer.
 *
 * @param table
 *     The table containing the row.
 *
 * @param output
 *     The output buffer to append to.
 *
 * @param values
 *     An array of exactly num_columns sanitized values, where NULL denotes
 *     the database NULL value.
 */
static void guac_dbshell_table_append_row(guac_dbshell_table* table,
        guac_dbshell_buffer* output, char** values) {

    guac_dbshell_buffer_append(output, "|", 1);

    for (int i = 0; i < table->num_columns; i++)
        guac_dbshell_table_append_cell(table, output, i, values[i]);

    guac_dbshell_buffer_append_string(output, "\r\n");

}

/**
 * Renders the header and all buffered rows of the given table to its
 * terminal, freeing the buffered rows and switching the table to streaming
 * output.
 *
 * @param table
 *     The table to flush.
 */
static void guac_dbshell_table_flush(guac_dbshell_table* table) {

    guac_dbshell_buffer output;
    guac_dbshell_buffer_init(&output);

    /* Render header between two borders */
    guac_dbshell_table_append_border(table, &output);
    guac_dbshell_table_append_row(table, &output, table->headers);
    guac_dbshell_table_append_border(table, &output);

    /* Render all buffered rows */
    for (int i = 0; i < table->num_buffered; i++) {

        char** row = table->rows[i];
        guac_dbshell_table_append_row(table, &output, row);

        for (int j = 0; j < table->num_columns; j++)
            guac_mem_free(row[j]);
        guac_mem_free(row);

    }

    guac_terminal_write(table->term, output.data, output.length);
    guac_dbshell_buffer_destroy(&output);

    table->num_buffered = 0;
    table->streaming = true;

}

void guac_dbshell_table_add_row(guac_dbshell_table* table,
        const char** values) {

    table->total_rows++;

    /* Once streaming, rows are rendered immediately using fixed widths */
    if (table->streaming) {

        guac_dbshell_buffer output;
        guac_dbshell_buffer_init(&output);

        char** sanitized = guac_mem_zalloc(guac_mem_ckd_mul_or_die(
                    table->num_columns, sizeof(char*)));

        for (int i = 0; i < table->num_columns; i++) {
            if (values[i] != NULL)
                sanitized[i] = guac_dbshell_table_sanitize(values[i]);
        }

        guac_dbshell_table_append_row(table, &output, sanitized);
        guac_terminal_write(table->term, output.data, output.length);
        guac_dbshell_buffer_destroy(&output);

        for (int i = 0; i < table->num_columns; i++)
            guac_mem_free(sanitized[i]);
        guac_mem_free(sanitized);

        return;

    }

    /* Otherwise, buffer the row and grow column widths */
    char** row = guac_mem_zalloc(guac_mem_ckd_mul_or_die(
                table->num_columns, sizeof(char*)));

    for (int i = 0; i < table->num_columns; i++) {

        if (values[i] != NULL)
            row[i] = guac_dbshell_table_sanitize(values[i]);

        int width = guac_dbshell_table_value_width(row[i]);
        if (width > table->widths[i])
            table->widths[i] = width;

    }

    table->rows[table->num_buffered++] = row;

    /* Flush and switch to streaming once the sizing window is full */
    if (table->num_buffered >= GUAC_DBSHELL_TABLE_SIZING_ROWS)
        guac_dbshell_table_flush(table);

}

unsigned long guac_dbshell_table_end(guac_dbshell_table* table) {

    /* Render everything if still buffering */
    if (!table->streaming)
        guac_dbshell_table_flush(table);

    /* Closing border */
    guac_dbshell_buffer output;
    guac_dbshell_buffer_init(&output);
    guac_dbshell_table_append_border(table, &output);
    guac_terminal_write(table->term, output.data, output.length);
    guac_dbshell_buffer_destroy(&output);

    unsigned long total_rows = table->total_rows;

    /* Free all table resources */
    for (int i = 0; i < table->num_columns; i++)
        guac_mem_free(table->headers[i]);

    guac_mem_free(table->headers);
    guac_mem_free(table->numeric);
    guac_mem_free(table->widths);
    guac_mem_free(table->rows);
    guac_mem_free(table);

    return total_rows;

}
