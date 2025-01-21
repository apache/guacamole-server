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

#ifndef _GUAC_TERMINAL_H
#define _GUAC_TERMINAL_H

/**
 * Constants, structures, and function definitions defining the core
 * functionality for the terminal library.
 *
 * @file terminal.h
 */

#include <pthread.h>
#include <stdbool.h>

#include <guacamole/client.h>
#include <guacamole/stream.h>

/**
 * The name of the font to use for the terminal if no name is specified.
 */
#define GUAC_TERMINAL_DEFAULT_FONT_NAME "monospace"

/**
 * The size of the font to use for the terminal if no font size is specified,
 * in points.
 */
#define GUAC_TERMINAL_DEFAULT_FONT_SIZE 12

/**
 * The default maximum scrollback size in rows.
 */
#define GUAC_TERMINAL_DEFAULT_MAX_SCROLLBACK 1000

/**
 * The default ASCII code to use for the backspace key.
 */
#define GUAC_TERMINAL_DEFAULT_BACKSPACE 127

/**
 * The default (unset) color scheme.
 */
#define GUAC_TERMINAL_DEFAULT_COLOR_SCHEME ""

/**
 * The default value for the "disable copy" flag; by default copying is enabled.
 */
#define GUAC_TERMINAL_DEFAULT_DISABLE_COPY false

/**
 * The absolute maximum number of rows to allow within the display.
 */
#define GUAC_TERMINAL_MAX_ROWS 1024

/**
 * The absolute maximum number of columns to allow within the display. This
 * implicitly limits the number of columns allowed within the buffer.
 */
#define GUAC_TERMINAL_MAX_COLUMNS 1024

/**
 * The maximum duration of a single frame, in milliseconds.
 */
#define GUAC_TERMINAL_FRAME_DURATION 40

/**
 * The maximum amount of time to wait for more data before declaring a frame
 * complete, in milliseconds.
 */
#define GUAC_TERMINAL_FRAME_TIMEOUT 10

/**
 * The maximum number of custom tab stops.
 */
#define GUAC_TERMINAL_MAX_TABS       16

/**
 * The number of rows to scroll per scroll wheel event.
 */
#define GUAC_TERMINAL_WHEEL_SCROLL_AMOUNT 3

/**
 * Flag which specifies that terminal output should be sent to both the current
 * pipe stream and the user's display. By default, terminal output will be sent
 * only to the open pipe.
 */
#define GUAC_TERMINAL_PIPE_INTERPRET_OUTPUT 1

/**
 * Flag which forces the open pipe stream to be flushed automatically, whenever
 * a new frame would be rendered, with only minimal buffering performed between
 * frames. By default, the contents of the pipe stream will be flushed only
 * when the buffer is full or the pipe stream is being closed.
 */
#define GUAC_TERMINAL_PIPE_AUTOFLUSH 2

/**
 * Keysym for up arrow.
 */
#define GUAC_TERMINAL_KEY_UP          0xFF52

/**
 * Keysym for down arrow.
 */
#define GUAC_TERMINAL_KEY_DOWN        0xFF54

/**
 * Keysym for left arrow.
 */
#define GUAC_TERMINAL_KEY_LEFT        0xFF51

/**
 * Keysym for right arrow.
 */
#define GUAC_TERMINAL_KEY_RIGHT       0xFF53

/**
 * Keysym for home key.
 */
#define GUAC_TERMINAL_KEY_HOME        0xFF50

/**
 * Keysym for end key.
 */
#define GUAC_TERMINAL_KEY_END         0xFF57

/**
 * Keysym for page up.
 */
#define GUAC_TERMINAL_KEY_PAGEUP      0xFF55

/**
 * Keysym for page down.
 */
#define GUAC_TERMINAL_KEY_PAGEDOWN    0xFF56

/**
 * Keysym for keypad up arrow.
 */
#define GUAC_TERMINAL_KEY_KP_UP       0xFF97

/**
 * Keysym for keypad down arrow.
 */
#define GUAC_TERMINAL_KEY_KP_DOWN     0xFF99

/**
 * Keysym for keypad left arrow.
 */
#define GUAC_TERMINAL_KEY_KP_LEFT     0xFF96

/**
 * Keysym for keypad right arrow.
 */
#define GUAC_TERMINAL_KEY_KP_RIGHT    0xFF98

/**
 * Keysym for keypad home key.
 */
#define GUAC_TERMINAL_KEY_KP_HOME     0xFF95

/**
 * Keysym for keypad end key.
 */
#define GUAC_TERMINAL_KEY_KP_END      0xFF9C

/**
 * Keysym for keypad page up.
 */
#define GUAC_TERMINAL_KEY_KP_PAGEUP   0xFF9A

/**
 * Keysym for keypad page down.
 */
#define GUAC_TERMINAL_KEY_KP_PAGEDOWN 0xFF9B

/**
 * Keysym for left Control.
 */
#define GUAC_TERMINAL_KEY_CTRL_L      0xFFE3

/**
 * Keysym for right Control.
 */
#define GUAC_TERMINAL_KEY_CTRL_R      0xFFE4

/**
 * Keysym for left Meta.
 */
#define GUAC_TERMINAL_KEY_META_L      0xFFE7

/**
 * Keysym for right Meta.
 */
#define GUAC_TERMINAL_KEY_META_R      0xFFE8

/**
 * Keysym for left ALT.
 */
#define GUAC_TERMINAL_KEY_ALT_L       0xFFE9

/**
 * Keysym for right ALT.
 */
#define GUAC_TERMINAL_KEY_ALT_R       0xFFEA

/**
 * Keysym for left Shift.
 */
#define GUAC_TERMINAL_KEY_SHIFT_L     0xFFE1

/**
 * Keysym for right Shift.
 */
#define GUAC_TERMINAL_KEY_SHIFT_R     0xFFE2

/**
 * Keysym for escape.
 */
#define GUAC_TERMINAL_KEY_ESCAPE      0xFF1B

/**
 * Keysym for insert.
 */
#define GUAC_TERMINAL_KEY_INSERT      0xFF63

/**
 * Keysym for enter.
 */
#define GUAC_TERMINAL_KEY_ENTER       0xFF0D

/**
 * Keysym for tab.
 */
#define GUAC_TERMINAL_KEY_TAB         0xFF09

/**
 * Keysym for backspace.
 */
#define GUAC_TERMINAL_KEY_BACKSPACE   0xFF08

/**
 * Keysym for delete.
 */
#define GUAC_TERMINAL_KEY_DELETE      0xFFFF

/**
 * Keysym for keypad insert.
 */
#define GUAC_TERMINAL_KEY_KP_INSERT   0xFF9E

/**
 * Keysym for keypad enter.
 */
#define GUAC_TERMINAL_KEY_KP_ENTER    0xFF8D

/**
 * Keysym for keypad tab.
 */
#define GUAC_TERMINAL_KEY_KP_TAB      0xFF89

/**
 * Keysym for keypad delete.
 */
#define GUAC_TERMINAL_KEY_KP_DELETE   0xFF9F

/**
 * Keysym for Function key F1.
 */
#define GUAC_TERMINAL_KEY_F1          0xFFBE

/**
 * Keysym for Function key F2.
 */
#define GUAC_TERMINAL_KEY_F2          0xFFBF

/**
 * Keysym for Function key F3.
 */
#define GUAC_TERMINAL_KEY_F3          0xFFC0

/**
 * Keysym for Function key F4.
 */
#define GUAC_TERMINAL_KEY_F4          0xFFC1

/**
 * Keysym for Function key F5.
 */
#define GUAC_TERMINAL_KEY_F5          0xFFC2

/**
 * Keysym for Function key F6.
 */
#define GUAC_TERMINAL_KEY_F6          0xFFC3

/**
 * Keysym for Function key F7.
 */
#define GUAC_TERMINAL_KEY_F7          0xFFC4

/**
 * Keysym for Function key F8.
 */
#define GUAC_TERMINAL_KEY_F8          0xFFC5

/**
 * Keysym for Function key F9.
 */
#define GUAC_TERMINAL_KEY_F9          0xFFC6

/**
 * Keysym for Function key F10.
 */
#define GUAC_TERMINAL_KEY_F10         0xFFC7

/**
 * Keysym for Function key F11.
 */
#define GUAC_TERMINAL_KEY_F11         0xFFC8

/**
 * Keysym for Function key F12.
 */
#define GUAC_TERMINAL_KEY_F12         0xFFC9

/**
 * Keysym for keypad Function key F1.
 */
#define GUAC_TERMINAL_KEY_KP_F1       0xFF91

/**
 * Keysym for keypad Function key F2.
 */
#define GUAC_TERMINAL_KEY_KP_F2       0xFF92

/**
 * Keysym for keypad Function key F3.
 */
#define GUAC_TERMINAL_KEY_KP_F3       0xFF93

/**
 * Keysym for keypad Function key F4.
 */
#define GUAC_TERMINAL_KEY_KP_F4       0xFF94

/**
 * Hex ASCII code for Tab.
 */
#define GUAC_TERMINAL_ASCII_TAB                "\x09"

/**
 * Hex ASCII code for Carriage Return.
 */
#define GUAC_TERMINAL_ASCII_CR                 "\x0D"

/**
 * Hex ASCII code for Escape.
 */
#define GUAC_TERMINAL_ASCII_ESCAPE             "\x1B"

/**
 * Escape sequence: page up.
 */
#define GUAC_TERMINAL_ESC_SEQ_PAGEUP           "\x1B[5~"

/**
 * Escape sequence: page down.
 */
#define GUAC_TERMINAL_ESC_SEQ_PAGEDOWN         "\x1B[6~"

/**
 * Escape sequence: end.
 */
#define GUAC_TERMINAL_ESC_SEQ_END              "\x1B[4~"

/**
 * Escape sequence: insert.
 */
#define GUAC_TERMINAL_ESC_SEQ_INSERT           "\x1B[2~"

/**
 * Escape sequence: home.
 */
#define GUAC_TERMINAL_ESC_SEQ_HOME             "\x1B[1~"

/**
 * Escape sequence: move cursor left.
 */
#define GUAC_TERMINAL_ESC_SEQ_CURSOR_LEFT      "\x1B[D"

/**
 * Escape sequence: move cursor right.
 */
#define GUAC_TERMINAL_ESC_SEQ_CURSOR_RIGHT     "\x1B[C"

/**
 * Escape sequence: move cursor up.
 */
#define GUAC_TERMINAL_ESC_SEQ_CURSOR_UP        "\x1B[A"

/**
 * Escape sequence: move cursor down.
 */
#define GUAC_TERMINAL_ESC_SEQ_CURSOR_DOWN      "\x1B[B"

/**
 * Escape sequence: delete.
 */
#define GUAC_TERMINAL_ESC_SEQ_DELETE           "\x1B[3~"

/**
 * Escape sequence: application cursor move left.
 */
#define GUAC_TERMINAL_ESC_SEQ_APP_CURSOR_LEFT  "\x1BOD"

/**
 * Escape sequence: application cursor move right.
 */
#define GUAC_TERMINAL_ESC_SEQ_APP_CURSOR_RIGHT "\x1BOC"

/**
 * Escape sequence: application cursor move up.
 */
#define GUAC_TERMINAL_ESC_SEQ_APP_CURSOR_UP    "\x1BOA"

/**
 * Escape sequence: application cursor move down.
 */
#define GUAC_TERMINAL_ESC_SEQ_APP_CURSOR_DOWN  "\x1BOB"

/**
 * Escape sequence: function key F1.
 */
#define GUAC_TERMINAL_ESC_SEQ_F1               "\x1B[[A"

/**
 * Escape sequence: function key F2.
 */
#define GUAC_TERMINAL_ESC_SEQ_F2               "\x1B[[B"

/**
 * Escape sequence: function key F3.
 */
#define GUAC_TERMINAL_ESC_SEQ_F3               "\x1B[[C"

/**
 * Escape sequence: function key F4.
 */
#define GUAC_TERMINAL_ESC_SEQ_F4               "\x1B[[D"

/**
 * Escape sequence: function key F5.
 */
#define GUAC_TERMINAL_ESC_SEQ_F5               "\x1B[[E"

/**
 * Escape sequence: function key F6.
 */
#define GUAC_TERMINAL_ESC_SEQ_F6               "\x1B[17~"

/**
 * Escape sequence: function key F7.
 */
#define GUAC_TERMINAL_ESC_SEQ_F7               "\x1B[18~"

/**
 * Escape sequence: function key F8.
 */
#define GUAC_TERMINAL_ESC_SEQ_F8               "\x1B[19~"

/**
 * Escape sequence: function key F9.
 */
#define GUAC_TERMINAL_ESC_SEQ_F9               "\x1B[20~"

/**
 * Escape sequence: function key F10.
 */
#define GUAC_TERMINAL_ESC_SEQ_F10              "\x1B[21~"

/**
 * Escape sequence: function key F11.
 */
#define GUAC_TERMINAL_ESC_SEQ_F11              "\x1B[22~"

/**
 * Escape sequence: function key F12.
 */
#define GUAC_TERMINAL_ESC_SEQ_F12              "\x1B[23~"

/**
 * Escape sequence: go to previous word.
 */
#define GUAC_TERMINAL_ESC_SEQ_PREV_WORD        "\033[1;5D"

/**
 * Escape sequence: go to next word.
 */
#define GUAC_TERMINAL_ESC_SEQ_NEXT_WORD        "\033[1;5C"

/**
 * Escape sequence: delete word.
 */
#define GUAC_TERMINAL_ESC_SEQ_DELETE_WORD      "\033D"

/**
 * Represents a terminal emulator which uses a given Guacamole client to
 * render itself.
 */
typedef struct guac_terminal guac_terminal;

/**
 * All possible mouse cursors used by the terminal emulator.
 */
typedef enum guac_terminal_cursor_type {

    /**
     * A transparent (blank) cursor.
     */
    GUAC_TERMINAL_CURSOR_BLANK,

    /**
     * A standard I-bar cursor for selecting text, etc.
     */
    GUAC_TERMINAL_CURSOR_IBAR,

    /**
     * A standard triangular mouse pointer for manipulating non-text objects.
     */
    GUAC_TERMINAL_CURSOR_POINTER

} guac_terminal_cursor_type;

/**
 * Handler that is invoked whenever the necessary terminal codes are sent to
 * to the given terminal to change the path for future file uploads.
 *
 * @param client
 *     The guac_client associated with the terminal receiving the upload path
 *     change terminal code.
 *
 * @param path
 *     The requested path.
 */
typedef void guac_terminal_upload_path_handler(guac_client* client, char* path);

/**
 * Handler that is invoked whenever the necessary terminal codes are sent to
 * initiate a download of a given remote file.
 *
 * @param client
 *     The guac_client associated with the terminal receiving the file download
 *     terminal code.
 *
 * @param filename
 *     The name of the requested file. This may be a relative or absolute path,
 *     and it is up to the implementation to define how this value is
 *     interpreted.
 *
 * @return
 *     The file stream created for the file download, already configured to
 *     properly handle "ack" responses, etc. from the client, or NULL if the
 *     stream could not be created.
 */
typedef guac_stream* guac_terminal_file_download_handler(guac_client* client, char* filename);

/**
 * Configuration options that may be passed when creating a new guac_terminal.
 *
 * Note that guac_terminal_options should not be instantiated directly -
 * to create a new options struct, use guac_terminal_options_create.
 */
typedef struct guac_terminal_options {

    /**
     * Whether copying from the terminal clipboard should be blocked. If set,
     * the contents of the terminal can still be copied, but will be usable
     * only within the terminal itself. The clipboard contents will not be
     * automatically streamed to the client.
     */
    bool disable_copy;

    /**
     * The maximum number of rows to allow within the scrollback buffer. The
     * user may still alter the size of the scrollback buffer using terminal
     * codes, however the size can never exceed the maximum size given here.
     * Note that this space is shared with the display, with the scrollable
     * area actually only containing the given number of rows less the number
     * of rows currently displayed, and sufficient buffer space will always be
     * allocated to represent the display area of the terminal regardless of
     * the value given here.
     */
    int max_scrollback;

    /**
     * The name of the font to use when rendering glyphs.
     */
    char* font_name;

    /**
     * The size of each glyph, in points.
     */
    int font_size;

    /**
     * The DPI of the display. The given font size will be adjusted to produce
     * glyphs at the given DPI.
     */
    int dpi;

    /**
     * The width of the terminal, in pixels.
     */
    int width;

    /**
     * The height of the terminal, in pixels.
     */
    int height;

    /**
     * The name of the color scheme to use. This string must be in the format
     * accepted by guac_terminal_parse_color_scheme().
     */
    char* color_scheme;

    /**
     * The integer ASCII code to send when backspace is pressed in
     * the terminal.
     */
    int backspace;

} guac_terminal_options;

/**
 * Creates a new guac_terminal, having the given width and height, and
 * rendering to the given client. As failover mechanisms and the Guacamole
 * client implementation typically use the receipt of a "sync" message to
 * denote successful connection, rendering of frames (sending of "sync") will
 * be withheld until guac_terminal_start() is called, and user input will be
 * ignored. The guac_terminal_start() function should be invoked only after
 * either the underlying connection has truly succeeded, or until visible
 * terminal output or user input is required.
 *
 * @param client
 *      The client to which the terminal will be rendered.
 *
 * @param terminal_options
 *     The configuration used for instantiating the terminal. For information
 *     about the options, see the guac_terminal_options definition.
 *
 * @return
 *     A new guac_terminal having the given font, dimensions, and attributes
 *     which renders all text to the given client.
 */
guac_terminal* guac_terminal_create(guac_client* client,
        guac_terminal_options* terminal_options);

/**
 * Create a new guac_terminal_options struct. All parameters are required.
 * Any options that are not passed in this constructor will be set to
 * default values unless overriden.
 *
 * The guac_terminal_options struct should only be created using this
 * constructor.
 *
 * @param width
 *     The width of the terminal, in pixels.
 *
 * @param height
 *     The height of the terminal, in pixels.
 *
 * @param dpi
 *     The DPI of the display. The given font size will be adjusted to produce
 *     glyphs at the given DPI.
 *
 * @return
 *     A new terminal options struct with all required options populated,
 *     ready to have any defaults overriden as needed.
 */
guac_terminal_options* guac_terminal_options_create(
        int width, int height, int dpi);

/**
 * Frees all resources associated with the given terminal.
 *
 * @param term
 *     The terminal to free.
 */
void guac_terminal_free(guac_terminal* term);

/**
 * Sets the upload path handler for the given terminal. The upload path handler
 * is invoked whenever the terminal codes requesting an upload path change are
 * sent.
 *
 * @param terminal
 *     The terminal to set the upload path handler for.
 *
 * @param upload_path_handler
 *      The handler to be called whenever the necessary terminal codes are sent
 *      to the given terminal to change the path for future file uploads.
 */
void guac_terminal_set_upload_path_handler(guac_terminal* terminal,
        guac_terminal_upload_path_handler* upload_path_handler);

/**
 * Sets the file download handler for the given terminal. The file download
 * handler is invoked whenever the terminal codes requesting download of a
 * given remote file are sent.
 *
 * @param terminal
 *     The terminal to set the file download handler for.
 *
 * @param file_download_handler
 *      The handler to be called whenever the necessary terminal codes are sent to
 *      the given terminal to initiate a download of a given remote file.
 */
void guac_terminal_set_file_download_handler(guac_terminal* terminal,
        guac_terminal_file_download_handler* file_download_handler);

/**
 * Renders a single frame of terminal data. If data is not yet available,
 * this function will block until data is written.
 *
 * @param terminal
 *     The terminal that should be rendered.
 *
 * @return
 *     Zero if the frame was rendered successfully, non-zero if an error
 *     occurred.
 */
int guac_terminal_render_frame(guac_terminal* terminal);

/**
 * Reads from this terminal's STDIN. Input comes from key and mouse events
 * supplied by calls to guac_terminal_send_key(),
 * guac_terminal_send_mouse(), and guac_terminal_send_stream(). If input is not
 * yet available, this function will block.
 *
 * @param terminal
 *     The terminal to read from.
 *
 * @param c
 *     The buffer that should receive the bytes read.
 *
 * @param size
 *     The number of bytes available within the buffer.
 *
 * @return
 *     The number of bytes read into the buffer, zero if end-of-file is reached
 *     (STDIN has been closed), or a negative value if an error occurs.
 */
int guac_terminal_read_stdin(guac_terminal* terminal, char* c, int size);

/**
 * Notifies the terminal that rendering should begin and that user input should
 * now be accepted. This function must be invoked following terminal creation
 * for the end of frames to be signalled with "sync" messages. Until this
 * function is invoked, "sync" messages will be withheld.
 *
 * @param term
 *     The terminal to start.
 */
void guac_terminal_start(guac_terminal* term);

/**
 * Manually stop the terminal to forcibly unblock any pending reads/writes,
 * e.g. forcing guac_terminal_read_stdin() to return and cease all terminal I/O.
 *
 * @param term
 *     The terminal to stop.
 */
void guac_terminal_stop(guac_terminal* term);

/**
 * Notifies the terminal that an event has occurred and the terminal should
 * flush itself when reasonable.
 *
 * @param terminal
 *     The terminal to notify.
 */
void guac_terminal_notify(guac_terminal* terminal);

/**
 * Reads a single line from this terminal's STDIN, storing the result in a
 * newly-allocated string. Input is retrieved in the same manner as
 * guac_terminal_read_stdin() and the same restrictions apply. As reading input
 * naturally requires user interaction, this function will implicitly invoke
 * guac_terminal_start().
 *
 * @param terminal
 *     The terminal to which the provided title should be output, and from
 *     whose STDIN the single line of input should be read.
 *
 * @param title
 *     The human-readable title to output to the terminal just prior to reading
 *     from STDIN.
 *
 * @param echo
 *     Non-zero if the characters read from STDIN should be echoed back as
 *     terminal output, or zero if asterisks should be displayed instead.
 *
 * @return
 *     A newly-allocated string containing a single line of input read from the
 *     provided terminal's STDIN. This string must eventually be manually
 *     freed with a call to guac_mem_free().
 */
char* guac_terminal_prompt(guac_terminal* terminal, const char* title,
        bool echo);

/**
 * Writes the given format string and arguments to this terminal's STDOUT in
 * the same manner as printf(). The entire populated format string will always
 * be written unless an error occurs. This function may block until space is
 * freed in the output buffer by guac_terminal_render_frame().
 *
 * @param terminal
 *     The terminal to write to.
 *
 * @param format
 *     A printf-style format string describing the data to be written to
 *     STDOUT.
 *
 * @param ...
 *     Any arguments to use when filling the format string.
 *
 * @return
 *     The number of bytes written to STDOUT, or a negative value if an error
 *     occurs preventing the data from being written in its entirety.
 */
int guac_terminal_printf(guac_terminal* terminal, const char* format, ...);

/**
 * Handles the given key event, sending data, scrolling, pasting clipboard
 * data, etc. as necessary. If terminal input is currently coming from a
 * stream due to a prior call to guac_terminal_send_stream(), any input
 * which would normally result from the key event is dropped.
 *
 * @param term
 *     The terminal which should receive the given data on STDIN.
 *
 * @param keysym
 *     The X11 keysym of the key that was pressed or released.
 *
 * @param pressed
 *     Non-zero if the key represented by the given keysym is currently
 *     pressed, zero if it is released.
 *
 * @return
 *     Zero if the key event was handled successfully, non-zero otherwise.
 */
int guac_terminal_send_key(guac_terminal* term, int keysym, int pressed);

/**
 * Handles the given mouse event, sending data, scrolling, pasting clipboard
 * data, etc. as necessary. If terminal input is currently coming from a
 * stream due to a prior call to guac_terminal_send_stream(), any input
 * which would normally result from the mouse event is dropped.
 *
 * @param term
 *     The terminal which should receive the given data on STDIN.
 *
 * @param user
 *     The user that originated the mouse event.
 *
 * @param x
 *     The X coordinate of the mouse within the display when the event
 *     occurred, in pixels. This value is not guaranteed to be within the
 *     bounds of the display area.
 *
 * @param y
 *     The Y coordinate of the mouse within the display when the event
 *     occurred, in pixels. This value is not guaranteed to be within the
 *     bounds of the display area.
 *
 * @param mask
 *     An integer value representing the current state of each button, where
 *     the Nth bit within the integer is set to 1 if and only if the Nth mouse
 *     button is currently pressed. The lowest-order bit is the left mouse
 *     button, followed by the middle button, right button, and finally the up
 *     and down buttons of the scroll wheel.
 *
 *     @see GUAC_CLIENT_MOUSE_LEFT
 *     @see GUAC_CLIENT_MOUSE_MIDDLE
 *     @see GUAC_CLIENT_MOUSE_RIGHT
 *     @see GUAC_CLIENT_MOUSE_SCROLL_UP
 *     @see GUAC_CLIENT_MOUSE_SCROLL_DOWN
 *
 * @return
 *     Zero if the mouse event was handled successfully, non-zero otherwise.
 */
int guac_terminal_send_mouse(guac_terminal* term, guac_user* user,
        int x, int y, int mask);

/**
 * Sends the given string as if typed by the user. If terminal input is
 * currently coming from a stream due to a prior call to
 * guac_terminal_send_stream(), any input which would normally result from
 * invoking this function is dropped.
 *
 * @param term
 *     The terminal which should receive the given data on STDIN.
 *
 * @param data
 *     The data the terminal should receive on STDIN.
 *
 * @param length
 *     The size of the given data, in bytes.
 *
 * @return
 *     The number of bytes written to STDIN, or a negative value if an error
 *     occurs preventing the data from being written. This should always be
 *     the size of the data given unless data is intentionally dropped.
 */
int guac_terminal_send_data(guac_terminal* term, const char* data, int length);

/**
 * Sends the given string as if typed by the user. If terminal input is
 * currently coming from a stream due to a prior call to
 * guac_terminal_send_stream(), any input which would normally result from
 * invoking this function is dropped.
 *
 * @param term
 *     The terminal which should receive the given data on STDIN.
 *
 * @param data
 *     The data the terminal should receive on STDIN.
 *
 * @return
 *     The number of bytes written to STDIN, or a negative value if an error
 *     occurs preventing the data from being written. This should always be
 *     the size of the data given unless data is intentionally dropped.
 */
int guac_terminal_send_string(guac_terminal* term, const char* data);

/**
 * Writes the given buffer to the given terminal's STDOUT. All requested bytes
 * will be written unless an error occurs. This function may block until space
 * is freed in the output buffer by guac_terminal_render_frame().
 *
 * @param term
 *     The terminal to write to.
 *
 * @param buffer
 *     A buffer containing the characters to be written to the terminal.
 *
 * @param length
 *     The number of bytes within the given string to write to the terminal.
 *
 * @return
 *     The number of bytes written to STDOUT, or a negative value if an error
 *     occurs preventing the data from being written in its entirety.
 */
int guac_terminal_write(guac_terminal* term, const char* buffer, int length);

/**
 * Initializes the handlers of the given guac_stream such that it serves as the
 * source of input to the terminal. Other input sources will be temporarily
 * ignored until the stream is closed through receiving an "end" instruction.
 * If input is already being read from a stream due to a prior call to
 * guac_terminal_send_stream(), the prior call will remain in effect and this
 * call will fail.
 *
 * Calling this function will overwrite the data member of the given
 * guac_stream.
 *
 * @param term
 *     The terminal emulator which should receive input from the given stream.
 *
 * @param user
 *     The user that opened the stream.
 *
 * @param stream
 *     The guac_stream which should serve as the source of input for the
 *     terminal.
 *
 * @return
 *     Zero if the terminal input has successfully been configured to read from
 *     the given stream, non-zero otherwise.
 */
int guac_terminal_send_stream(guac_terminal* term, guac_user* user,
        guac_stream* stream);

/**
 * Sends data through STDIN as if typed by the user, using the format string
 * given and any args (similar to printf). If terminal input is currently
 * coming from a stream due to a prior call to guac_terminal_send_stream(), any
 * input which would normally result from invoking this function is dropped.
 *
 * @param term
 *     The terminal which should receive the given data on STDIN.
 *
 * @param format
 *     A printf-style format string describing the data to be received on
 *     STDIN.
 *
 * @param ...
 *     Any arguments to use when filling the format string.
 *
 * @return
 *     The number of bytes written to STDIN, or a negative value if an error
 *     occurs preventing the data from being written. This should always be
 *     the size of the data given unless data is intentionally dropped.
 */
int guac_terminal_sendf(guac_terminal* term, const char* format, ...);

/**
 * Replicates the current display state to a user that has just joined the
 * connection. All instructions necessary to replicate state are sent over the
 * given socket.
 *
 * @deprecated The guac_terminal_sync_users method should be used when
 * duplicating display state to a set of users.
 *
 * @param term
 *     The terminal emulator associated with the connection being joined.
 *
 * @param user
 *     The user joining the connection.
 *
 * @param socket
 *     The guac_socket specific to the joining user and across which messages
 *     synchronizing the current display state should be sent.
 */
void guac_terminal_dup(guac_terminal* term, guac_user* user,
        guac_socket* socket);

/**
 * Replicates the current display state to one or more users that are joining
 * the connection. All instructions necessary to replicate state are sent over
 * the given socket. The set of users receiving these instructions is
 * determined solely by the socket chosen.
 *
 * @param term
 *     The terminal whose state should be synchronized to the users.
 *
 * @param client
 *     The client associated with the users to be synchronized.
 *
 * @param socket
 *     The socket to which the terminal state will be broadcast.
 */
void guac_terminal_sync_users(
        guac_terminal* term, guac_client* client, guac_socket* socket);

/**
 * Resize the client display and terminal to the given pixel dimensions.
 *
 * @param term
 *     The terminal to resize.
 *
 * @param width
 *     The new terminal width, in pixels.
 *
 * @param height
 *     The new terminal height, in pixels.
 *
 * @return
 *     Zero if the terminal was successfully resized to the given dimensions,
 *     non-zero otherwise.
 */
int guac_terminal_resize(guac_terminal* term, int width, int height);

/**
 * Returns the number of rows within the buffer of the given terminal which are
 * not currently displayed on screen. Adjustments to the desired scrollback
 * size are taken into account, and rows which are within the buffer but
 * unavailable due to being outside the desired scrollback range are ignored.
 *
 * @param term
 *     The terminal whose off-screen row count should be determined.
 *
 * @return
 *     The number of rows within the buffer which are not currently displayed
 *     on screen.
 */
int guac_terminal_get_available_scroll(guac_terminal* term);

/**
 * Returns the height of the given terminal, in characters.
 *
 * @param term
 *     The terminal whose height should be determined.
 *
 * @return
 *     The height of the terminal, in characters.
 */
int guac_terminal_get_rows(guac_terminal* term);

/**
 * Returns the width of the given terminal, in characters.
 *
 * @param term
 *     The terminal whose width should be determined.
 *
 * @return
 *     The width of the terminal, in characters.
 */
int guac_terminal_get_columns(guac_terminal* term);

/**
 * Clears the clipboard contents for a given terminal, and assigns a new
 * mimetype for future data.
 *
 * @param terminal
 *      The terminal whose clipboard is being reset.
 * @param mimetype
 *      The mimetype of future data.
 */
void guac_terminal_clipboard_reset(guac_terminal* terminal,
        const char* mimetype);

/**
 * Appends the given data to the contents of the clipboard for the given
 * terminal. The data must match the mimetype chosen for the clipboard data by
 * guac_terminal_clipboard_reset().
 *
 * @param terminal
 *      The terminal whose clipboard is being appended to.
 * @param data
 *      The data to append.
 * @param length
 *      The number of bytes to append from the data given.
 */
void guac_terminal_clipboard_append(guac_terminal* terminal,
        const char* data, int length);

/**
 * Removes the given user from any user-specific resources internal to the
 * given terminal. This function must be called whenever a user leaves a
 * terminal connection.
 *
 * @param terminal
 *      The terminal that the given user is leaving.
 * @param user
 *      The user who is disconnecting.
 */
void guac_terminal_remove_user(guac_terminal* terminal, guac_user* user);

/**
 * Requests that the terminal write all output to a pair of typescript
 * files within the given path and using the given base name. If
 * allow_write_existing is non-zero, these may be existing files; otherwise,
 * the existing files may not be written to, and a non-zero value will be
 * returned.
 *
 * Terminal output will be written to the files, along with timing information.
 * If the create_path flag is non-zero, the given path will be created if it
 * does not yet exist. If creation of the typescript files or path fails,
 * error messages will automatically be logged, and no typescript will be
 * written. The typescript will automatically be closed once the terminal is
 * freed.
 *
 * @param term
 *     The terminal whose output should be written to a typescript.
 *
 * @param path
 *     The full absolute path to a directory in which the typescript files
 *     should be created.
 *
 * @param name
 *     The base name to use for the typescript files created within the
 *     specified path.
 *
 * @param create_path
 *     Zero if the specified path MUST exist for typescript files to be
 *     written, or non-zero if the path should be created if it does not yet
 *     exist.
 *
 * @param allow_write_existing
 *     Non-zero if writing to existing files should be allowed, or zero
 *     otherwise.
 *
 * @return
 *     Zero if the typescript files have been successfully created and a
 *     typescript will be written, non-zero otherwise.
 */
int guac_terminal_create_typescript(guac_terminal* term, const char* path,
        const char* name, int create_path, int allow_write_existing);

/**
 * Immediately applies the given color scheme to the given terminal, overriding
 * the color scheme provided when the terminal was created. Valid color schemes
 * are those accepted by guac_terminal_parse_color_scheme().
 *
 * @param terminal
 *     The terminal to apply the color scheme to.
 *
 * @param color_scheme
 *     The color scheme to apply.
 */
void guac_terminal_apply_color_scheme(guac_terminal* terminal,
        const char* color_scheme);

/**
 * Returns name of the color scheme currently in use by the given terminal.
 *
 * @param terminal
 *      The terminal whose color scheme should be returned.
 *
 * @return
 *     The name of the color scheme currently in use by the given terminal.
 */
const char* guac_terminal_get_color_scheme(guac_terminal* terminal);

/**
 * Alters the font of the terminal. The terminal will automatically be redrawn
 * and resized as necessary. If the terminal size changes, the remote side of
 * the terminal session must be manually informed of that change or graphical
 * artifacts may result.
 *
 * @param terminal
 *     The terminal whose font family and/or size are being changed.
 *
 * @param font_name
 *     The name of the new font family, or NULL if the font family should
 *     remain unchanged.
 *
 * @param font_size
 *     The new font size, in points, or -1 if the font size should remain
 *     unchanged.
 *
 * @param dpi
 *     The resolution of the display in DPI. If the font size will not be
 *     changed (the font size given is -1), this value is ignored.
 */
void guac_terminal_apply_font(guac_terminal* terminal, const char* font_name,
        int font_size, int dpi);

/**
 * Returns the font name currently in use by the given terminal.
 *
 * @param terminal
 *     The terminal whose font name is being retrieved.
 *
 * @return
 *     The name of the font in use by the given terminal.
 */
const char* guac_terminal_get_font_name(guac_terminal* terminal);

/**
 * Returns the font size currently in use by the given terminal.
 *
 * @param terminal
 *     The terminal whose font size is being retrieved.
 *
 * @return
 *     The size of the font in use by the given terminal.
 */
int guac_terminal_get_font_size(guac_terminal* terminal);

/**
 * Returns the current state of the mod_ctrl flag in the given terminal.
 *
 * @param terminal
 *     The terminal for which the mod_ctrl state is being checked.
 *
 * @return
 *     The current state of the mod_ctrl flag.
 */
int guac_terminal_get_mod_ctrl(guac_terminal* terminal);

#endif
