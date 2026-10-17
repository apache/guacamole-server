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

#include "dbshell/splitter.h"

#include <guacamole/mem.h>

#include <ctype.h>
#include <stdbool.h>
#include <string.h>

/**
 * The maximum number of bytes of input which may accumulate within a
 * splitter without forming a complete statement. Input beyond this limit is
 * discarded, protecting against unbounded memory growth from unterminated
 * string literals in large pasted input.
 */
#define GUAC_DBSHELL_MAX_STATEMENT_LENGTH 1048576

/**
 * The lexical states of the statement scanner.
 */
typedef enum guac_dbshell_lex_state {

    /**
     * Outside any string literal or comment.
     */
    GUAC_DBSHELL_LEX_NORMAL,

    /**
     * Within a single-quoted string literal.
     */
    GUAC_DBSHELL_LEX_SINGLE,

    /**
     * Within a double-quoted string literal or identifier.
     */
    GUAC_DBSHELL_LEX_DOUBLE,

    /**
     * Within a backtick-quoted identifier (MySQL).
     */
    GUAC_DBSHELL_LEX_BACKTICK,

    /**
     * Within a bracket-quoted identifier (Transact-SQL).
     */
    GUAC_DBSHELL_LEX_BRACKET,

    /**
     * Within a dollar-quoted string literal (PostgreSQL).
     */
    GUAC_DBSHELL_LEX_DOLLAR,

    /**
     * Within a comment which extends to the end of the line.
     */
    GUAC_DBSHELL_LEX_LINE_COMMENT,

    /**
     * Within a block comment.
     */
    GUAC_DBSHELL_LEX_BLOCK_COMMENT

} guac_dbshell_lex_state;

/**
 * The maximum length of the tag of a PostgreSQL dollar-quoted string
 * literal which the scanner will recognize, in bytes, excluding the
 * enclosing dollar signs.
 */
#define GUAC_DBSHELL_MAX_DOLLAR_TAG_LENGTH 64

struct guac_dbshell_splitter {

    /**
     * The dialect determining the lexical rules applied by the splitter.
     */
    guac_dbshell_dialect dialect;

    /**
     * The accumulated input which has not yet been emitted as complete
     * statements.
     */
    char* buffer;

    /**
     * The number of bytes of accumulated input.
     */
    int length;

    /**
     * The number of bytes currently allocated for the input buffer.
     */
    int allocated;

    /**
     * Whether accumulated input has been discarded because it exceeded
     * GUAC_DBSHELL_MAX_STATEMENT_LENGTH.
     */
    bool overflowed;

};

/**
 * The result of scanning accumulated input for the end of the first
 * complete statement.
 */
typedef struct guac_dbshell_scan_result {

    /**
     * The index one past the last byte consumed by the statement, including
     * its terminator, or -1 if no complete statement was found.
     */
    int end;

    /**
     * The number of trailing bytes within the consumed region which belong
     * to the terminator and must not be included in the statement text.
     */
    int terminator_length;

} guac_dbshell_scan_result;

/**
 * Returns whether the given byte may appear within the tag of a PostgreSQL
 * dollar-quoted string literal.
 *
 * @param c
 *     The byte to test.
 *
 * @return
 *     Non-zero if the byte may appear within a dollar-quote tag, zero
 *     otherwise.
 */
static int guac_dbshell_is_dollar_tag_byte(char c) {
    return isalnum((unsigned char) c) || c == '_';
}

/**
 * Returns whether the line within the given buffer, spanning the given
 * range, consists solely of the given word after leading and trailing
 * whitespace is ignored, compared case-insensitively.
 *
 * @param buffer
 *     The buffer containing the line.
 *
 * @param start
 *     The index of the first byte of the line.
 *
 * @param end
 *     The index one past the last byte of the line, not including any
 *     newline character.
 *
 * @param word
 *     The null-terminated word to compare the line against.
 *
 * @return
 *     Non-zero if the line consists solely of the given word, zero
 *     otherwise.
 */
static int guac_dbshell_line_is_word(const char* buffer, int start, int end,
        const char* word) {

    /* Ignore leading whitespace */
    while (start < end && isspace((unsigned char) buffer[start]))
        start++;

    /* Ignore trailing whitespace */
    while (end > start && isspace((unsigned char) buffer[end - 1]))
        end--;

    /* Compare remaining text against word, case-insensitively */
    int word_length = strlen(word);
    if (end - start != word_length)
        return 0;

    for (int i = 0; i < word_length; i++) {
        if (tolower((unsigned char) buffer[start + i])
                != tolower((unsigned char) word[i]))
            return 0;
    }

    return 1;

}

/**
 * Reads the next SQL word (a run of alphabetic characters) from the given
 * buffer, skipping any preceding whitespace, storing a lowercased copy
 * within the given word buffer.
 *
 * @param buffer
 *     The buffer to read from.
 *
 * @param length
 *     The number of bytes within the buffer.
 *
 * @param pos
 *     The index to begin reading at. On return, this will have advanced
 *     past the word read.
 *
 * @param word
 *     The buffer to store the lowercased word within.
 *
 * @param word_size
 *     The size of the word buffer, in bytes. Words too long to fit are
 *     truncated.
 */
static void guac_dbshell_read_word(const char* buffer, int length, int* pos,
        char* word, int word_size) {

    int i = *pos;
    int written = 0;

    /* Skip whitespace preceding the word */
    while (i < length && isspace((unsigned char) buffer[i]))
        i++;

    /* Copy and lowercase the word itself */
    while (i < length && isalpha((unsigned char) buffer[i])) {
        if (written < word_size - 1)
            word[written++] = tolower((unsigned char) buffer[i]);
        i++;
    }

    word[written] = '\0';
    *pos = i;

}

/**
 * Returns whether the statement beginning at the start of the given buffer
 * is an Oracle PL/SQL block (or the creation of a procedural object), in
 * which case semicolons do not terminate the statement and only a line
 * consisting solely of a slash does.
 *
 * @param buffer
 *     The buffer containing the accumulated statement text.
 *
 * @param length
 *     The number of bytes within the buffer.
 *
 * @return
 *     Non-zero if the statement is a PL/SQL block, zero otherwise.
 */
static int guac_dbshell_is_plsql_block(const char* buffer, int length) {

    char word[32];
    int pos = 0;

    /* Skip any comments preceding the statement, so that a leading comment
     * does not hide the first keyword */
    for (;;) {

        while (pos < length && isspace((unsigned char) buffer[pos]))
            pos++;

        if (pos + 1 < length && buffer[pos] == '-'
                && buffer[pos + 1] == '-') {
            while (pos < length && buffer[pos] != '\n')
                pos++;
        }

        else if (pos + 1 < length && buffer[pos] == '/'
                && buffer[pos + 1] == '*') {
            pos += 2;
            while (pos + 1 < length
                    && !(buffer[pos] == '*' && buffer[pos + 1] == '/'))
                pos++;
            pos += 2;
        }

        else
            break;

    }

    /* Examine the first word of the statement */
    guac_dbshell_read_word(buffer, length, &pos, word, sizeof(word));

    /* Anonymous blocks begin with DECLARE or BEGIN */
    if (strcmp(word, "declare") == 0 || strcmp(word, "begin") == 0)
        return 1;

    /* All other blocks begin with CREATE [OR REPLACE] */
    if (strcmp(word, "create") != 0)
        return 0;

    guac_dbshell_read_word(buffer, length, &pos, word, sizeof(word));
    if (strcmp(word, "or") == 0) {

        guac_dbshell_read_word(buffer, length, &pos, word, sizeof(word));
        if (strcmp(word, "replace") != 0)
            return 0;

        guac_dbshell_read_word(buffer, length, &pos, word, sizeof(word));

    }

    /* Skip editioning keywords which may precede the object type */
    if (strcmp(word, "editionable") == 0
            || strcmp(word, "noneditionable") == 0)
        guac_dbshell_read_word(buffer, length, &pos, word, sizeof(word));

    /* The statement is procedural if a procedural object type follows */
    return strcmp(word, "function") == 0
        || strcmp(word, "procedure") == 0
        || strcmp(word, "package") == 0
        || strcmp(word, "trigger") == 0
        || strcmp(word, "type") == 0
        || strcmp(word, "library") == 0;

}

/**
 * Scans the accumulated input of the given splitter for the end of the
 * first complete statement, according to the splitter's dialect.
 *
 * @param splitter
 *     The splitter whose accumulated input should be scanned.
 *
 * @param result
 *     The structure to populate with the scan result.
 */
static void guac_dbshell_scan(guac_dbshell_splitter* splitter,
        guac_dbshell_scan_result* result) {

    const char* buffer = splitter->buffer;
    int length = splitter->length;
    guac_dbshell_dialect dialect = splitter->dialect;

    guac_dbshell_lex_state state = GUAC_DBSHELL_LEX_NORMAL;

    /* The tag of the dollar-quoted literal currently being read */
    char dollar_tag[GUAC_DBSHELL_MAX_DOLLAR_TAG_LENGTH + 1] = { 0 };
    int dollar_tag_length = 0;

    /* Nesting depth of block comments (only PostgreSQL nests) */
    int comment_depth = 0;

    /* Depth of JSON braces/brackets and whether any content has begun */
    int json_depth = 0;
    bool json_started = false;

    /* The index of the first byte of the line currently being scanned */
    int line_start = 0;

    /* Whether semicolons terminate the current statement (Oracle PL/SQL
     * blocks are terminated only by a slash line) */
    bool semicolon_ends = true;
    if (dialect == GUAC_DBSHELL_DIALECT_ORACLE
            && guac_dbshell_is_plsql_block(buffer, length))
        semicolon_ends = false;

    result->end = -1;
    result->terminator_length = 0;

    for (int i = 0; i < length; i++) {

        char c = buffer[i];

        switch (state) {

            case GUAC_DBSHELL_LEX_NORMAL:

                /* JSON completion is tested at each end of line */
                if (dialect == GUAC_DBSHELL_DIALECT_JSON) {

                    if (c == '\'' || c == '"') {
                        state = (c == '\'') ? GUAC_DBSHELL_LEX_SINGLE
                                            : GUAC_DBSHELL_LEX_DOUBLE;
                        json_started = true;
                    }
                    else if (c == '{' || c == '[') {
                        json_depth++;
                        json_started = true;
                    }
                    else if (c == '}' || c == ']') {
                        if (json_depth > 0)
                            json_depth--;
                    }
                    else if (c == '\n') {
                        if (json_started && json_depth == 0) {
                            result->end = i + 1;
                            result->terminator_length = 1;
                            return;
                        }
                        line_start = i + 1;
                    }
                    else if (!isspace((unsigned char) c))
                        json_started = true;

                    break;

                }

                /* Semicolon ends the statement when permitted */
                if (c == ';' && semicolon_ends) {
                    result->end = i + 1;
                    result->terminator_length = 1;
                    return;
                }

                /* Terminator lines (GO, /) are tested at each end of line */
                if (c == '\n') {

                    if (dialect == GUAC_DBSHELL_DIALECT_TSQL
                            && guac_dbshell_line_is_word(buffer, line_start,
                                i, "go")) {
                        result->end = i + 1;
                        result->terminator_length = i + 1 - line_start;
                        return;
                    }

                    if (dialect == GUAC_DBSHELL_DIALECT_ORACLE
                            && guac_dbshell_line_is_word(buffer, line_start,
                                i, "/")) {
                        result->end = i + 1;
                        result->terminator_length = i + 1 - line_start;
                        return;
                    }

                    line_start = i + 1;
                    break;

                }

                /* String literal and quoted identifier openings */
                if (c == '\'')
                    state = GUAC_DBSHELL_LEX_SINGLE;

                else if (c == '"')
                    state = GUAC_DBSHELL_LEX_DOUBLE;

                else if (c == '`'
                        && dialect == GUAC_DBSHELL_DIALECT_MYSQL)
                    state = GUAC_DBSHELL_LEX_BACKTICK;

                else if (c == '['
                        && dialect == GUAC_DBSHELL_DIALECT_TSQL)
                    state = GUAC_DBSHELL_LEX_BRACKET;

                /* Dollar-quoted literals (PostgreSQL only) */
                else if (c == '$'
                        && dialect == GUAC_DBSHELL_DIALECT_PGSQL) {

                    /* Find closing dollar sign of the opening tag */
                    int tag_end = i + 1;
                    while (tag_end < length
                            && guac_dbshell_is_dollar_tag_byte(buffer[tag_end])
                            && tag_end - i - 1 < GUAC_DBSHELL_MAX_DOLLAR_TAG_LENGTH)
                        tag_end++;

                    /* A tag beginning with a digit is a positional
                     * parameter reference ($1), not a dollar quote */
                    if (tag_end < length && buffer[tag_end] == '$'
                            && (tag_end == i + 1
                                || !isdigit((unsigned char) buffer[i + 1]))) {
                        dollar_tag_length = tag_end - i - 1;
                        memcpy(dollar_tag, buffer + i + 1, dollar_tag_length);
                        dollar_tag[dollar_tag_length] = '\0';
                        state = GUAC_DBSHELL_LEX_DOLLAR;
                        i = tag_end;
                    }

                }

                /* MySQL "#" comments */
                else if (c == '#'
                        && dialect == GUAC_DBSHELL_DIALECT_MYSQL)
                    state = GUAC_DBSHELL_LEX_LINE_COMMENT;

                /* "--" comments (MySQL requires trailing whitespace) */
                else if (c == '-' && i + 1 < length
                        && buffer[i + 1] == '-') {

                    if (dialect != GUAC_DBSHELL_DIALECT_MYSQL
                            || i + 2 >= length
                            || isspace((unsigned char) buffer[i + 2])) {
                        state = GUAC_DBSHELL_LEX_LINE_COMMENT;
                        i++;
                    }

                }

                /* Block comments */
                else if (c == '/' && i + 1 < length
                        && buffer[i + 1] == '*') {
                    state = GUAC_DBSHELL_LEX_BLOCK_COMMENT;
                    comment_depth = 1;
                    i++;
                }

                break;

            case GUAC_DBSHELL_LEX_SINGLE:
            case GUAC_DBSHELL_LEX_DOUBLE:

                /* Backslash escapes apply only to MySQL (and JSON strings,
                 * which use double quotes) */
                if (c == '\\'
                        && (dialect == GUAC_DBSHELL_DIALECT_MYSQL
                            || dialect == GUAC_DBSHELL_DIALECT_JSON)) {
                    if (i + 1 < length)
                        i++;
                    break;
                }

                /* Close quote, unless doubled */
                if ((state == GUAC_DBSHELL_LEX_SINGLE && c == '\'')
                        || (state == GUAC_DBSHELL_LEX_DOUBLE && c == '"')) {

                    /* Doubled quote characters remain within the literal
                     * (SQL dialects only) */
                    if (dialect != GUAC_DBSHELL_DIALECT_JSON
                            && i + 1 < length && buffer[i + 1] == c)
                        i++;
                    else
                        state = GUAC_DBSHELL_LEX_NORMAL;

                }

                break;

            case GUAC_DBSHELL_LEX_BACKTICK:

                /* Close backtick, unless doubled */
                if (c == '`') {
                    if (i + 1 < length && buffer[i + 1] == '`')
                        i++;
                    else
                        state = GUAC_DBSHELL_LEX_NORMAL;
                }

                break;

            case GUAC_DBSHELL_LEX_BRACKET:

                /* Close bracket, unless doubled */
                if (c == ']') {
                    if (i + 1 < length && buffer[i + 1] == ']')
                        i++;
                    else
                        state = GUAC_DBSHELL_LEX_NORMAL;
                }

                break;

            case GUAC_DBSHELL_LEX_DOLLAR:

                /* Close only on the exact matching tag */
                if (c == '$' && i + dollar_tag_length + 1 < length
                        && memcmp(buffer + i + 1, dollar_tag,
                            dollar_tag_length) == 0
                        && buffer[i + dollar_tag_length + 1] == '$') {
                    i += dollar_tag_length + 1;
                    state = GUAC_DBSHELL_LEX_NORMAL;
                }

                break;

            case GUAC_DBSHELL_LEX_LINE_COMMENT:

                if (c == '\n') {
                    state = GUAC_DBSHELL_LEX_NORMAL;
                    line_start = i + 1;
                }

                break;

            case GUAC_DBSHELL_LEX_BLOCK_COMMENT:

                /* Only PostgreSQL block comments nest */
                if (c == '/' && i + 1 < length && buffer[i + 1] == '*'
                        && dialect == GUAC_DBSHELL_DIALECT_PGSQL) {
                    comment_depth++;
                    i++;
                }

                else if (c == '*' && i + 1 < length
                        && buffer[i + 1] == '/') {
                    comment_depth--;
                    i++;
                    if (comment_depth == 0)
                        state = GUAC_DBSHELL_LEX_NORMAL;
                }

                break;

        }

    }

}

guac_dbshell_splitter* guac_dbshell_splitter_alloc(
        guac_dbshell_dialect dialect) {

    guac_dbshell_splitter* splitter =
        guac_mem_zalloc(sizeof(guac_dbshell_splitter));

    splitter->dialect = dialect;
    return splitter;

}

void guac_dbshell_splitter_free(guac_dbshell_splitter* splitter) {
    guac_mem_free(splitter->buffer);
    guac_mem_free(splitter);
}

void guac_dbshell_splitter_feed(guac_dbshell_splitter* splitter,
        const char* line) {

    int line_length = strlen(line);

    /* Discard input beyond the statement length limit */
    if (splitter->length + line_length + 1
            > GUAC_DBSHELL_MAX_STATEMENT_LENGTH) {
        guac_dbshell_splitter_reset(splitter);
        splitter->overflowed = true;
        return;
    }

    /* Expand buffer to fit the line and its newline */
    int required = splitter->length + line_length + 1;
    if (required > splitter->allocated) {

        int allocated = splitter->allocated;
        if (allocated == 0)
            allocated = 1024;

        while (allocated < required)
            allocated *= 2;

        splitter->buffer = guac_mem_realloc(splitter->buffer, allocated);
        splitter->allocated = allocated;

    }

    /* Append line and implicit newline */
    memcpy(splitter->buffer + splitter->length, line, line_length);
    splitter->length += line_length;
    splitter->buffer[splitter->length++] = '\n';

}

char* guac_dbshell_splitter_next_statement(
        guac_dbshell_splitter* splitter) {

    for (;;) {

        /* Locate the end of the first complete statement, if any */
        guac_dbshell_scan_result result;
        guac_dbshell_scan(splitter, &result);

        if (result.end < 0)
            return NULL;

        /* Extract statement text, without its terminator */
        int content_length = result.end - result.terminator_length;
        const char* content = splitter->buffer;

        /* Trim leading and trailing whitespace */
        while (content_length > 0
                && isspace((unsigned char) *content)) {
            content++;
            content_length--;
        }

        while (content_length > 0
                && isspace((unsigned char) content[content_length - 1]))
            content_length--;

        char* statement = NULL;
        if (content_length > 0) {
            statement = guac_mem_alloc(content_length + 1);
            memcpy(statement, content, content_length);
            statement[content_length] = '\0';
        }

        /* Remove the consumed region from the accumulated input */
        splitter->length -= result.end;
        memmove(splitter->buffer, splitter->buffer + result.end,
                splitter->length);

        /* Skip statements which are empty after trimming */
        if (statement != NULL)
            return statement;

    }

}

bool guac_dbshell_splitter_pending(guac_dbshell_splitter* splitter) {

    /* Input is pending if any accumulated byte is not whitespace */
    for (int i = 0; i < splitter->length; i++) {
        if (!isspace((unsigned char) splitter->buffer[i]))
            return true;
    }

    return false;

}

void guac_dbshell_splitter_reset(guac_dbshell_splitter* splitter) {
    splitter->length = 0;
    splitter->overflowed = false;
}

bool guac_dbshell_splitter_overflowed(guac_dbshell_splitter* splitter) {
    bool overflowed = splitter->overflowed;
    splitter->overflowed = false;
    return overflowed;
}
