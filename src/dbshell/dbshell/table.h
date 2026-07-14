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

#ifndef GUAC_DBSHELL_TABLE_H
#define GUAC_DBSHELL_TABLE_H

/**
 * Declarations for the result table renderer of the dbshell REPL, which
 * renders result sets as bordered ASCII tables in the style of common
 * database CLIs. Because result sets may be arbitrarily large, an initial
 * window of rows is buffered to compute column widths, after which rows are
 * streamed directly to the terminal using the computed widths.
 *
 * All cell data is sanitized before rendering: control characters are
 * replaced such that data received from the database server can never
 * inject terminal escape sequences.
 *
 * @file table.h
 */

#include <terminal/terminal.h>

#include <stdbool.h>

/**
 * The number of leading rows buffered in memory in order to compute column
 * widths before any output is produced. Rows beyond this window are
 * streamed using the widths computed from the buffered window.
 */
#define GUAC_DBSHELL_TABLE_SIZING_ROWS 1000

/**
 * The maximum display width of a single table column, in terminal columns.
 * Cell values wider than this are truncated, with the final column replaced
 * by a truncation marker.
 */
#define GUAC_DBSHELL_TABLE_MAX_COL_WIDTH 128

/**
 * A result table being rendered to a terminal. The members of this
 * structure are internal to the table renderer implementation.
 */
typedef struct guac_dbshell_table guac_dbshell_table;

/**
 * Begins rendering of a result table having the given number of columns to
 * the given terminal. The returned table must eventually be finished and
 * freed with guac_dbshell_table_end().
 *
 * @param term
 *     The terminal the table will be rendered to.
 *
 * @param num_columns
 *     The number of columns of the result set. Must be positive.
 *
 * @return
 *     A newly-allocated table, or NULL if the number of columns is not
 *     positive.
 */
guac_dbshell_table* guac_dbshell_table_begin(guac_terminal* term,
        int num_columns);

/**
 * Assigns the header (column name) of the given column of the given table,
 * along with whether the column holds numeric values and should thus be
 * right-aligned. All headers must be assigned before the first row is
 * added.
 *
 * @param table
 *     The table whose column header is being assigned.
 *
 * @param column
 *     The index of the column, where 0 is the leftmost column.
 *
 * @param name
 *     The name of the column. A copy is made; the caller retains ownership.
 *     If NULL, an empty name is used.
 *
 * @param numeric
 *     Whether values within the column are numeric and should be
 *     right-aligned.
 */
void guac_dbshell_table_set_header(guac_dbshell_table* table, int column,
        const char* name, bool numeric);

/**
 * Adds a row of values to the given table. Values are copied and sanitized
 * immediately; the caller retains ownership of the given array and strings.
 *
 * @param table
 *     The table to add the row to.
 *
 * @param values
 *     An array of exactly num_columns null-terminated UTF-8 strings, where
 *     a NULL element denotes the database NULL value.
 */
void guac_dbshell_table_add_row(guac_dbshell_table* table,
        const char** values);

/**
 * Completes rendering of the given table, flushing all buffered rows and
 * the closing border to the terminal, and frees the table. If no rows were
 * added, only the header and borders are rendered.
 *
 * @param table
 *     The table to complete and free.
 *
 * @return
 *     The total number of rows which were rendered within the body of the
 *     table.
 */
unsigned long guac_dbshell_table_end(guac_dbshell_table* table);

/**
 * Copies the given UTF-8 cell value into a newly-allocated string,
 * replacing each control character (all bytes below 0x20 and the DEL
 * character) with a single space such that data from the database server
 * cannot inject terminal control sequences or break table layout. The
 * result must eventually be freed with guac_mem_free().
 *
 * @param value
 *     The null-terminated value to sanitize.
 *
 * @return
 *     A newly-allocated, sanitized copy of the given value.
 */
char* guac_dbshell_table_sanitize(const char* value);

#endif
