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

#ifndef GUAC_DBSHELL_SPLITTER_H
#define GUAC_DBSHELL_SPLITTER_H

/**
 * Declarations for the statement splitter of the dbshell REPL, which
 * accumulates submitted input lines and determines where complete statements
 * end according to the lexical rules of the database's query language
 * (string literals, comments, statement terminators, etc.).
 *
 * @file splitter.h
 */

#include <stdbool.h>

/**
 * The statement-splitting dialects understood by the splitter, determining
 * which quoting styles, comment styles, and statement terminators apply.
 */
typedef enum guac_dbshell_dialect {

    /**
     * MySQL-family SQL: single-quote, double-quote, and backtick quoting
     * with backslash escapes, "-- " (with trailing space), "#", and block
     * comments, statements terminated by semicolons.
     */
    GUAC_DBSHELL_DIALECT_MYSQL,

    /**
     * PostgreSQL SQL: single-quote, double-quote, and dollar-quoted string
     * literals, "--" and nestable block comments, statements terminated by
     * semicolons.
     */
    GUAC_DBSHELL_DIALECT_PGSQL,

    /**
     * Transact-SQL: single-quote, double-quote, and bracket quoting, "--"
     * and block comments, statements terminated by semicolons or by a line
     * consisting solely of the batch separator "GO".
     */
    GUAC_DBSHELL_DIALECT_TSQL,

    /**
     * Oracle SQL and PL/SQL: single-quote and double-quote literals, "--"
     * and block comments. Plain SQL statements are terminated by
     * semicolons, while PL/SQL blocks (statements beginning with DECLARE or
     * BEGIN, and CREATE statements for procedural objects) are terminated
     * only by a line consisting solely of a slash ("/"), as in SQL*Plus.
     */
    GUAC_DBSHELL_DIALECT_ORACLE,

    /**
     * A single JSON document: a statement is complete once at least one
     * value has been read and all braces and brackets outside string
     * literals are balanced.
     */
    GUAC_DBSHELL_DIALECT_JSON

} guac_dbshell_dialect;

/**
 * A statement splitter which accumulates input lines and produces complete
 * statements according to a fixed dialect. The members of this structure
 * are internal to the splitter implementation.
 */
typedef struct guac_dbshell_splitter guac_dbshell_splitter;

/**
 * Allocates a new, empty statement splitter for the given dialect. The
 * returned splitter must eventually be freed with
 * guac_dbshell_splitter_free().
 *
 * @param dialect
 *     The dialect determining the lexical rules the splitter applies.
 *
 * @return
 *     A newly-allocated, empty statement splitter.
 */
guac_dbshell_splitter* guac_dbshell_splitter_alloc(
        guac_dbshell_dialect dialect);

/**
 * Frees the given statement splitter and any input accumulated within it.
 *
 * @param splitter
 *     The splitter to free.
 */
void guac_dbshell_splitter_free(guac_dbshell_splitter* splitter);

/**
 * Appends the given input line to the input accumulated within the given
 * splitter. A newline character is implicitly appended after the given
 * line. Completed statements, if any, become available through
 * guac_dbshell_splitter_next_statement().
 *
 * @param splitter
 *     The splitter which should receive the line.
 *
 * @param line
 *     The null-terminated line of input to append, without any trailing
 *     newline character.
 */
void guac_dbshell_splitter_feed(guac_dbshell_splitter* splitter,
        const char* line);

/**
 * Removes and returns the next complete statement from the input
 * accumulated within the given splitter, if any. The returned statement has
 * its statement terminator removed and leading/trailing whitespace trimmed.
 * Statements which are empty after trimming are skipped.
 *
 * @param splitter
 *     The splitter to retrieve a statement from.
 *
 * @return
 *     A newly-allocated string containing the next complete statement,
 *     which must eventually be freed with a call to guac_mem_free(), or
 *     NULL if no complete statement is currently available.
 */
char* guac_dbshell_splitter_next_statement(guac_dbshell_splitter* splitter);

/**
 * Returns whether the given splitter currently holds accumulated input
 * which does not yet form a complete statement, in which case the REPL
 * should display a continuation prompt.
 *
 * @param splitter
 *     The splitter to test.
 *
 * @return
 *     True if the splitter holds an incomplete statement, false otherwise.
 */
bool guac_dbshell_splitter_pending(guac_dbshell_splitter* splitter);

/**
 * Discards all input accumulated within the given splitter, resetting it to
 * its initial state.
 *
 * @param splitter
 *     The splitter to reset.
 */
void guac_dbshell_splitter_reset(guac_dbshell_splitter* splitter);

/**
 * Returns whether accumulated input has been discarded by the given
 * splitter because it exceeded the maximum statement length, clearing the
 * overflow indication in the process.
 *
 * @param splitter
 *     The splitter to test.
 *
 * @return
 *     True if accumulated input was discarded since the last time the
 *     overflow indication was cleared, false otherwise.
 */
bool guac_dbshell_splitter_overflowed(guac_dbshell_splitter* splitter);

#endif
