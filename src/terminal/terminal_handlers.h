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


#ifndef _GUAC_TERMINAL_HANDLERS
#define _GUAC_TERMINAL_HANDLERS

#include "config.h"

#include "terminal.h"

/**
 * The default mode of the terminal. This character handler simply echoes
 * received characters to the terminal display, entering other terminal modes
 * if control characters are received.
 *
 * @param term
 *     The terminal that received the given character of data.
 *
 * @param c
 *     The character that was received by the given terminal.
 */
int guac_terminal_echo(guac_terminal* term, unsigned char c);

/**
 * Handles any characters which follow an ANSI ESC (0x1B) character.
 *
 * @param term
 *     The terminal that received the given character of data.
 *
 * @param c
 *     The character that was received by the given terminal.
 */
int guac_terminal_escape(guac_terminal* term, unsigned char c);

/**
 * Selects the G0 character mapping from the provided character mapping
 * specifier (such as B, 0, U, or K).
 *
 * @param term
 *     The terminal that received the given character of data.
 *
 * @param c
 *     The character that was received by the given terminal.
 */
int guac_terminal_g0_charset(guac_terminal* term, unsigned char c);

/**
 * Selects the G1 character mapping from the provided character mapping
 * specifier (such as B, 0, U, or K).
 *
 * @param term
 *     The terminal that received the given character of data.
 *
 * @param c
 *     The character that was received by the given terminal.
 */
int guac_terminal_g1_charset(guac_terminal* term, unsigned char c);

/**
 * Handles characters within a CSI sequence. CSI sequences are most often
 * introduced with "ESC [".
 *
 * @param term
 *     The terminal that received the given character of data.
 *
 * @param c
 *     The character that was received by the given terminal.
 */
int guac_terminal_csi(guac_terminal* term, unsigned char c);

/**
 * Parses the remainder of the download initiation OSC specific to the
 * Guacamole terminal emulator. A download will be initiated for the specified
 * file once the OSC sequence is complete.
 *
 * @param term
 *     The terminal that received the given character of data.
 *
 * @param c
 *     The character that was received by the given terminal.
 */
int guac_terminal_download(guac_terminal* term, unsigned char c);

/**
 * Parses the remainder of the set directory OSC specific to the Guacamole
 * terminal emulator. The upload directory will be set to the specified path
 * once the OSC sequence is complete.
 *
 * @param term
 *     The terminal that received the given character of data.
 *
 * @param c
 *     The character that was received by the given terminal.
 */
int guac_terminal_set_directory(guac_terminal* term, unsigned char c);

/**
 * Parses the remainder of the open pipe OSC specific to the
 * Guacamole terminal emulator. Terminal output will be redirected to a new
 * named pipe having the given name once the OSC sequence is complete.
 *
 * @param term
 *     The terminal that received the given character of data.
 *
 * @param c
 *     The character that was received by the given terminal.
 */
int guac_terminal_open_pipe_stream(guac_terminal* term, unsigned char c);

/**
 * Parses the remainder of the close pipe OSC specific to the Guacamole
 * terminal emulator. Terminal output will be redirected back to the terminal
 * display and any open named pipe will be closed once the OSC sequence is
 * complete.
 *
 * @param term
 *     The terminal that received the given character of data.
 *
 * @param c
 *     The character that was received by the given terminal.
 */
int guac_terminal_close_pipe_stream(guac_terminal* term, unsigned char c);

/**
 * Handles the remaining characters of an Operating System Code (OSC) sequence,
 * typically initiated with "ESC ]".
 *
 * @param term
 *     The terminal that received the given character of data.
 *
 * @param c
 *     The character that was received by the given terminal.
 */
int guac_terminal_osc(guac_terminal* term, unsigned char c);

/**
 * Handles terminal control function sequences initiated with "ESC #".
 *
 * @param term
 *     The terminal that received the given character of data.
 *
 * @param c
 *     The character that was received by the given terminal.
 */
int guac_terminal_ctrl_func(guac_terminal* term, unsigned char c);

#endif

