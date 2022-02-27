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

#include "terminal/char_mappings.h"
#include "terminal/palette.h"
#include "terminal/terminal.h"
#include "terminal/terminal_handlers.h"
#include "terminal/ansi_escape_codes.h"
#include "terminal/types.h"
#include "terminal/xparsecolor.h"

#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>

#include <stdbool.h>
#include <stdlib.h>
#include <wchar.h>

//#define GUAC_TERMINAL_MAX_COLORS 3

/**
 * Response string sent when identification is requested.
 */
#define GUAC_TERMINAL_VT102_ID    "\x1B[?6c"

/**
 * Arbitrary response to ENQ control character.
 */
#define GUAC_TERMINAL_ANSWERBACK  "GUACAMOLE"

/**
 * Response which indicates the terminal is alive.
 */
#define GUAC_TERMINAL_OK          "\x1B[0n"

/**
 * Advances the cursor to the next row, scrolling if the cursor would otherwise
 * leave the scrolling region. If the cursor is already outside the scrolling
 * region, the cursor is prevented from leaving the terminal bounds.
 *
 * @param term
 *     The guac_terminal whose cursor should be advanced to the next row.
 */
static void guac_terminal_linefeed(guac_terminal* term) {

    /* Scroll up if necessary */
    if (term->cursor_row == term->scroll_end)
        guac_terminal_scroll_up(term, term->scroll_start,
                term->scroll_end, 1);

    /* Otherwise, just advance to next row if space remains */
    else if (term->cursor_row < term->term_height - 1)
        term->cursor_row++;

}

/**
 * Moves the cursor backward to the previous row, scrolling if the cursor would
 * otherwise leave the scrolling region. If the cursor is already outside the
 * scrolling region, the cursor is prevented from leaving the terminal bounds.
 *
 * @param term
 *     The guac_terminal whose cursor should be moved backward by one row.
 */
static void guac_terminal_reverse_linefeed(guac_terminal* term) {

    /* Scroll down if necessary */
    if (term->cursor_row == term->scroll_start)
        guac_terminal_scroll_down(term, term->scroll_start,
                term->scroll_end, 1);

    /* Otherwise, move back one row if space remains */
    else if (term->cursor_row > 0)
        term->cursor_row--;

}

/**
 * Sets the position of the cursor without exceeding terminal bounds. Values
 * which are out of bounds will be shifted to the nearest legal boundary.
 *
 * @param term
 *     The guac_terminal whose cursor position is being set.
 *
 * @param row
 *     The desired new row position.
 *
 * @param col
 *     The desired new column position.
 */
static void guac_terminal_move_cursor(guac_terminal* term, int row, int col) {

    /* Ensure cursor row is within terminal bounds */
    if (row >= term->term_height)
        row = term->term_height - 1;
    else if (row < 0)
        row = 0;

    /* Ensure cursor column is within terminal bounds */
    if (col >= term->term_width)
        col = term->term_width - 1;
    else if (col < 0)
        col = 0;

    /* Update cursor position */
    term->cursor_row = row;
    term->cursor_col = col;

}

int guac_terminal_echo(guac_terminal* term, unsigned char c) {

    int width;

    static int bytes_remaining = 0;
    static int codepoint = 0;

    const int* char_mapping = term->char_mapping[term->active_char_set];

    /* Echo to pipe stream if open and not starting an ESC sequence */
    if (term->pipe_stream != NULL && c != 0x1B) {

        guac_terminal_pipe_stream_write(term, c);

        /* Do not render output while pipe is open unless explicitly requested
         * via flags */
        if (!(term->pipe_stream_flags & GUAC_TERMINAL_PIPE_INTERPRET_OUTPUT))
            return 0;

    }

    /* If using non-Unicode mapping, just map straight bytes */
    if (char_mapping != NULL) {
        codepoint = c;
        bytes_remaining = 0;
    }

    /* 1-byte UTF-8 codepoint */
    else if ((c & 0x80) == 0x00) {    /* 0xxxxxxx */
        codepoint = c & 0x7F;
        bytes_remaining = 0;
    }

    /* 2-byte UTF-8 codepoint */
    else if ((c & 0xE0) == 0xC0) { /* 110xxxxx */
        codepoint = c & 0x1F;
        bytes_remaining = 1;
    }

    /* 3-byte UTF-8 codepoint */
    else if ((c & 0xF0) == 0xE0) { /* 1110xxxx */
        codepoint = c & 0x0F;
        bytes_remaining = 2;
    }

    /* 4-byte UTF-8 codepoint */
    else if ((c & 0xF8) == 0xF0) { /* 11110xxx */
        codepoint = c & 0x07;
        bytes_remaining = 3;
    }

    /* Continuation of UTF-8 codepoint */
    else if ((c & 0xC0) == 0x80) { /* 10xxxxxx */
        codepoint = (codepoint << 6) | (c & 0x3F);
        bytes_remaining--;
    }

    /* Unrecognized prefix */
    else {
        codepoint = '?';
        bytes_remaining = 0;
    }

    /* If we need more bytes, wait for more bytes */
    if (bytes_remaining != 0)
        return 0;

    switch (codepoint) {

        /* Enquiry */
        case 0x05:
            guac_terminal_send_string(term, GUAC_TERMINAL_ANSWERBACK);
            break;

        /* Bell */
        case 0x07:
            break;

        /* Backspace */
        case 0x08:
            guac_terminal_move_cursor(term, term->cursor_row,
                    term->cursor_col - 1);
            break;

        /* Tab */
        case 0x09:
            guac_terminal_move_cursor(term, term->cursor_row,
                    guac_terminal_next_tab(term, term->cursor_col));
            break;

        /* Line feed / VT / FF */
        case '\n':
        case 0x0B: /* VT */
        case 0x0C: /* FF */

            /* Advance to next row */
            guac_terminal_linefeed(term);

            /* If automatic carriage return, fall through to CR handler */
            if (!term->automatic_carriage_return)
                break;

        /* Carriage return */
        case '\r':
            guac_terminal_move_cursor(term, term->cursor_row, 0);
            break;

        /* SO (activates character set G1) */
        case 0x0E:
            term->active_char_set = 1;
            break;

        /* SI (activates character set G0) */
        case 0x0F:
            term->active_char_set = 0;
            break;

        /* ESC */
        case 0x1B:
            term->char_handler = guac_terminal_escape; 
            break;

        /* CSI */
        case 0x9B:
            term->char_handler = guac_terminal_csi; 
            break;

        /* DEL (ignored) */
        case 0x7F:
            break;

        /* Displayable chars */
        default:

            /* Don't bother handling control chars if unknown */
            if (codepoint < 0x20)
                break;

            /* Translate mappable codepoints to whatever codepoint is mapped */
            if (codepoint >= 0x20 && codepoint <= 0xFF && char_mapping != NULL)
                codepoint = char_mapping[codepoint - 0x20];

            /* Wrap if necessary */
            if (term->cursor_col >= term->term_width) {
                term->cursor_col = 0;
                guac_terminal_linefeed(term);
            }

            /* If insert mode, shift other characters right by 1 */
            if (term->insert_mode)
                guac_terminal_copy_columns(term, term->cursor_row,
                        term->cursor_col, term->term_width-2, 1);

            /* Write character */
            guac_terminal_set(term,
                    term->cursor_row,
                    term->cursor_col,
                    codepoint);

            width = wcwidth(codepoint);
            if (width < 0)
                width = 1;

            /* Advance cursor */
            term->cursor_col += width;

    }

    return 0;

}

int guac_terminal_escape(guac_terminal* term, unsigned char c) {

    switch (c) {

        case GUAC_TERMINAL_G0_CHARSET:
            term->char_handler = guac_terminal_g0_charset; 
            break;

        case GUAC_TERMINAL_G1_CHARSET:
            term->char_handler = guac_terminal_g1_charset; 
            break;

        case GUAC_TERMINAL_OSC:
            term->char_handler = guac_terminal_osc; 
            break;

        case GUAC_TERMINAL_CSI:
            term->char_handler = guac_terminal_csi; 
            break;

        case GUAC_TERMINAL_DEC:
            term->char_handler = guac_terminal_ctrl_func; 
            break;

        /* Save Cursor (DECSC) */
        case GUAC_TERMINAL_OSC_SAVE_CURSOR:
            term->saved_cursor_row = term->cursor_row;
            term->saved_cursor_col = term->cursor_col;

            term->char_handler = guac_terminal_echo; 
            break;

        /* Restore Cursor (DECRC) */
        case GUAC_TERMINAL_OSC_RESTORE_CURSOR:
            guac_terminal_move_cursor(term,
                    term->saved_cursor_row,
                    term->saved_cursor_col);

            term->char_handler = guac_terminal_echo; 
            break;

        /* Index (IND) */
        case GUAC_TERMINAL_LINEFEED:
            guac_terminal_linefeed(term);
            term->char_handler = guac_terminal_echo; 
            break;

        /* Next Line (NEL) */
        case GUAC_TERMINAL_NEWLINE:
            guac_terminal_move_cursor(term, term->cursor_row, 0);
            guac_terminal_linefeed(term);
            term->char_handler = guac_terminal_echo; 
            break;

        /* Set Tab (HTS) */
        case GUAC_TERMINAL_TABSET:
            guac_terminal_set_tab(term, term->cursor_col);
            term->char_handler = guac_terminal_echo; 
            break;

        /* Reverse Linefeed */
        case GUAC_TERMINAL_REVERSE_LINEFEED:
            guac_terminal_reverse_linefeed(term);
            term->char_handler = guac_terminal_echo; 
            break;

        /* DEC Identify */
        case GUAC_TERMINAL_PVT_ID:
            guac_terminal_send_string(term, GUAC_TERMINAL_VT102_ID);
            term->char_handler = guac_terminal_echo; 
            break;

        /* Reset */
        case GUAC_TERMINAL_RESET:
            guac_terminal_reset(term);
            break;

        case '_':
            term->char_handler = guac_terminal_apc;
            break;

        default:
            guac_client_log(term->client, GUAC_LOG_DEBUG,
                    "Unhandled ESC sequence: %c", c);
            term->char_handler = guac_terminal_echo; 

    }

    return 0;

}

/**
 * Given a character mapping specifier (such as B, 0, U, or K),
 * returns the corresponding character mapping.
 */
static const int* __guac_terminal_get_char_mapping(char c) {

    /* Translate character specifier to actual mapping */
    switch (c) {
        case GUAC_TERMINAL_DEFAULT_MAPPING: return NULL;
        case GUAC_TERMINAL_VT100: return vt100_map;
        case GUAC_TERMINAL_NULL: return null_map;
        case GUAC_TERMINAL_USER: return user_map;
    }

    /* Default to Unicode */
    return NULL;

}

int guac_terminal_g0_charset(guac_terminal* term, unsigned char c) {

    term->char_mapping[0] = __guac_terminal_get_char_mapping(c);
    term->char_handler = guac_terminal_echo; 
    return 0;

}

int guac_terminal_g1_charset(guac_terminal* term, unsigned char c) {

    term->char_mapping[1] = __guac_terminal_get_char_mapping(c);
    term->char_handler = guac_terminal_echo; 
    return 0;

}

/**
 * Looks up the flag specified by the given number and mode. Used by the Set/Reset Mode
 * functions of the terminal.
 */
static bool* __guac_terminal_get_flag(guac_terminal* term, int num, char private_mode) {

    if (private_mode == GUAC_TERMINAL_PVT_MODE) {
        switch (num) {
            case 1:  return &(term->application_cursor_keys); /* DECCKM */
            case 25: return &(term->cursor_visible); /* DECTECM */
        }
    }

    else if (private_mode == 0) {
        switch (num) {
            case GUAC_TERMINAL_CSI_INSERT_MODE:  return &(term->insert_mode); /* DECIM */
            case GUAC_TERMINAL_CSI_AUTOMATIC: return &(term->automatic_carriage_return); /* LF/NL */
        }
    }

    /* Unknown flag */
    return NULL;

}

/**
 * Parses an xterm SGR sequence specifying the RGB values of a color.
 *
 * @param argc
 *     The number of arguments within the argv array.
 *
 * @param argv
 *     The SGR arguments to parse, with the first relevant argument the
 *     red component of the RGB color.
 *
 * @param color
 *     The guac_terminal_color structure which should receive the parsed
 *     color values.
 *
 * @return
 *     The number of arguments parsed, or zero if argv does not contain
 *     enough elements to represent an RGB color.
 */
static int guac_terminal_parse_xterm256_rgb(int argc, const int* argv,
        guac_terminal_color* color) {

    /* RGB color palette entries require three arguments */
    if (argc < 3)
        return 0;

    /* Read RGB components from arguments */
    int red   = argv[0];
    int green = argv[1];
    int blue  = argv[2];

    /* Ignore if components are out of range */
    if (   red   < 0 || red   > GUAC_TERMINAL_MAX_COLOR_RANGE
        || green < 0 || green > GUAC_TERMINAL_MAX_COLOR_RANGE
        || blue  < 0 || blue  > GUAC_TERMINAL_MAX_COLOR_RANGE)
        return 3;

    /* Store RGB components */
    color->red   = (uint8_t) red;
    color->green = (uint8_t) green;
    color->blue  = (uint8_t) blue;

    /* Color is not from the palette */
    color->palette_index = -1;

    /* Done */
    return 3;

}

/**
 * Parses an xterm SGR sequence specifying the index of a color within the
 * 256-color palette.
 *
 * @param terminal
 *     The terminal associated with the palette.
 *
 * @param argc
 *     The number of arguments within the argv array.
 *
 * @param argv
 *     The SGR arguments to parse, with the first relevant argument being
 *     the index of the color.
 *
 * @param color
 *     The guac_terminal_color structure which should receive the parsed
 *     color values.
 *
 * @return
 *     The number of arguments parsed, or zero if the palette index is
 *     absent.
 */
static int guac_terminal_parse_xterm256_index(guac_terminal* terminal,
        int argc, const int* argv, guac_terminal_color* color) {

    /* 256-color palette entries require only one argument */
    if (argc < 1)
        return 0;

    /* Copy palette entry */
    guac_terminal_display_lookup_color(terminal->display, argv[0], color);

    /* Done */
    return 1;

}

/**
 * Parses an xterm SGR sequence specifying the index of a color within the
 * 256-color palette, or specfying the RGB values of a color. The number of
 * arguments required by these sequences varies. If a 256-color sequence is
 * recognized, the number of arguments parsed is returned.
 *
 * @param terminal
 *     The terminal whose palette state should be used when parsing the xterm
 *     256-color SGR sequence.
 *
 * @param argc
 *     The number of arguments within the argv array.
 *
 * @param argv
 *     The SGR arguments to parse, with the first relevant argument being
 *     the first element of the array. In the case of an xterm 256-color
 *     SGR sequence, the first element here will be either 2, for an
 *     RGB color, or 5, for a 256-color palette index. All other values
 *     are invalid and will not be parsed.
 *
 * @param color
 *     The guac_terminal_color structure which should receive the parsed
 *     color values.
 *
 * @return
 *     The number of arguments parsed, or zero if argv does not point to
 *     the first element of an xterm 256-color SGR sequence.
 */
static int guac_terminal_parse_xterm256(guac_terminal* terminal,
        int argc, const int* argv, guac_terminal_color* color) {

    /* All 256-color codes must have at least a type */
    if (argc < 1)
        return 0;

    switch (argv[0]) {

        /* RGB */
        case 2:
            return guac_terminal_parse_xterm256_rgb(
                    argc - 1, &argv[1], color) + 1;

        /* Palette index */
        case 5:
            return guac_terminal_parse_xterm256_index(terminal,
                    argc - 1, &argv[1], color) + 1;

    }

    /* Invalid type */
    return 0;

}

int guac_terminal_csi(guac_terminal* term, unsigned char c) {

    /* CSI function arguments */
    static int argc = 0;
    static int argv[16] = {0};

    /* Sequence prefix, if any */
    static char private_mode_character = 0;

    /* Argument building counter and buffer */
    static int argv_length = 0;
    static char argv_buffer[256];

    /* Digits get concatenated into argv */
    if (c >= '0' && c <= '9') {

        /* Concatenate digit if there is space in buffer */
        if (argv_length < sizeof(argv_buffer)-1)
            argv_buffer[argv_length++] = c;

    }

    /* Specific non-digits stop the parameter, and possibly the sequence */
    else if ((c >= 0x40 && c <= 0x7E) || c == ';') {

        int i, row, col, amount;
        bool* flag;

        /* At most 16 parameters */
        if (argc < GUAC_TERMINAL_CSI_MAX_ARGUMENTS) {

            /* Finish parameter */
            argv_buffer[argv_length] = 0;
            argv[argc++] = atoi(argv_buffer);

            /* Prepare for next parameter */
            argv_length = 0;

        }

        /* Handle CSI functions */ 
        switch (c) {

            /* @: Insert characters (scroll right) */
            case GUAC_TERMINAL_CSI_BLANK_CHARS:

                amount = argv[0];
                if (amount == 0) amount = 1;

                /* Scroll right by amount */
                if (term->cursor_col + amount < term->term_width)
                    guac_terminal_copy_columns(term, term->cursor_row,
                            term->cursor_col, term->term_width - amount - 1,
                            amount);

                /* Clear left */
                guac_terminal_clear_columns(term, term->cursor_row,
                        term->cursor_col, term->cursor_col + amount - 1);

                break;

            /* A: Move up */
            case GUAC_TERMINAL_CSI_CURSOR_UP:

                /* Get move amount */
                amount = argv[0];
                if (amount == 0) amount = 1;

                /* Move cursor */
                guac_terminal_move_cursor(term,
                        term->cursor_row - amount,
                        term->cursor_col);

                break;

            /* B: Move down */
            case GUAC_TERMINAL_CSI_ROW_DOWN:
            case GUAC_TERMINAL_CSI_CURSOR_DOWN:

                /* Get move amount */
                amount = argv[0];
                if (amount == 0) amount = 1;

                /* Move cursor */
                guac_terminal_move_cursor(term,
                        term->cursor_row + amount,
                        term->cursor_col);

                break;

            /* C: Move right */
            case GUAC_TERMINAL_CSI_COLUMN_RIGHT:
            case GUAC_TERMINAL_CSI_CURSOR_RIGHT:

                /* Get move amount */
                amount = argv[0];
                if (amount == 0) amount = 1;

                /* Move cursor */
                guac_terminal_move_cursor(term,
                        term->cursor_row,
                        term->cursor_col + amount);

                break;

            /* D: Move left */
            case GUAC_TERMINAL_CSI_CURSOR_LEFT:

                /* Get move amount */
                amount = argv[0];
                if (amount == 0) amount = 1;

                /* Move cursor */
                guac_terminal_move_cursor(term,
                        term->cursor_row,
                        term->cursor_col - amount);

                break;

            /* E: Move cursor down given number rows, column 1 */
            case GUAC_TERMINAL_CSI_CURSOR_DOWN_COL_ONE:

                /* Get move amount */
                amount = argv[0];
                if (amount == 0) amount = 1;

                /* Move cursor down, reset to column 1 */
                guac_terminal_move_cursor(term,
                        term->cursor_row + amount,
                        0);

                break;

            /* F: Move cursor up given number rows, column 1 */
            case GUAC_TERMINAL_CSI_CURSOR_UP_COL_ONE:

                /* Get move amount */
                amount = argv[0];
                if (amount == 0) amount = 1;

                /* Move cursor up , reset to column 1 */
                guac_terminal_move_cursor(term,
                        term->cursor_row - amount,
                        0);

                break;

            /* G: Move cursor, current row */
            case GUAC_TERMINAL_CSI_MOVE_HORIZONTAL:
            case GUAC_TERMINAL_CSI_MOVE_TO_COLUMN:
                col = argv[0]; if (col != 0) col--;
                guac_terminal_move_cursor(term, term->cursor_row, col);
                break;

            /* H: Move cursor */
            case GUAC_TERMINAL_CSI_MOVE_CURSOR_ANYWHERE:
            case GUAC_TERMINAL_CSI_MOVE_CURSOR:

                row = argv[0]; if (row != 0) row--;
                col = argv[1]; if (col != 0) col--;

                guac_terminal_move_cursor(term, row, col);
                break;

            /* J: Erase display */
            case GUAC_TERMINAL_CSI_ERASE_DISPLAY:
 
                /* Erase from cursor to end of display */
                if (argv[0] == 0)
                    guac_terminal_clear_range(term,
                            term->cursor_row, term->cursor_col,
                            term->term_height-1, term->term_width-1);
                
                /* Erase from start to cursor */
                else if (argv[0] == GUAC_TERMINAL_ERASE_DISPLAY_UPTO)
                    guac_terminal_clear_range(term,
                            0, 0,
                            term->cursor_row, term->cursor_col);

                /* Entire screen */
                else if (argv[0] == GUAC_TERMINAL_ERASE_DISPLAY || argv[0] == GUAC_TERMINAL_ERASE_SCROLLBACK)
                    guac_terminal_clear_range(term,
                            0, 0, term->term_height - 1, term->term_width - 1);

                break;

            /* K: Erase line */
            case GUAC_TERMINAL_CSI_ERASE_LINE:

                /* Erase from cursor to end of line */
                if (argv[0] == 0)
                    guac_terminal_clear_columns(term, term->cursor_row,
                            term->cursor_col, term->term_width - 1);

                /* Erase from start to cursor */
                else if (argv[0] == GUAC_TERMINAL_ERASE_LINE_UPTO)
                    guac_terminal_clear_columns(term, term->cursor_row,
                            0, term->cursor_col);

                /* Erase line */
                else if (argv[0] == GUAC_TERMINAL_ERASE_WHOLE_LINE)
                    guac_terminal_clear_columns(term, term->cursor_row,
                            0, term->term_width - 1);

                break;

            /* L: Insert blank lines (scroll down) */
            case GUAC_TERMINAL_CSI_INSERT_LINES:

                amount = argv[0];
                if (amount == 0) amount = 1;

                guac_terminal_scroll_down(term,
                        term->cursor_row, term->scroll_end, amount);

                break;

            /* M: Delete lines (scroll up) */
            case GUAC_TERMINAL_CSI_DELETE_LINES:

                amount = argv[0];
                if (amount == 0) amount = 1;

                guac_terminal_scroll_up(term,
                        term->cursor_row, term->scroll_end, amount);

                break;

            /* P: Delete characters (scroll left) */
            case GUAC_TERMINAL_CSI_DELETE_CHARS:

                amount = argv[0];
                if (amount == 0) amount = 1;

                /* Scroll left by amount */
                if (term->cursor_col + amount < term->term_width)
                    guac_terminal_copy_columns(term, term->cursor_row,
                            term->cursor_col + amount, term->term_width - 1,
                            -amount);

                /* Clear right */
                guac_terminal_clear_columns(term, term->cursor_row,
                        term->term_width - amount, term->term_width - 1);

                break;

            /* X: Erase characters (no scroll) */
            case GUAC_TERMINAL_CSI_ERASE_CHARS:

                amount = argv[0];
                if (amount == 0) amount = 1;

                /* Clear characters */
                guac_terminal_clear_columns(term, term->cursor_row,
                        term->cursor_col, term->cursor_col + amount - 1);

                break;

            /* ]: Linux Private CSI */
            case GUAC_TERMINAL_LINUX_PRIVATE_CSI:
                /* Explicitly ignored */
                break;

            /* c: Identify */
            case GUAC_TERMINAL_CSI_DISPLAY_ANSWER:
                if (argv[0] == 0 && private_mode_character == 0)
                    guac_terminal_send_string(term, GUAC_TERMINAL_VT102_ID);
                break;

            /* d: Move cursor, current col */
            case GUAC_TERMINAL_CSI_CHANGE_ROW:
                row = argv[0]; if (row != 0) row--;
                guac_terminal_move_cursor(term, row, term->cursor_col);
                break;

            /* g: Clear tab */
            case GUAC_TERMINAL_CSI_CLEAR_TABSTOP:

                /* Clear tab at current location */
                if (argv[0] == 0)
                    guac_terminal_unset_tab(term, term->cursor_col);

                /* Clear all tabs */
                else if (argv[0] == GUAC_TERMINAL_CLEAR_ALL_TABSTOPS)
                    guac_terminal_clear_tabs(term);

                break;

            /* h: Set Mode */
            case GUAC_TERMINAL_CSI_SET_MODE:
             
                /* Look up flag and set */ 
                flag = __guac_terminal_get_flag(term, argv[0], private_mode_character);
                if (flag != NULL)
                    *flag = true;

                break;

            /* l: Reset Mode */
            case GUAC_TERMINAL_CSI_RESET_MODE:
              
                /* Look up flag and clear */ 
                flag = __guac_terminal_get_flag(term, argv[0], private_mode_character);
                if (flag != NULL)
                    *flag = false;

                break;

            /* m: Set graphics rendition */
            case GUAC_TERMINAL_CSI_GRAPHICS_RENDITION:

                for (i=0; i<argc; i++) {

                    int value = argv[i];

                    /* Reset attributes */
                    if (value == 0)
                        term->current_attributes = term->default_char.attributes;

                    /* Bold */
                    else if (value == GUAC_TERMINAL_CSI_BOLD_TEXT)
                        term->current_attributes.bold = true;

                    /* Faint (low intensity) */
                    else if (value == GUAC_TERMINAL_FAINT)
                        term->current_attributes.half_bright = true;

                    /* Underscore on */
                    else if (value == GUAC_TERMINAL_UNDERLINE_ON)
                        term->current_attributes.underscore = true;

                    /* Reverse video */
                    else if (value == GUAC_TERMINAL_REVERSE_VIDEO)
                        term->current_attributes.reverse = true;

                    /* Normal intensity (not bold) */
                    else if (value == GUAC_TERMINAL_DOUBLY_UNDERLINED || 
                            value == GUAC_TERMINAL_NORMAL_INTENSITY) {
                        term->current_attributes.bold = false;
                        term->current_attributes.half_bright = false;
                    }

                    /* Reset underscore */
                    else if (value == GUAC_TERMINAL_UNDERLINE_OFF)
                        term->current_attributes.underscore = false;

                    /* Reset reverse video */
                    else if (value == GUAC_TERMINAL_REVERSE_VIDEO_OFF)
                        term->current_attributes.reverse = false;

                    /* Foreground */
                    else if (value >= GUAC_TERMINAL_BLACK_FOREGROUND && 
                            value <= GUAC_TERMINAL_WHITE_FOREGROUND)
                        guac_terminal_display_lookup_color(term->display,
                                value - 30,
                                &term->current_attributes.foreground);

                    /* Underscore on, default foreground OR 256-color
                     * foreground */
                    else if (value == GUAC_TERMINAL_DEF_FOREGROUND_UNDERSCORE_ON) {

                        /* Attempt to set foreground with 256-color entry */
                        int xterm256_length =
                            guac_terminal_parse_xterm256(term,
                                    argc - i - 1, &argv[i + 1],
                                    &term->current_attributes.foreground);

                        /* If valid 256-color entry, foreground has been set */
                        if (xterm256_length > 0)
                            i += xterm256_length;

                        /* Otherwise interpret as underscore and default
                         * foreground  */
                        else {
                            term->current_attributes.underscore = true;
                            term->current_attributes.foreground =
                                term->default_char.attributes.foreground;
                        }

                    }

                    /* Underscore off, default foreground */
                    else if (value == GUAC_TERMINAL_DEF_FOREGROUND_UNDERSCORE_OFF) {
                        term->current_attributes.underscore = false;
                        term->current_attributes.foreground =
                            term->default_char.attributes.foreground;
                    }

                    /* Background */
                    else if (value >= GUAC_TERMINAL_BLACK_BACKGROUND && 
                            value <= GUAC_TERMINAL_WHITE_BACKGROUND)
                        guac_terminal_display_lookup_color(term->display,
                                value - 40,
                                &term->current_attributes.background);

                    /* 256-color background */
                    else if (value == GUAC_TERMINAL_SET_BACKGROUND)
                        i += guac_terminal_parse_xterm256(term,
                                argc - i - 1, &argv[i + 1],
                                &term->current_attributes.background);

                    /* Reset background */
                    else if (value == GUAC_TERMINAL_DEFAULT_BACKGROUND)
                        term->current_attributes.background =
                            term->default_char.attributes.background;

                    /* Intense foreground */
                    else if (value >= GUAC_TERMINAL_BRIGHT_FOREGROUND_LOW && 
                            value <= GUAC_TERMINAL_BRIGHT_FOREGROUND_HIGH)
                        guac_terminal_display_lookup_color(term->display,
                                value - 90 + GUAC_TERMINAL_FIRST_INTENSE,
                                &term->current_attributes.foreground);

                    /* Intense background */
                    else if (value >= GUAC_TERMINAL_BRIGHT_BACKGROUND_LOW && 
                                value <= GUAC_TERMINAL_BRIGHT_BACKGROUND_HIGH)
                        guac_terminal_display_lookup_color(term->display,
                                value - 100 + GUAC_TERMINAL_FIRST_INTENSE,
                                &term->current_attributes.background);

                }

                break;

            /* n: Status report */
            case GUAC_TERMINAL_CSI_STATUS_REPORT:

                /* Device status report */
                if (argv[0] == GUAC_TERMINAL_DEVICE_STATUS_REPORT && 
                        private_mode_character == 0)
                    guac_terminal_send_string(term, GUAC_TERMINAL_OK);

                /* Cursor position report */
                else if (argv[0] == GUAC_TERMINAL_CURSOR_POSITION_REPORT && 
                            private_mode_character == 0)
                    guac_terminal_sendf(term, "\x1B[%i;%iR", term->cursor_row+1, 
                        term->cursor_col+1);

                break;

            /* q: Set keyboard LEDs */
            case GUAC_TERMINAL_CSI_KEYBOARD_LED:
                /* Explicitly ignored */
                break;

            /* r: Set scrolling region */
            case GUAC_TERMINAL_CSI_SET_SCROLLING_REGION:

                /* If parameters given, set region */
                if (argc == GUAC_TERMINAL_SCROLLING_REGION_PARAMS) {
                    term->scroll_start = argv[0]-1;
                    term->scroll_end   = argv[1]-1;
                }

                /* Otherwise, reset scrolling region */
                else {
                    term->scroll_start = 0;
                    term->scroll_end = term->term_height - 1;
                }

                break;

            /* Save Cursor */
            case GUAC_TERMINAL_SAVE_CURSOR_LOCATION:
                term->saved_cursor_row = term->cursor_row;
                term->saved_cursor_col = term->cursor_col;
                break;

            /* Restore Cursor */
            case GUAC_TERMINAL_RESTORE_CURSOR_LOCATION:
                guac_terminal_move_cursor(term,
                        term->saved_cursor_row,
                        term->saved_cursor_col);
                break;

            /* Warn of unhandled codes */
            default:
                if (c != ';') {

                    guac_client_log(term->client, GUAC_LOG_DEBUG,
                            "Unhandled CSI sequence: %c", c);

                    for (i=0; i<argc; i++)
                        guac_client_log(term->client, GUAC_LOG_DEBUG,
                                " -> argv[%i] = %i", i, argv[i]);

                }

        }

        /* If not a semicolon, end of CSI sequence */
        if (c != ';') {
            term->char_handler = guac_terminal_echo;

            /* Reset parameters */
            for (i=0; i<argc; i++)
                argv[i] = 0;

            /* Reset private mode character */
            private_mode_character = 0;

            /* Reset argument counters */
            argc = 0;
            argv_length = 0;
        }

    }

    /* Set private mode character if given and unset */
    else if (c >= 0x3A && c <= 0x3F && private_mode_character == 0)
        private_mode_character = c;

    return 0;

}

int guac_terminal_set_directory(guac_terminal* term, unsigned char c) {

    static char filename[2048];
    static int length = 0;

    /* Stop on ECMA-48 ST (String Terminator */
    if (c == 0x9C || c == 0x5C || c == 0x07) {
        filename[length++] = '\0';
        term->char_handler = guac_terminal_echo;
        if (term->upload_path_handler)
            term->upload_path_handler(term->client, filename);
        else
            guac_client_log(term->client, GUAC_LOG_DEBUG,
                    "Cannot set upload path. File transfer is not enabled.");
        length = 0;
    }

    /* Otherwise, store character */
    else if (length < sizeof(filename)-1)
        filename[length++] = c;

    return 0;

}

int guac_terminal_download(guac_terminal* term, unsigned char c) {

    static char filename[2048];
    static int length = 0;

    /* Stop on ECMA-48 ST (String Terminator */
    if (c == 0x9C || c == 0x5C || c == 0x07) {
        filename[length++] = '\0';
        term->char_handler = guac_terminal_echo;
        if (term->file_download_handler)
            term->file_download_handler(term->client, filename);
        else
            guac_client_log(term->client, GUAC_LOG_DEBUG,
                    "Cannot send file. File transfer is not enabled.");
        length = 0;
    }

    /* Otherwise, store character */
    else if (length < sizeof(filename)-1)
        filename[length++] = c;

    return 0;

}

int guac_terminal_open_pipe_stream(guac_terminal* term, unsigned char c) {

    static char param[2048];
    static int length = 0;
    static int flags = 0;

    /* Open pipe on ECMA-48 ST (String Terminator) */
    if (c == 0x9C || c == 0x5C || c == 0x07) {

        /* End parameter string */
        param[length++] = '\0';
        length = 0;

        /* Open new pipe stream using final parameter as name */
        guac_terminal_pipe_stream_open(term, param, flags);

        /* Return to echo mode */
        term->char_handler = guac_terminal_echo;

        /* Reset tracked flags for sake of any future pipe streams */
        flags = 0;

    }

    /* Interpret all parameters prior to the final parameter as integer
     * flags which should affect the pipe stream when opened */
    else if (c == ';') {

        /* End parameter string */
        param[length++] = '\0';
        length = 0;

        /* Parse parameter string as integer flags */
        flags |= atoi(param);

    }

    /* Otherwise, store character within parameter */
    else if (length < sizeof(param)-1)
        param[length++] = c;

    return 0;

}

int guac_terminal_close_pipe_stream(guac_terminal* term, unsigned char c) {

    /* Handle closure on ECMA-48 ST (String Terminator) */
    if (c == 0x9C || c == 0x5C || c == 0x07) {

        /* Close any existing pipe */
        guac_terminal_pipe_stream_close(term);

        /* Return to echo mode */
        term->char_handler = guac_terminal_echo;

    }

    /* Ignore all other characters */
    return 0;

}

int guac_terminal_set_scrollback(guac_terminal* term, unsigned char c) {

    static char param[16];
    static int length = 0;

    /* Open pipe on ECMA-48 ST (String Terminator) */
    if (c == 0x9C || c == 0x5C || c == 0x07) {

        /* End parameter string */
        param[length++] = '\0';
        length = 0;

        /* Assign scrollback size */
        term->requested_scrollback = atoi(param);

        /* Update scrollbar bounds */
        guac_terminal_scrollbar_set_bounds(term->scrollbar,
                -guac_terminal_available_scroll(term), 0);

        /* Return to echo mode */
        term->char_handler = guac_terminal_echo;

    }

    /* Otherwise, store character within parameter */
    else if (length < sizeof(param) - 1)
        param[length++] = c;

    return 0;

}


int guac_terminal_window_title(guac_terminal* term, unsigned char c) {

    static int position = 0;
    static char title[4096];

    guac_socket* socket = term->client->socket;

    /* Stop on ECMA-48 ST (String Terminator */
    if (c == 0x9C || c == 0x5C || c == 0x07) {

        /* Terminate and reset stored title */
        title[position] = '\0';
        position = 0;

        /* Send title as connection name */
        guac_protocol_send_name(socket, title);
        guac_socket_flush(socket);

        term->char_handler = guac_terminal_echo;

    }

    /* Store all other characters within title, space permitting */
    else if (position < sizeof(title) - 1)
        title[position++] = (char) c;

    return 0;

}

int guac_terminal_xterm_palette(guac_terminal* term, unsigned char c) {

    /**
     * Whether we are currently reading the color spec. If false, we are
     * currently reading the color index.
     */
    static bool read_color_spec = false;

    /**
     * The index of the palette entry being modified.
     */
    static int index = 0;

    /**
     * The color spec string, valid only if read_color_spec is true.
     */
    static char color_spec[256];

    /**
     * The current position within the color spec string, valid only if
     * read_color_spec is true.
     */
    static int color_spec_pos = 0;

    /* If not reading the color spec, parse the index */
    if (!read_color_spec) {

        /* If digit, append to index */
        if (c >= '0' && c <= '9')
            index = index * 10 + c - '0';

        /* If end of parameter, switch to reading the color */
        else if (c == ';') {
            read_color_spec = true;
            color_spec_pos = 0;
        }

    }

    /* Once the index has been parsed, read the color spec */
    else {

        /* Modify palette once index/spec pair has been read */
        if (c == ';' || c == 0x9C || c == 0x5C || c == 0x07) {

            guac_terminal_color color;

            /* Terminate color spec string */
            color_spec[color_spec_pos] = '\0';

            /* Modify palette if color spec is valid */
            if (!guac_terminal_xparsecolor(color_spec, &color))
                guac_terminal_display_assign_color(term->display,
                        index, &color);
            else
                guac_client_log(term->client, GUAC_LOG_DEBUG,
                        "Invalid XParseColor() color spec: \"%s\"",
                        color_spec);

            /* Resume parsing index */
            read_color_spec = false;
            index = 0;

        }

        /* Append characters to color spec as long as available space is not
         * exceeded */
        else if (color_spec_pos < GUAC_TERMINAL_MAX_COLOR_RANGE) {
            color_spec[color_spec_pos++] = c;
        }

    }

    /* Stop on ECMA-48 ST (String Terminator */
    if (c == 0x9C || c == 0x5C || c == 0x07)
        term->char_handler = guac_terminal_echo;

    return 0;

}

int guac_terminal_osc(guac_terminal* term, unsigned char c) {

    static int operation = 0;

    /* If digit, append to operation */
    if (c >= '0' && c <= '9')
        operation = operation * 10 + c - '0';

    /* If end of parameter, check value */
    else if (c == ';') {

        /* Download OSC */
        if (operation == GUAC_TERMINAL_DOWNLOAD_INIT)
            term->char_handler = guac_terminal_download;

        /* Set upload directory OSC */
        else if (operation == GUAC_TERMINAL_UPLOAD_DIRECTORY)
            term->char_handler = guac_terminal_set_directory;

        /* Open and redirect output to pipe stream OSC */
        else if (operation == GUAC_TERMINAL_PIPESTREAM_REDIRECT)
            term->char_handler = guac_terminal_open_pipe_stream;

        /* Close pipe stream OSC */
        else if (operation == GUAC_TERMINAL_TERMINAL_REDIRECT)
            term->char_handler = guac_terminal_close_pipe_stream;

        /* Set scrollback size OSC */
        else if (operation == GUAC_TERMINAL_RESIZE_SCROLLBACK)
            term->char_handler = guac_terminal_set_scrollback;

        /* Set window title OSC */
        else if (operation == 0 || operation == GUAC_TERMINAL_SET_WINDOW_TITLE)
            term->char_handler = guac_terminal_window_title;

        /* xterm 256-color palette redefinition */
        else if (operation == GUAC_TERMINAL_SET_COLOR)
            term->char_handler = guac_terminal_xterm_palette;

        /* Reset parameter for next OSC */
        operation = 0;

    }

    /* Stop on ECMA-48 ST (String Terminator */
    else if (c == 0x9C || c == 0x5C || c == 0x07)
        term->char_handler = guac_terminal_echo;

    /* Stop on unrecognized character */
    else {
        guac_client_log(term->client, GUAC_LOG_DEBUG,
                "Unexpected character in OSC: 0x%X", c);
        term->char_handler = guac_terminal_echo;
    }

    return 0;
}

int guac_terminal_ctrl_func(guac_terminal* term, unsigned char c) {

    int row;

    /* Build character with current attributes */
    guac_terminal_char guac_char;
    guac_char.value = 'E';
    guac_char.attributes = term->current_attributes;

    switch (c) {

        /* Alignment test (fill screen with E's) */
        case GUAC_TERMINAL_ALIGNMENT_TEST:

            for (row=0; row<term->term_height; row++)
                guac_terminal_set_columns(term, row, 0, term->term_width-1, &guac_char);

            break;

    }

    term->char_handler = guac_terminal_echo; 

    return 0;

}

int guac_terminal_apc(guac_terminal* term, unsigned char c) {

    /* xterm does not implement APC functions and neither do we. Look for the
     * "ESC \" (string terminator) sequence, while ignoring other chars. */
    static bool escaping = false;

    if (escaping) {
        if (c == '\\')
            term->char_handler = guac_terminal_echo;
        escaping = false;
    }

    if (c == 0x1B)
        escaping = true;
    return 0;
}
