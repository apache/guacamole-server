
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

#include "common.h"
#include "terminal.h"
#include "terminal_handlers.h"

int guac_terminal_echo(guac_terminal* term, char c) {

    static int bytes_remaining = 0;
    static int codepoint = 0;

    /* 1-byte UTF-8 codepoint */
    if ((c & 0x80) == 0x00) {    /* 0xxxxxxx */
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

        /* Bell */
        case 0x07:
            break;

        /* Backspace */
        case 0x08:
            if (term->cursor_col >= 1)
                term->cursor_col--;
            break;

        /* Carriage return */
        case '\r':
            term->cursor_col = 0;
            break;

        /* Line feed / VT / FF */
        case '\n':
        case 0x0B: /* VT */
        case 0x0C: /* FF */
            term->cursor_row++;

            /* Scroll up if necessary */
            if (term->cursor_row > term->scroll_end) {
                term->cursor_row = term->scroll_end;

                /* Scroll up by one row */        
                guac_terminal_scroll_up(term, term->scroll_start,
                        term->scroll_end, 1);

            }
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

            /* Wrap if necessary */
            if (term->cursor_col >= term->term_width) {
                term->cursor_col = 0;
                term->cursor_row++;
            }

            /* Scroll up if necessary */
            if (term->cursor_row > term->scroll_end) {
                term->cursor_row = term->scroll_end;

                /* Scroll up by one row */        
                guac_terminal_scroll_up(term, term->scroll_start,
                        term->scroll_end, 1);

            }

            /* Write character */
            guac_terminal_set(term,
                    term->cursor_row,
                    term->cursor_col,
                    codepoint);

            /* Advance cursor */
            term->cursor_col++;

    }

    return 0;

}

int guac_terminal_escape(guac_terminal* term, char c) {

    switch (c) {

        case '(':
            term->char_handler = guac_terminal_g0_charset; 
            break;

        case ')':
            term->char_handler = guac_terminal_g1_charset; 
            break;

        case '*':
            term->char_handler = guac_terminal_g2_charset; 
            break;

        case '+':
            term->char_handler = guac_terminal_g3_charset; 
            break;

        case ']':
            term->char_handler = guac_terminal_osc; 
            break;

        case '[':
            term->char_handler = guac_terminal_csi; 
            break;

        case '#':
            term->char_handler = guac_terminal_ctrl_func; 
            break;

        /* Save Cursor (DECSC) */
        case '7':
            term->saved_cursor_row = term->cursor_row;
            term->saved_cursor_col = term->cursor_col;

            term->char_handler = guac_terminal_echo; 
            break;

        /* Restore Cursor (DECRC) */
        case '8':

            term->cursor_row = term->saved_cursor_row;
            if (term->cursor_row >= term->term_height)
                term->cursor_row = term->term_height - 1;

            term->cursor_col = term->saved_cursor_col;
            if (term->cursor_col >= term->term_width)
                term->cursor_col = term->term_width - 1;

            term->char_handler = guac_terminal_echo; 
            break;

        /* Index (IND) */
        case 'D':
            term->cursor_row++;

            /* Scroll up if necessary */
            if (term->cursor_row > term->scroll_end) {
                term->cursor_row = term->scroll_end;

                /* Scroll up by one row */        
                guac_terminal_scroll_up(term, term->scroll_start,
                        term->scroll_end, 1);

            }

            term->char_handler = guac_terminal_echo; 
            break;

        /* Next Line (NEL) */
        case 'E':
            term->cursor_col = 0;
            term->cursor_row++;

            /* Scroll up if necessary */
            if (term->cursor_row > term->scroll_end) {
                term->cursor_row = term->scroll_end;

                /* Scroll up by one row */        
                guac_terminal_scroll_up(term, term->scroll_start,
                        term->scroll_end, 1);

            }

            term->char_handler = guac_terminal_echo; 
            break;

        /* Reverse Linefeed */
        case 'M':

            term->cursor_row--;

            /* Scroll down if necessary */
            if (term->cursor_row < term->scroll_start) {
                term->cursor_row = term->scroll_start;

                /* Scroll down by one row */        
                guac_terminal_scroll_down(term, term->scroll_start,
                        term->scroll_end, 1);

            }

            term->char_handler = guac_terminal_echo; 
            break;

        default:
            guac_client_log_info(term->client, "Unhandled ESC sequence: %c", c);
            term->char_handler = guac_terminal_echo; 

    }

    return 0;

}

int guac_terminal_g0_charset(guac_terminal* term, char c) {

    /* STUB */
    guac_client_log_info(term->client, "Ignoring G0 charset: 0x%02x", c);
    term->char_handler = guac_terminal_echo; 
    return 0;

}

int guac_terminal_g1_charset(guac_terminal* term, char c) {

    /* STUB */
    guac_client_log_info(term->client, "Ignoring G1 charset: 0x%02x", c);
    term->char_handler = guac_terminal_echo; 
    return 0;

}

int guac_terminal_g2_charset(guac_terminal* term, char c) {

    /* STUB */
    guac_client_log_info(term->client, "Ignoring G2 charset: 0x%02x", c);
    term->char_handler = guac_terminal_echo; 
    return 0;

}

int guac_terminal_g3_charset(guac_terminal* term, char c) {

    /* STUB */
    guac_client_log_info(term->client, "Ignoring G3 charset: 0x%02x", c);
    term->char_handler = guac_terminal_echo; 
    return 0;

}

/**
 * Looks up the flag specified by the given number and mode. Used by the Set/Reset Mode
 * functions of the terminal.
 */
static bool* __guac_terminal_get_flag(guac_terminal* term, int num, char private_mode) {

    if (private_mode == '?') {
        switch (num) {
            case 1:  return &(term->application_cursor_keys); /* DECCKM */
        }
    }


    /* Unknown flag */
    return NULL;

}

int guac_terminal_csi(guac_terminal* term, char c) {

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
        if (argc < 16) {

            /* Finish parameter */
            argv_buffer[argv_length] = 0;
            argv[argc++] = atoi(argv_buffer);

            /* Prepare for next parameter */
            argv_length = 0;

        }

        /* Handle CSI functions */ 
        switch (c) {

            /* @: Insert characters (scroll right) */
            case '@':

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
            case 'A':

                /* Get move amount */
                amount = argv[0];
                if (amount == 0) amount = 1;

                /* Move cursor */
                term->cursor_row -= amount;
                if (term->cursor_row < 0)
                    term->cursor_row = 0;

                break;

            /* B: Move down */
            case 'e':
            case 'B':

                /* Get move amount */
                amount = argv[0];
                if (amount == 0) amount = 1;

                /* Move cursor */
                term->cursor_row += amount;
                if (term->cursor_row >= term->term_height)
                    term->cursor_row = term->term_height - 1;

                break;

            /* C: Move right */
            case 'a':
            case 'C':

                /* Get move amount */
                amount = argv[0];
                if (amount == 0) amount = 1;

                /* Move cursor */
                term->cursor_col += amount;
                if (term->cursor_col >= term->term_width)
                    term->cursor_col = term->term_width - 1;

                break;

            /* D: Move left */
            case 'D':

                /* Get move amount */
                amount = argv[0];
                if (amount == 0) amount = 1;

                /* Move cursor */
                term->cursor_col -= amount;
                if (term->cursor_col < 0)
                    term->cursor_col = 0;

                break;

            /* E: Move cursor down given number rows, column 1 */
            case 'E':

                /* Get move amount */
                amount = argv[0];
                if (amount == 0) amount = 1;

                /* Move cursor */
                term->cursor_row += amount;
                if (term->cursor_row >= term->term_height)
                    term->cursor_row = term->term_height - 1;

                /* Reset to column 1 */
                term->cursor_col = 0;

                break;

            /* F: Move cursor up given number rows, column 1 */
            case 'F':

                /* Get move amount */
                amount = argv[0];
                if (amount == 0) amount = 1;

                /* Move cursor */
                term->cursor_row -= amount;
                if (term->cursor_row < 0)
                    term->cursor_row = 0;

                /* Reset to column 1 */
                term->cursor_col = 0;

                break;

            /* G: Move cursor, current row */
            case 'G':
                col = argv[0]; if (col != 0) col--;
                term->cursor_col = col;
                break;

            /* H: Move cursor */
            case 'f':
            case 'H':

                row = argv[0]; if (row != 0) row--;
                col = argv[1]; if (col != 0) col--;

                term->cursor_row = row;
                term->cursor_col = col;
                break;

            /* J: Erase display */
            case 'J':
 
                /* Erase from cursor to end of display */
                if (argv[0] == 0)
                    guac_terminal_clear_range(term,
                            term->cursor_row, term->cursor_col,
                            term->term_height-1, term->term_width-1);
                
                /* Erase from start to cursor */
                else if (argv[0] == 1)
                    guac_terminal_clear_range(term,
                            0, 0,
                            term->cursor_row, term->cursor_col);

                /* Entire screen */
                else if (argv[0] == 2)
                    guac_terminal_clear_range(term,
                            0, 0, term->term_height - 1, term->term_width - 1);

                break;

            /* K: Erase line */
            case 'K':

                /* Erase from cursor to end of line */
                if (argv[0] == 0)
                    guac_terminal_clear_columns(term, term->cursor_row,
                            term->cursor_col, term->term_width - 1);

                /* Erase from start to cursor */
                else if (argv[0] == 1)
                    guac_terminal_clear_columns(term, term->cursor_row,
                            0, term->cursor_col);

                /* Erase line */
                else if (argv[0] == 2)
                    guac_terminal_clear_columns(term, term->cursor_row,
                            0, term->term_width - 1);

                break;

            /* L: Insert blank lines (scroll down) */
            case 'L':

                amount = argv[0];
                if (amount == 0) amount = 1;

                guac_terminal_scroll_down(term,
                        term->cursor_row, term->scroll_end, amount);

                break;

            /* M: Delete lines (scroll up) */
            case 'M':

                amount = argv[0];
                if (amount == 0) amount = 1;

                guac_terminal_scroll_up(term,
                        term->cursor_row, term->scroll_end, amount);

                break;

            /* P: Delete characters (scroll left) */
            case 'P':

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
            case 'X':

                amount = argv[0];
                if (amount == 0) amount = 1;

                /* Clear characters */
                guac_terminal_clear_columns(term, term->cursor_row,
                        term->cursor_col, term->cursor_col + amount - 1);

                break;

            /* c: Identify */
            case 'c':
                guac_terminal_write_all(term->stdin_pipe_fd[1], "\x1B[?6c", 5);
                break;

            /* d: Move cursor, current col */
            case 'd':
                row = argv[0]; if (row != 0) row--;
                term->cursor_row = row;
                break;

            /* h: Set Mode */
            case 'h':
             
                /* Look up flag and set */ 
                flag = __guac_terminal_get_flag(term, argv[0], private_mode_character);
                if (flag != NULL)
                    *flag = true;

                else
                    guac_client_log_info(term->client,
                            "Unhandled mode set: mode=%i, private_mode_character=0x%0x",
                            argv[0], private_mode_character);

                break;

            /* l: Reset Mode */
            case 'l':
              
                /* Look up flag and clear */ 
                flag = __guac_terminal_get_flag(term, argv[0], private_mode_character);
                if (flag != NULL)
                    *flag = false;

                else
                    guac_client_log_info(term->client,
                            "Unhandled mode reset: mode=%i, private_mode_character=0x%0x",
                            argv[0], private_mode_character);

                break;

            /* m: Set graphics rendition */
            case 'm':

                for (i=0; i<argc; i++) {

                    int value = argv[i];

                    /* Reset attributes */
                    if (value == 0)
                        term->current_attributes = term->default_char.attributes;

                    /* Bold */
                    else if (value == 1)
                        term->current_attributes.bold = true;

                    /* Underscore on */
                    else if (value == 4)
                        term->current_attributes.underscore = true;

                    /* Foreground */
                    else if (value >= 30 && value <= 37)
                        term->current_attributes.foreground = value - 30;

                    /* Background */
                    else if (value >= 40 && value <= 47)
                        term->current_attributes.background = value - 40;

                    /* Underscore on, default foreground */
                    else if (value == 38) {
                        term->current_attributes.underscore = true;
                        term->current_attributes.foreground =
                            term->default_char.attributes.foreground;
                    }

                    /* Underscore off, default foreground */
                    else if (value == 39) {
                        term->current_attributes.underscore = false;
                        term->current_attributes.foreground =
                            term->default_char.attributes.foreground;
                    }

                    /* Reset background */
                    else if (value == 49)
                        term->current_attributes.background =
                            term->default_char.attributes.background;

                    /* Reverse video */
                    else if (value == 7)
                        term->current_attributes.reverse = true;

                    /* Reset underscore */
                    else if (value == 24)
                        term->current_attributes.underscore = false;

                    /* Reset reverse video */
                    else if (value == 27)
                        term->current_attributes.reverse = false;

                    else
                        guac_client_log_info(term->client,
                                "Unhandled graphics rendition: %i", value);

                }

                break;

            /* r: Set scrolling region */
            case 'r':

                /* If parameters given, set region */
                if (argc == 2) {
                    term->scroll_start = argv[0]-1;
                    term->scroll_end   = argv[1]-1;
                }

                /* Otherwise, reset scrolling region */
                else {
                    term->scroll_start = 0;
                    term->scroll_end = term->term_height - 1;
                }

                break;

            /* Warn of unhandled codes */
            default:
                if (c != ';') {

                    guac_client_log_info(term->client,
                            "Unhandled CSI sequence: %c", c);

                    for (i=0; i<argc; i++)
                        guac_client_log_info(term->client,
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

int guac_terminal_osc(guac_terminal* term, char c) {
    /* TODO: Implement OSC */
    if (c == 0x9C || c == 0x5C || c == 0x07) /* ECMA-48 ST (String Terminator */
       term->char_handler = guac_terminal_echo; 
    return 0;
}

int guac_terminal_ctrl_func(guac_terminal* term, char c) {

    int row;

    /* Build character with current attributes */
    guac_terminal_char guac_char;
    guac_char.value = 'E';
    guac_char.attributes = term->current_attributes;

    switch (c) {

        /* Alignment test (fill screen with E's) */
        case '8':

            for (row=0; row<term->term_height; row++)
                guac_terminal_set_columns(term, row, 0, term->term_width-1, &guac_char);

            break;

    }

    term->char_handler = guac_terminal_echo; 

    return 0;

}

