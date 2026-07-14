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

#ifndef GUAC_DBSHELL_LINE_EDITOR_H
#define GUAC_DBSHELL_LINE_EDITOR_H

/**
 * Declarations for the line editor of the dbshell REPL. The line editor
 * reads raw bytes typed into the terminal and provides interactive editing
 * of a single logical input line (which may span multiple display rows),
 * including cursor movement, history navigation, and safe handling of
 * escape sequences.
 *
 * The editing buffer, input parser, and display layout computations are
 * plain data structures and pure functions, independent of the terminal,
 * so that they can be unit tested in isolation.
 *
 * @file line-editor.h
 */

#include "history.h"

#include <terminal/terminal.h>

#include <stdbool.h>

/**
 * The maximum number of bytes of a single UTF-8 encoded character.
 */
#define GUAC_DBSHELL_MAX_CHAR_LENGTH 4

/**
 * The number of spaces inserted into the editing buffer when the Tab key is
 * pressed. Literal tab characters are never stored, keeping display width
 * computations exact.
 */
#define GUAC_DBSHELL_TAB_WIDTH 4

/**
 * The overall outcome of reading one line of input from the terminal.
 */
typedef enum guac_dbshell_read_status {

    /**
     * A line of input was read successfully.
     */
    GUAC_DBSHELL_READ_LINE,

    /**
     * The user cancelled the current input with Ctrl+C.
     */
    GUAC_DBSHELL_READ_CANCELLED,

    /**
     * Input has ended, either because the user pressed Ctrl+D on an empty
     * line or because the terminal has been closed or stopped.
     */
    GUAC_DBSHELL_READ_CLOSED

} guac_dbshell_read_status;

/**
 * The distinct editing actions produced by the input parser from the raw
 * byte stream of the terminal.
 */
typedef enum guac_dbshell_key {

    /**
     * No complete action is available yet; more input bytes are required.
     */
    GUAC_DBSHELL_KEY_NONE,

    /**
     * A complete printable character is available within the parser's
     * char_buffer.
     */
    GUAC_DBSHELL_KEY_CHAR,

    /**
     * The Enter key (carriage return or line feed).
     */
    GUAC_DBSHELL_KEY_ENTER,

    /**
     * The Backspace key.
     */
    GUAC_DBSHELL_KEY_BACKSPACE,

    /**
     * The Delete key (forward delete).
     */
    GUAC_DBSHELL_KEY_DELETE,

    /**
     * The left arrow key.
     */
    GUAC_DBSHELL_KEY_LEFT,

    /**
     * The right arrow key.
     */
    GUAC_DBSHELL_KEY_RIGHT,

    /**
     * The up arrow key (older history).
     */
    GUAC_DBSHELL_KEY_UP,

    /**
     * The down arrow key (newer history).
     */
    GUAC_DBSHELL_KEY_DOWN,

    /**
     * The Home key or Ctrl+A.
     */
    GUAC_DBSHELL_KEY_HOME,

    /**
     * The End key or Ctrl+E.
     */
    GUAC_DBSHELL_KEY_END,

    /**
     * Ctrl+C.
     */
    GUAC_DBSHELL_KEY_INTERRUPT,

    /**
     * Ctrl+D.
     */
    GUAC_DBSHELL_KEY_EOF,

    /**
     * Ctrl+L (clear screen and redraw the current line).
     */
    GUAC_DBSHELL_KEY_CLEAR,

    /**
     * Ctrl+U (discard everything before the cursor).
     */
    GUAC_DBSHELL_KEY_KILL_LINE,

    /**
     * Ctrl+K (discard everything at and after the cursor).
     */
    GUAC_DBSHELL_KEY_KILL_TO_END,

    /**
     * Ctrl+W (discard the word before the cursor).
     */
    GUAC_DBSHELL_KEY_KILL_WORD,

    /**
     * The Tab key.
     */
    GUAC_DBSHELL_KEY_TAB,

    /**
     * A key or escape sequence which the editor deliberately ignores.
     */
    GUAC_DBSHELL_KEY_IGNORED

} guac_dbshell_key;

/**
 * The internal states of the input parser.
 */
typedef enum guac_dbshell_parser_state {

    /**
     * Not within any escape sequence or multi-byte character.
     */
    GUAC_DBSHELL_PARSER_GROUND,

    /**
     * An ESC byte has been read.
     */
    GUAC_DBSHELL_PARSER_ESC,

    /**
     * Within a CSI sequence ("ESC ["), reading parameter and intermediate
     * bytes until the final byte.
     */
    GUAC_DBSHELL_PARSER_CSI,

    /**
     * Within an SS3 sequence ("ESC O"), reading the single final byte.
     */
    GUAC_DBSHELL_PARSER_SS3,

    /**
     * Within a multi-byte UTF-8 character, reading continuation bytes.
     */
    GUAC_DBSHELL_PARSER_UTF8

} guac_dbshell_parser_state;

/**
 * The maximum number of parameter/intermediate bytes of a CSI sequence
 * which the parser stores for interpretation. Longer sequences are still
 * consumed correctly but are ignored.
 */
#define GUAC_DBSHELL_PARSER_CSI_LENGTH 16

/**
 * A state machine translating the raw byte stream of the terminal into
 * editing actions. Bytes are fed one at a time and each byte produces
 * exactly one guac_dbshell_key value (frequently GUAC_DBSHELL_KEY_NONE).
 */
typedef struct guac_dbshell_parser {

    /**
     * The current state of the parser.
     */
    guac_dbshell_parser_state state;

    /**
     * The bytes of the UTF-8 character currently being accumulated, valid
     * when guac_dbshell_parser_feed() returns GUAC_DBSHELL_KEY_CHAR.
     */
    char char_buffer[GUAC_DBSHELL_MAX_CHAR_LENGTH];

    /**
     * The number of bytes stored within char_buffer.
     */
    int char_length;

    /**
     * The number of UTF-8 continuation bytes still expected.
     */
    int utf8_remaining;

    /**
     * The parameter/intermediate bytes of the CSI sequence currently being
     * read.
     */
    char csi_buffer[GUAC_DBSHELL_PARSER_CSI_LENGTH];

    /**
     * The number of bytes stored within csi_buffer.
     */
    int csi_length;

    /**
     * Whether the previously-fed byte was a carriage return, used to
     * silently consume the line feed of a CRLF pair.
     */
    bool last_was_cr;

} guac_dbshell_parser;

/**
 * Initializes the given parser to its ground state.
 *
 * @param parser
 *     The parser to initialize.
 */
void guac_dbshell_parser_init(guac_dbshell_parser* parser);

/**
 * Feeds a single input byte to the given parser, returning the editing
 * action the byte completes, if any.
 *
 * @param parser
 *     The parser to feed the byte to.
 *
 * @param byte
 *     The input byte.
 *
 * @return
 *     The editing action completed by the byte, or GUAC_DBSHELL_KEY_NONE if
 *     the byte does not complete an action.
 */
guac_dbshell_key guac_dbshell_parser_feed(guac_dbshell_parser* parser,
        char byte);

/**
 * The editing buffer of the line editor: a single logical line of UTF-8
 * text together with a cursor position.
 */
typedef struct guac_dbshell_line {

    /**
     * The UTF-8 text of the line. This buffer is dynamically resized as
     * needed and is null-terminated.
     */
    char* buffer;

    /**
     * The length of the text within the buffer, in bytes, excluding the
     * null terminator.
     */
    int length;

    /**
     * The number of bytes currently allocated for the buffer.
     */
    int allocated;

    /**
     * The cursor position within the buffer, in bytes. The cursor always
     * lies on a UTF-8 character boundary, between 0 and length inclusive.
     */
    int cursor;

} guac_dbshell_line;

/**
 * Initializes the given editing buffer to an empty line. The buffer must
 * eventually be released with guac_dbshell_line_destroy().
 *
 * @param line
 *     The editing buffer to initialize.
 */
void guac_dbshell_line_init(guac_dbshell_line* line);

/**
 * Releases the storage of the given editing buffer.
 *
 * @param line
 *     The editing buffer to release.
 */
void guac_dbshell_line_destroy(guac_dbshell_line* line);

/**
 * Inserts the given bytes at the cursor position of the given editing
 * buffer, advancing the cursor past the inserted bytes.
 *
 * @param line
 *     The editing buffer to insert into.
 *
 * @param bytes
 *     The bytes to insert.
 *
 * @param length
 *     The number of bytes to insert.
 */
void guac_dbshell_line_insert(guac_dbshell_line* line, const char* bytes,
        int length);

/**
 * Removes the UTF-8 character immediately before the cursor of the given
 * editing buffer, if any, moving the cursor back accordingly.
 *
 * @param line
 *     The editing buffer to modify.
 */
void guac_dbshell_line_backspace(guac_dbshell_line* line);

/**
 * Removes the UTF-8 character at the cursor of the given editing buffer,
 * if any. The cursor does not move.
 *
 * @param line
 *     The editing buffer to modify.
 */
void guac_dbshell_line_delete(guac_dbshell_line* line);

/**
 * Moves the cursor of the given editing buffer one UTF-8 character to the
 * left, if possible.
 *
 * @param line
 *     The editing buffer whose cursor should move.
 */
void guac_dbshell_line_left(guac_dbshell_line* line);

/**
 * Moves the cursor of the given editing buffer one UTF-8 character to the
 * right, if possible.
 *
 * @param line
 *     The editing buffer whose cursor should move.
 */
void guac_dbshell_line_right(guac_dbshell_line* line);

/**
 * Removes all text before the cursor of the given editing buffer, moving
 * the cursor to the beginning of the line.
 *
 * @param line
 *     The editing buffer to modify.
 */
void guac_dbshell_line_kill_before(guac_dbshell_line* line);

/**
 * Removes all text at and after the cursor of the given editing buffer.
 *
 * @param line
 *     The editing buffer to modify.
 */
void guac_dbshell_line_kill_after(guac_dbshell_line* line);

/**
 * Removes the word immediately before the cursor of the given editing
 * buffer, along with any whitespace between that word and the cursor.
 *
 * @param line
 *     The editing buffer to modify.
 */
void guac_dbshell_line_kill_word(guac_dbshell_line* line);

/**
 * Replaces the entire content of the given editing buffer with the given
 * text, placing the cursor at the end of the line.
 *
 * @param line
 *     The editing buffer to modify.
 *
 * @param text
 *     The null-terminated text which should replace the current content.
 */
void guac_dbshell_line_set(guac_dbshell_line* line, const char* text);

/**
 * The display layout of a prompt and editing buffer within a terminal of a
 * given width, produced by guac_dbshell_line_layout().
 */
typedef struct guac_dbshell_layout {

    /**
     * The total number of display rows occupied by the prompt and text,
     * including the forced wrap row if the text ends exactly at the right
     * margin.
     */
    int rows;

    /**
     * The display row containing the cursor, relative to the first row of
     * the prompt (0-based).
     */
    int cursor_row;

    /**
     * The display column of the cursor within its row (0-based).
     */
    int cursor_col;

    /**
     * Whether the prompt and text end exactly at the right margin, in which
     * case rendering must force a wrap to the following row to avoid
     * ambiguity with the terminal's deferred wrapping behavior.
     */
    bool forced_wrap;

} guac_dbshell_layout;

/**
 * Returns the display width of the given UTF-8 string in terminal columns,
 * decoding UTF-8 directly and treating characters of indeterminate width as
 * occupying a single column.
 *
 * @param text
 *     The UTF-8 text to measure.
 *
 * @param length
 *     The number of bytes of the text to measure.
 *
 * @return
 *     The display width of the text, in columns.
 */
int guac_dbshell_display_width(const char* text, int length);

/**
 * Advances the given byte position past the single UTF-8 character
 * beginning at that position, returning the display width of that
 * character.
 *
 * @param text
 *     The UTF-8 text being traversed.
 *
 * @param length
 *     The number of bytes of the text.
 *
 * @param position
 *     The byte index of the character. On return, this will have advanced
 *     past the character.
 *
 * @return
 *     The display width of the character, in columns.
 */
int guac_dbshell_display_next(const char* text, int length, int* position);

/**
 * Computes the display layout of the given prompt and editing buffer within
 * a terminal having the given number of columns.
 *
 * @param prompt
 *     The null-terminated prompt string displayed before the text.
 *
 * @param line
 *     The editing buffer being displayed.
 *
 * @param cols
 *     The width of the terminal, in columns. Values less than one are
 *     treated as one.
 *
 * @param layout
 *     The structure to populate with the computed layout.
 */
void guac_dbshell_line_layout(const char* prompt,
        const guac_dbshell_line* line, int cols,
        guac_dbshell_layout* layout);

/**
 * Reads one logical line of input from the given terminal, providing
 * interactive editing and history navigation. The prompt is written to the
 * terminal before input is read and is redrawn as editing requires.
 *
 * @param term
 *     The terminal to read from and render to.
 *
 * @param parser
 *     The input parser to use. The parser must be owned by the caller and
 *     reused across successive reads of the same input stream, such that
 *     multi-byte sequences (like CRLF pairs) spanning two reads are handled
 *     correctly.
 *
 * @param history
 *     The history ring used for up/down navigation, or NULL if history
 *     navigation should be disabled. Submitted lines are NOT automatically
 *     added to the ring.
 *
 * @param prompt
 *     The null-terminated prompt string to display.
 *
 * @param status
 *     The structure to populate with the overall outcome of the read.
 *
 * @return
 *     A newly-allocated string containing the line read, which must
 *     eventually be freed with a call to guac_mem_free(), or NULL if the
 *     status is not GUAC_DBSHELL_READ_LINE.
 */
char* guac_dbshell_line_editor_read(guac_terminal* term,
        guac_dbshell_parser* parser, guac_dbshell_history* history,
        const char* prompt, guac_dbshell_read_status* status);

#endif
