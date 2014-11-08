/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"

#include "char_mappings.h"
#include "terminal.h"
#include "terminal_handlers.h"
#include "types.h"

#include <guacamole/client.h>

#include <stdbool.h>
#include <stdlib.h>
#include <wchar.h>

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

int guac_terminal_echo(guac_terminal* term, unsigned char c) {

    int width;

    static int bytes_remaining = 0;
    static int codepoint = 0;

    const int* char_mapping = term->char_mapping[term->active_char_set];

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
            if (term->cursor_col >= 1)
                term->cursor_col--;
            break;

        /* Tab */
        case 0x09:
            term->cursor_col = guac_terminal_next_tab(term, term->cursor_col);
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

            /* If automatic carriage return, fall through to CR handler */
            if (!term->automatic_carriage_return)
                break;

        /* Carriage return */
        case '\r':
            term->cursor_col = 0;
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
                term->cursor_row++;
            }

            /* Scroll up if necessary */
            if (term->cursor_row > term->scroll_end) {
                term->cursor_row = term->scroll_end;

                /* Scroll up by one row */        
                guac_terminal_scroll_up(term, term->scroll_start,
                        term->scroll_end, 1);

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

        case '(':
            term->char_handler = guac_terminal_g0_charset; 
            break;

        case ')':
            term->char_handler = guac_terminal_g1_charset; 
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

        /* Set Tab (HTS) */
        case 'H':
            guac_terminal_set_tab(term, term->cursor_col);
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

        /* DEC Identify */
        case 'Z':
            guac_terminal_send_string(term, GUAC_TERMINAL_VT102_ID);
            term->char_handler = guac_terminal_echo; 
            break;

        /* Reset */
        case 'c':
            guac_terminal_reset(term);
            break;

        default:
            guac_client_log(term->client, GUAC_LOG_INFO, "Unhandled ESC sequence: %c", c);
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
        case 'B': return NULL;
        case '0': return vt100_map;
        case 'U': return null_map;
        case 'K': return user_map;
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

    if (private_mode == '?') {
        switch (num) {
            case 1:  return &(term->application_cursor_keys); /* DECCKM */
        }
    }

    else if (private_mode == 0) {
        switch (num) {
            case 4:  return &(term->insert_mode); /* DECIM */
            case 20: return &(term->automatic_carriage_return); /* LF/NL */
        }
    }

    /* Unknown flag */
    return NULL;

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
            case '`':
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
                else if (argv[0] == 2 || argv[0] == 3)
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

            /* ]: Linux Private CSI */
            case ']':
                /* Explicitly ignored */
                break;

            /* c: Identify */
            case 'c':
                if (argv[0] == 0 && private_mode_character == 0)
                    guac_terminal_send_string(term, GUAC_TERMINAL_VT102_ID);
                break;

            /* d: Move cursor, current col */
            case 'd':
                row = argv[0]; if (row != 0) row--;
                term->cursor_row = row;
                break;

            /* g: Clear tab */
            case 'g':

                /* Clear tab at current location */
                if (argv[0] == 0)
                    guac_terminal_unset_tab(term, term->cursor_col);

                /* Clear all tabs */
                else if (argv[0] == 3)
                    guac_terminal_clear_tabs(term);

                break;

            /* h: Set Mode */
            case 'h':
             
                /* Look up flag and set */ 
                flag = __guac_terminal_get_flag(term, argv[0], private_mode_character);
                if (flag != NULL)
                    *flag = true;

                break;

            /* l: Reset Mode */
            case 'l':
              
                /* Look up flag and clear */ 
                flag = __guac_terminal_get_flag(term, argv[0], private_mode_character);
                if (flag != NULL)
                    *flag = false;

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

                    /* Reverse video */
                    else if (value == 7)
                        term->current_attributes.reverse = true;

                    /* Normal intensity (not bold) */
                    else if (value == 21 || value == 22)
                        term->current_attributes.bold = false;

                    /* Reset underscore */
                    else if (value == 24)
                        term->current_attributes.underscore = false;

                    /* Reset reverse video */
                    else if (value == 27)
                        term->current_attributes.reverse = false;

                    /* Foreground */
                    else if (value >= 30 && value <= 37)
                        term->current_attributes.foreground = value - 30;

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

                    /* Background */
                    else if (value >= 40 && value <= 47)
                        term->current_attributes.background = value - 40;

                    /* Reset background */
                    else if (value == 49)
                        term->current_attributes.background =
                            term->default_char.attributes.background;

                }

                break;

            /* n: Status report */
            case 'n':

                /* Device status report */
                if (argv[0] == 5 && private_mode_character == 0)
                    guac_terminal_send_string(term, GUAC_TERMINAL_OK);

                /* Cursor position report */
                else if (argv[0] == 6 && private_mode_character == 0)
                    guac_terminal_sendf(term, "\x1B[%i;%iR", term->cursor_row+1, term->cursor_col+1);

                break;

            /* q: Set keyboard LEDs */
            case 'q':
                /* Explicitly ignored */
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

            /* Save Cursor */
            case 's':
                term->saved_cursor_row = term->cursor_row;
                term->saved_cursor_col = term->cursor_col;
                break;

            /* Restore Cursor */
            case 'u':

                term->cursor_row = term->saved_cursor_row;
                if (term->cursor_row >= term->term_height)
                    term->cursor_row = term->term_height - 1;

                term->cursor_col = term->saved_cursor_col;
                if (term->cursor_col >= term->term_width)
                    term->cursor_col = term->term_width - 1;

                break;

            /* Warn of unhandled codes */
            default:
                if (c != ';') {

                    guac_client_log(term->client, GUAC_LOG_INFO,
                            "Unhandled CSI sequence: %c", c);

                    for (i=0; i<argc; i++)
                        guac_client_log(term->client, GUAC_LOG_INFO,
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
        length = 0;
    }

    /* Otherwise, store character */
    else if (length < sizeof(filename)-1)
        filename[length++] = c;

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
        if (operation == 482200)
            term->char_handler = guac_terminal_download;

        /* Set upload directory OSC */
        else if (operation == 482201)
            term->char_handler = guac_terminal_set_directory;

        /* Reset parameter for next OSC */
        operation = 0;

    }

    /* Stop on ECMA-48 ST (String Terminator */
    else if (c == 0x9C || c == 0x5C || c == 0x07)
        term->char_handler = guac_terminal_echo;

    /* Stop on unrecognized character */
    else {
        guac_client_log(term->client, GUAC_LOG_INFO, "Unexpected character in OSC: 0x%X", c);
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
        case '8':

            for (row=0; row<term->term_height; row++)
                guac_terminal_set_columns(term, row, 0, term->term_width-1, &guac_char);

            break;

    }

    term->char_handler = guac_terminal_echo; 

    return 0;

}

