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
#include "dbshell/history.h"
#include "dbshell/line-editor.h"

#include <guacamole/mem.h>
#include <guacamole/string.h>
#include <terminal/terminal.h>

#include <ctype.h>
#include <stdbool.h>
#include <string.h>
#include <wchar.h>

/**
 * The initial number of bytes allocated for the editing buffer.
 */
#define GUAC_DBSHELL_LINE_INITIAL_SIZE 256

void guac_dbshell_parser_init(guac_dbshell_parser* parser) {
    memset(parser, 0, sizeof(guac_dbshell_parser));
    parser->state = GUAC_DBSHELL_PARSER_GROUND;
}

/**
 * Interprets the final byte of a CSI sequence together with any
 * accumulated parameter bytes, returning the corresponding editing action.
 *
 * @param parser
 *     The parser whose csi_buffer contains the accumulated parameter
 *     bytes.
 *
 * @param final
 *     The final byte of the CSI sequence.
 *
 * @return
 *     The editing action denoted by the sequence, or
 *     GUAC_DBSHELL_KEY_IGNORED if the sequence has no editing meaning.
 */
static guac_dbshell_key guac_dbshell_parser_csi(guac_dbshell_parser* parser,
        char final) {

    switch (final) {

        /* Arrow keys */
        case 'A': return GUAC_DBSHELL_KEY_UP;
        case 'B': return GUAC_DBSHELL_KEY_DOWN;
        case 'C': return GUAC_DBSHELL_KEY_RIGHT;
        case 'D': return GUAC_DBSHELL_KEY_LEFT;

        /* Home/End */
        case 'H': return GUAC_DBSHELL_KEY_HOME;
        case 'F': return GUAC_DBSHELL_KEY_END;

        /* Sequences of the form "ESC [ n ~" */
        case '~':

            if (strcmp(parser->csi_buffer, "1") == 0
                    || strcmp(parser->csi_buffer, "7") == 0)
                return GUAC_DBSHELL_KEY_HOME;

            if (strcmp(parser->csi_buffer, "4") == 0
                    || strcmp(parser->csi_buffer, "8") == 0)
                return GUAC_DBSHELL_KEY_END;

            if (strcmp(parser->csi_buffer, "3") == 0)
                return GUAC_DBSHELL_KEY_DELETE;

            return GUAC_DBSHELL_KEY_IGNORED;

        /* All other sequences are deliberately ignored */
        default:
            return GUAC_DBSHELL_KEY_IGNORED;

    }

}

guac_dbshell_key guac_dbshell_parser_feed(guac_dbshell_parser* parser,
        char byte) {

    unsigned char c = (unsigned char) byte;

    /* Track CR solely for absorbing the LF of CRLF pairs */
    bool last_was_cr = parser->last_was_cr;
    parser->last_was_cr = false;

    switch (parser->state) {

        case GUAC_DBSHELL_PARSER_GROUND:

            /* Plain printable ASCII */
            if (c >= 0x20 && c != 0x7F && c < 0x80) {
                parser->char_buffer[0] = byte;
                parser->char_length = 1;
                return GUAC_DBSHELL_KEY_CHAR;
            }

            /* Leading byte of a multi-byte UTF-8 character */
            if (c >= 0xC2 && c <= 0xF4) {

                parser->char_buffer[0] = byte;
                parser->char_length = 1;

                if (c >= 0xF0)
                    parser->utf8_remaining = 3;
                else if (c >= 0xE0)
                    parser->utf8_remaining = 2;
                else
                    parser->utf8_remaining = 1;

                parser->state = GUAC_DBSHELL_PARSER_UTF8;
                return GUAC_DBSHELL_KEY_NONE;

            }

            /* Control characters */
            switch (c) {

                case 0x0D: /* CR */
                    parser->last_was_cr = true;
                    return GUAC_DBSHELL_KEY_ENTER;

                case 0x0A: /* LF (absorbed if it completes a CRLF pair) */
                    if (last_was_cr)
                        return GUAC_DBSHELL_KEY_NONE;
                    return GUAC_DBSHELL_KEY_ENTER;

                case 0x7F: /* DEL */
                case 0x08: /* BS */
                    return GUAC_DBSHELL_KEY_BACKSPACE;

                case 0x1B: /* ESC */
                    parser->state = GUAC_DBSHELL_PARSER_ESC;
                    return GUAC_DBSHELL_KEY_NONE;

                case 0x01: /* Ctrl+A */
                    return GUAC_DBSHELL_KEY_HOME;

                case 0x05: /* Ctrl+E */
                    return GUAC_DBSHELL_KEY_END;

                case 0x03: /* Ctrl+C */
                    return GUAC_DBSHELL_KEY_INTERRUPT;

                case 0x04: /* Ctrl+D */
                    return GUAC_DBSHELL_KEY_EOF;

                case 0x0C: /* Ctrl+L */
                    return GUAC_DBSHELL_KEY_CLEAR;

                case 0x15: /* Ctrl+U */
                    return GUAC_DBSHELL_KEY_KILL_LINE;

                case 0x0B: /* Ctrl+K */
                    return GUAC_DBSHELL_KEY_KILL_TO_END;

                case 0x17: /* Ctrl+W */
                    return GUAC_DBSHELL_KEY_KILL_WORD;

                case 0x09: /* Tab */
                    return GUAC_DBSHELL_KEY_TAB;

                default:
                    return GUAC_DBSHELL_KEY_IGNORED;

            }

        case GUAC_DBSHELL_PARSER_ESC:

            if (byte == '[') {
                parser->state = GUAC_DBSHELL_PARSER_CSI;
                parser->csi_length = 0;
                parser->csi_buffer[0] = '\0';
                return GUAC_DBSHELL_KEY_NONE;
            }

            if (byte == 'O') {
                parser->state = GUAC_DBSHELL_PARSER_SS3;
                return GUAC_DBSHELL_KEY_NONE;
            }

            /* Any other byte following ESC (Alt+key combinations, etc.) is
             * consumed and ignored */
            parser->state = GUAC_DBSHELL_PARSER_GROUND;
            return GUAC_DBSHELL_KEY_IGNORED;

        case GUAC_DBSHELL_PARSER_CSI:

            /* Parameter and intermediate bytes */
            if (c >= 0x20 && c <= 0x3F) {

                if (parser->csi_length < GUAC_DBSHELL_PARSER_CSI_LENGTH - 1) {
                    parser->csi_buffer[parser->csi_length++] = byte;
                    parser->csi_buffer[parser->csi_length] = '\0';
                }

                return GUAC_DBSHELL_KEY_NONE;

            }

            /* Final byte */
            parser->state = GUAC_DBSHELL_PARSER_GROUND;
            if (c >= 0x40 && c <= 0x7E)
                return guac_dbshell_parser_csi(parser, byte);

            /* Malformed sequence */
            return GUAC_DBSHELL_KEY_IGNORED;

        case GUAC_DBSHELL_PARSER_SS3:

            parser->state = GUAC_DBSHELL_PARSER_GROUND;

            switch (byte) {
                case 'A': return GUAC_DBSHELL_KEY_UP;
                case 'B': return GUAC_DBSHELL_KEY_DOWN;
                case 'C': return GUAC_DBSHELL_KEY_RIGHT;
                case 'D': return GUAC_DBSHELL_KEY_LEFT;
                case 'H': return GUAC_DBSHELL_KEY_HOME;
                case 'F': return GUAC_DBSHELL_KEY_END;
                default:  return GUAC_DBSHELL_KEY_IGNORED;
            }

        case GUAC_DBSHELL_PARSER_UTF8:

            /* Valid continuation byte */
            if (c >= 0x80 && c <= 0xBF) {

                parser->char_buffer[parser->char_length++] = byte;

                if (--parser->utf8_remaining == 0) {
                    parser->state = GUAC_DBSHELL_PARSER_GROUND;
                    return GUAC_DBSHELL_KEY_CHAR;
                }

                return GUAC_DBSHELL_KEY_NONE;

            }

            /* Invalid byte within UTF-8 character */
            parser->state = GUAC_DBSHELL_PARSER_GROUND;
            return GUAC_DBSHELL_KEY_IGNORED;

    }

    return GUAC_DBSHELL_KEY_IGNORED;

}

void guac_dbshell_line_init(guac_dbshell_line* line) {
    line->buffer = guac_mem_alloc(GUAC_DBSHELL_LINE_INITIAL_SIZE);
    line->buffer[0] = '\0';
    line->length = 0;
    line->allocated = GUAC_DBSHELL_LINE_INITIAL_SIZE;
    line->cursor = 0;
}

void guac_dbshell_line_destroy(guac_dbshell_line* line) {
    guac_mem_free(line->buffer);
    line->buffer = NULL;
}

/**
 * Returns the number of bytes of the UTF-8 character beginning at the given
 * byte, as determined from its leading byte.
 *
 * @param c
 *     The leading byte of the character.
 *
 * @return
 *     The number of bytes of the character, between 1 and 4 inclusive.
 */
static int guac_dbshell_char_length(unsigned char c) {

    if (c >= 0xF0)
        return 4;

    if (c >= 0xE0)
        return 3;

    if (c >= 0xC0)
        return 2;

    return 1;

}

/**
 * Returns the byte index of the beginning of the UTF-8 character preceding
 * the given position within the given buffer.
 *
 * @param buffer
 *     The buffer containing UTF-8 text.
 *
 * @param position
 *     The byte index to search backwards from.
 *
 * @return
 *     The byte index of the beginning of the preceding character, or zero
 *     if there is no preceding character.
 */
static int guac_dbshell_prev_char(const char* buffer, int position) {

    if (position <= 0)
        return 0;

    /* Skip backwards over continuation bytes */
    position--;
    while (position > 0
            && ((unsigned char) buffer[position] & 0xC0) == 0x80)
        position--;

    return position;

}

void guac_dbshell_line_insert(guac_dbshell_line* line, const char* bytes,
        int length) {

    if (length <= 0)
        return;

    /* Expand buffer as necessary */
    int required = line->length + length + 1;
    if (required > line->allocated) {

        int allocated = line->allocated;
        while (allocated < required)
            allocated = guac_mem_ckd_mul_or_die(allocated, 2);

        line->buffer = guac_mem_realloc(line->buffer, allocated);
        line->allocated = allocated;

    }

    /* Shift text after the cursor and insert */
    memmove(line->buffer + line->cursor + length,
            line->buffer + line->cursor, line->length - line->cursor);
    memcpy(line->buffer + line->cursor, bytes, length);

    line->length += length;
    line->cursor += length;
    line->buffer[line->length] = '\0';

}

void guac_dbshell_line_backspace(guac_dbshell_line* line) {

    if (line->cursor == 0)
        return;

    int start = guac_dbshell_prev_char(line->buffer, line->cursor);
    memmove(line->buffer + start, line->buffer + line->cursor,
            line->length - line->cursor);

    line->length -= line->cursor - start;
    line->cursor = start;
    line->buffer[line->length] = '\0';

}

void guac_dbshell_line_delete(guac_dbshell_line* line) {

    if (line->cursor >= line->length)
        return;

    int char_length = guac_dbshell_char_length(
            (unsigned char) line->buffer[line->cursor]);

    /* Never remove beyond the end of the buffer */
    if (line->cursor + char_length > line->length)
        char_length = line->length - line->cursor;

    memmove(line->buffer + line->cursor,
            line->buffer + line->cursor + char_length,
            line->length - line->cursor - char_length);

    line->length -= char_length;
    line->buffer[line->length] = '\0';

}

void guac_dbshell_line_left(guac_dbshell_line* line) {
    line->cursor = guac_dbshell_prev_char(line->buffer, line->cursor);
}

void guac_dbshell_line_right(guac_dbshell_line* line) {

    if (line->cursor >= line->length)
        return;

    int char_length = guac_dbshell_char_length(
            (unsigned char) line->buffer[line->cursor]);

    line->cursor += char_length;
    if (line->cursor > line->length)
        line->cursor = line->length;

}

void guac_dbshell_line_kill_before(guac_dbshell_line* line) {

    memmove(line->buffer, line->buffer + line->cursor,
            line->length - line->cursor);

    line->length -= line->cursor;
    line->cursor = 0;
    line->buffer[line->length] = '\0';

}

void guac_dbshell_line_kill_after(guac_dbshell_line* line) {
    line->length = line->cursor;
    line->buffer[line->length] = '\0';
}

void guac_dbshell_line_kill_word(guac_dbshell_line* line) {

    int start = line->cursor;

    /* Skip whitespace immediately before the cursor */
    while (start > 0
            && isspace((unsigned char) line->buffer[start - 1]))
        start--;

    /* Skip the word itself */
    while (start > 0
            && !isspace((unsigned char) line->buffer[start - 1]))
        start--;

    memmove(line->buffer + start, line->buffer + line->cursor,
            line->length - line->cursor);

    line->length -= line->cursor - start;
    line->cursor = start;
    line->buffer[line->length] = '\0';

}

void guac_dbshell_line_set(guac_dbshell_line* line, const char* text) {

    line->length = 0;
    line->cursor = 0;
    line->buffer[0] = '\0';

    guac_dbshell_line_insert(line, text, strlen(text));

}

/**
 * Decodes the UTF-8 character beginning at the given byte position,
 * returning its codepoint and advancing the position past the character.
 * Invalid bytes are consumed individually and returned as the replacement
 * value of their raw byte.
 *
 * @param text
 *     The UTF-8 text to decode from.
 *
 * @param length
 *     The number of bytes of text.
 *
 * @param position
 *     The byte index of the character to decode. On return, this will have
 *     advanced past the decoded character.
 *
 * @return
 *     The decoded codepoint.
 */
static int guac_dbshell_decode_utf8(const char* text, int length,
        int* position) {

    unsigned char c = (unsigned char) text[*position];
    int remaining;
    int codepoint;

    if (c < 0x80) {
        (*position)++;
        return c;
    }

    if (c >= 0xF0) {
        codepoint = c & 0x07;
        remaining = 3;
    }
    else if (c >= 0xE0) {
        codepoint = c & 0x0F;
        remaining = 2;
    }
    else if (c >= 0xC0) {
        codepoint = c & 0x1F;
        remaining = 1;
    }
    else {
        /* Unexpected continuation byte */
        (*position)++;
        return c;
    }

    (*position)++;
    while (remaining > 0 && *position < length) {

        unsigned char continuation = (unsigned char) text[*position];
        if ((continuation & 0xC0) != 0x80)
            break;

        codepoint = (codepoint << 6) | (continuation & 0x3F);
        (*position)++;
        remaining--;

    }

    return codepoint;

}

/**
 * Returns the display width of the given codepoint in terminal columns,
 * treating codepoints of indeterminate width as occupying a single column.
 *
 * @param codepoint
 *     The codepoint to measure.
 *
 * @return
 *     The display width of the codepoint, in columns.
 */
static int guac_dbshell_codepoint_width(int codepoint) {

    int width = wcwidth((wchar_t) codepoint);
    if (width < 0)
        width = 1;

    return width;

}

int guac_dbshell_display_width(const char* text, int length) {

    int width = 0;
    int position = 0;

    while (position < length)
        width += guac_dbshell_codepoint_width(
                guac_dbshell_decode_utf8(text, length, &position));

    return width;

}

int guac_dbshell_display_next(const char* text, int length, int* position) {
    return guac_dbshell_codepoint_width(
            guac_dbshell_decode_utf8(text, length, position));
}

/**
 * The display position reached after writing a sequence of characters,
 * used while simulating terminal layout.
 */
typedef struct guac_dbshell_position {

    /**
     * The display row, relative to the first row of the prompt (0-based).
     */
    int row;

    /**
     * The display column within the row (0-based). This may equal the
     * terminal width when the last character written ended exactly at the
     * right margin.
     */
    int col;

} guac_dbshell_position;

/**
 * Advances the given display position by a character of the given width,
 * wrapping to the following row if the character would not fit within the
 * current row. Characters wider than the terminal never fit and occupy a
 * row by themselves.
 *
 * @param position
 *     The display position to advance.
 *
 * @param width
 *     The display width of the character, in columns.
 *
 * @param cols
 *     The width of the terminal, in columns.
 */
static void guac_dbshell_position_advance(guac_dbshell_position* position,
        int width, int cols) {

    /* Wrap to the following row if the character does not fit */
    if (position->col + width > cols) {
        position->row++;
        position->col = 0;
    }

    position->col += width;

}

void guac_dbshell_line_layout(const char* prompt,
        const guac_dbshell_line* line, int cols,
        guac_dbshell_layout* layout) {

    if (cols < 1)
        cols = 1;

    guac_dbshell_position position = { 0, 0 };
    guac_dbshell_position cursor = { 0, 0 };

    /* Lay out the prompt (assumed to contain no multi-byte characters) */
    int prompt_length = strlen(prompt);
    for (int i = 0; i < prompt_length; i++)
        guac_dbshell_position_advance(&position, 1, cols);

    /* Cursor cannot precede the text */
    cursor = position;

    /* Lay out the text, capturing the position at the cursor */
    int byte = 0;
    while (byte < line->length) {

        if (byte == line->cursor)
            cursor = position;

        int width = guac_dbshell_codepoint_width(
                guac_dbshell_decode_utf8(line->buffer, line->length, &byte));

        guac_dbshell_position_advance(&position, width, cols);

    }

    if (line->cursor >= line->length)
        cursor = position;

    /* A position at the exact right margin is displayed at the beginning
     * of the following row */
    layout->forced_wrap = (position.col >= cols);
    if (position.col >= cols) {
        position.row++;
        position.col = 0;
    }

    if (cursor.col >= cols) {
        cursor.row++;
        cursor.col = 0;
    }

    layout->rows = position.row + 1;
    layout->cursor_row = cursor.row;
    layout->cursor_col = cursor.col;

}

/**
 * The full state of an in-progress line read, tying together the editing
 * buffer, the layout most recently rendered, and history navigation.
 */
typedef struct guac_dbshell_editor {

    /**
     * The terminal being read from and rendered to.
     */
    guac_terminal* term;

    /**
     * The prompt displayed before the text.
     */
    const char* prompt;

    /**
     * The editing buffer.
     */
    guac_dbshell_line line;

    /**
     * The layout of the most recent redraw, used to locate and clear the
     * previously-rendered rows.
     */
    guac_dbshell_layout rendered;

    /**
     * Whether a redraw has occurred and the rendered layout is valid.
     */
    bool have_rendered;

    /**
     * The history ring used for up/down navigation, or NULL if history
     * navigation is disabled.
     */
    guac_dbshell_history* history;

    /**
     * The current offset within the history ring, where zero denotes the
     * line being edited and one denotes the newest history entry.
     */
    int history_offset;

    /**
     * The content of the line being edited at the time history navigation
     * began, restored when navigating back to offset zero, or NULL if no
     * content is stashed.
     */
    char* stash;

} guac_dbshell_editor;

/**
 * Redraws the prompt and editing buffer of the given editor, clearing all
 * previously-rendered rows and leaving the terminal cursor at the editing
 * cursor position.
 *
 * @param editor
 *     The editor to redraw.
 */
static void guac_dbshell_editor_redraw(guac_dbshell_editor* editor) {

    guac_dbshell_buffer output;
    guac_dbshell_buffer_init(&output);

    int cols = guac_terminal_get_columns(editor->term);

    /* Clear all previously-rendered rows, from the bottom row upwards */
    if (editor->have_rendered) {

        int below = editor->rendered.rows - 1 - editor->rendered.cursor_row;
        if (below > 0)
            guac_dbshell_buffer_appendf(&output, "\x1B[%iB", below);

        for (int i = editor->rendered.rows - 1; i > 0; i--)
            guac_dbshell_buffer_append(&output, "\r\x1B[K\x1B[A", 7);

    }

    guac_dbshell_buffer_append(&output, "\r\x1B[K", 4);

    /* Render prompt and text */
    guac_dbshell_buffer_append_string(&output, editor->prompt);
    guac_dbshell_buffer_append(&output, editor->line.buffer,
            editor->line.length);

    /* Compute the new layout, forcing the final wrap if the text ends
     * exactly at the right margin */
    guac_dbshell_layout layout;
    guac_dbshell_line_layout(editor->prompt, &editor->line, cols, &layout);

    if (layout.forced_wrap)
        guac_dbshell_buffer_append(&output, "\r\n", 2);

    /* Move from the end of the text to the cursor position */
    int up = (layout.rows - 1) - layout.cursor_row;
    if (up > 0)
        guac_dbshell_buffer_appendf(&output, "\x1B[%iA", up);

    guac_dbshell_buffer_append(&output, "\r", 1);
    if (layout.cursor_col > 0)
        guac_dbshell_buffer_appendf(&output, "\x1B[%iC", layout.cursor_col);

    guac_terminal_write(editor->term, output.data, output.length);
    guac_dbshell_buffer_destroy(&output);

    editor->rendered = layout;
    editor->have_rendered = true;

}

/**
 * Moves the terminal cursor of the given editor from its current position
 * to the row following the final row of the rendered text, in preparation
 * for output which follows the completed line.
 *
 * @param editor
 *     The editor whose rendering is being finished.
 */
static void guac_dbshell_editor_finish(guac_dbshell_editor* editor) {

    guac_dbshell_buffer output;
    guac_dbshell_buffer_init(&output);

    if (editor->have_rendered) {
        int below = editor->rendered.rows - 1 - editor->rendered.cursor_row;
        if (below > 0)
            guac_dbshell_buffer_appendf(&output, "\x1B[%iB", below);
    }

    guac_dbshell_buffer_append(&output, "\r\n", 2);

    guac_terminal_write(editor->term, output.data, output.length);
    guac_dbshell_buffer_destroy(&output);

}

/**
 * Replaces the content of the given editor's editing buffer according to
 * the given step through history, stashing or restoring the in-progress
 * line as appropriate.
 *
 * @param editor
 *     The editor navigating through history.
 *
 * @param direction
 *     The direction of navigation: positive to move to older entries,
 *     negative to move to newer entries.
 */
static void guac_dbshell_editor_history(guac_dbshell_editor* editor,
        int direction) {

    if (editor->history == NULL)
        return;

    int offset = editor->history_offset + direction;

    /* Clamp navigation to the available entries */
    if (offset < 0 || offset > editor->history->length)
        return;

    /* Stash the in-progress line upon first navigating away from it */
    if (editor->history_offset == 0 && offset > 0) {
        guac_mem_free(editor->stash);
        editor->stash = guac_strdup(editor->line.buffer);
    }

    if (offset == 0) {

        /* Restore the stashed in-progress line */
        guac_dbshell_line_set(&editor->line,
                editor->stash != NULL ? editor->stash : "");

    }

    else {

        const char* entry = guac_dbshell_history_get(editor->history,
                offset);
        if (entry == NULL)
            return;

        guac_dbshell_line_set(&editor->line, entry);

    }

    editor->history_offset = offset;

}

char* guac_dbshell_line_editor_read(guac_terminal* term,
        guac_dbshell_parser* parser, guac_dbshell_history* history,
        const char* prompt, guac_dbshell_read_status* status) {

    guac_dbshell_editor editor = {
        .term = term,
        .prompt = prompt,
        .history = history
    };

    guac_dbshell_line_init(&editor.line);
    guac_dbshell_editor_redraw(&editor);

    char* result = NULL;
    *status = GUAC_DBSHELL_READ_CLOSED;

    /* Spaces inserted in place of a literal tab character */
    static const char tab_spaces[] = "    ";

    char byte;
    bool done = false;

    while (!done && guac_terminal_read_stdin(term, &byte, 1) == 1) {

        bool modified = false;

        switch (guac_dbshell_parser_feed(parser, byte)) {

            case GUAC_DBSHELL_KEY_CHAR:
                guac_dbshell_line_insert(&editor.line, parser->char_buffer,
                        parser->char_length);
                modified = true;
                break;

            case GUAC_DBSHELL_KEY_TAB:
                guac_dbshell_line_insert(&editor.line, tab_spaces,
                        sizeof(tab_spaces) - 1);
                modified = true;
                break;

            case GUAC_DBSHELL_KEY_ENTER:
                guac_dbshell_editor_finish(&editor);
                result = guac_strdup(editor.line.buffer);
                *status = GUAC_DBSHELL_READ_LINE;
                done = true;
                break;

            case GUAC_DBSHELL_KEY_BACKSPACE:
                guac_dbshell_line_backspace(&editor.line);
                modified = true;
                break;

            case GUAC_DBSHELL_KEY_DELETE:
                guac_dbshell_line_delete(&editor.line);
                modified = true;
                break;

            case GUAC_DBSHELL_KEY_LEFT:
                guac_dbshell_line_left(&editor.line);
                modified = true;
                break;

            case GUAC_DBSHELL_KEY_RIGHT:
                guac_dbshell_line_right(&editor.line);
                modified = true;
                break;

            case GUAC_DBSHELL_KEY_UP:
                guac_dbshell_editor_history(&editor, 1);
                modified = true;
                break;

            case GUAC_DBSHELL_KEY_DOWN:
                guac_dbshell_editor_history(&editor, -1);
                modified = true;
                break;

            case GUAC_DBSHELL_KEY_HOME:
                editor.line.cursor = 0;
                modified = true;
                break;

            case GUAC_DBSHELL_KEY_END:
                editor.line.cursor = editor.line.length;
                modified = true;
                break;

            case GUAC_DBSHELL_KEY_INTERRUPT:
                guac_dbshell_editor_finish(&editor);
                guac_terminal_printf(term, "^C\r\n");
                *status = GUAC_DBSHELL_READ_CANCELLED;
                done = true;
                break;

            case GUAC_DBSHELL_KEY_EOF:

                /* Ctrl+D on an empty line ends input; on a non-empty line
                 * it deletes forward, as in readline */
                if (editor.line.length == 0) {
                    guac_dbshell_editor_finish(&editor);
                    *status = GUAC_DBSHELL_READ_CLOSED;
                    done = true;
                }
                else {
                    guac_dbshell_line_delete(&editor.line);
                    modified = true;
                }

                break;

            case GUAC_DBSHELL_KEY_CLEAR:
                guac_terminal_write(term, "\x1B[H\x1B[2J", 7);
                editor.have_rendered = false;
                modified = true;
                break;

            case GUAC_DBSHELL_KEY_KILL_LINE:
                guac_dbshell_line_kill_before(&editor.line);
                modified = true;
                break;

            case GUAC_DBSHELL_KEY_KILL_TO_END:
                guac_dbshell_line_kill_after(&editor.line);
                modified = true;
                break;

            case GUAC_DBSHELL_KEY_KILL_WORD:
                guac_dbshell_line_kill_word(&editor.line);
                modified = true;
                break;

            case GUAC_DBSHELL_KEY_NONE:
            case GUAC_DBSHELL_KEY_IGNORED:
                break;

        }

        if (modified)
            guac_dbshell_editor_redraw(&editor);

    }

    guac_dbshell_line_destroy(&editor.line);
    guac_mem_free(editor.stash);

    return result;

}
