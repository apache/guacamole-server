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

#include "common/clipboard.h"
#include "common/cursor.h"
#include "common/iconv.h"
#include "terminal/buffer.h"
#include "terminal/color-scheme.h"
#include "terminal/common.h"
#include "terminal/display.h"
#include "terminal/palette.h"
#include "terminal/select.h"
#include "terminal/terminal.h"
#include "terminal/terminal-handlers.h"
#include "terminal/terminal-priv.h"
#include "terminal/types.h"
#include "terminal/typescript.h"

#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <wchar.h>

#include <guacamole/client.h>
#include <guacamole/error.h>
#include <guacamole/flag.h>
#include <guacamole/mem.h>
#include <guacamole/proctitle.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/string.h>
#include <guacamole/timestamp.h>
#include <guacamole/user.h>

/*
 * Terminal key input is represented and encoded across several layers:
 *
 * 1) Incoming keys use X11 keysyms (e.g. XK_Left, XK_F1, modifier keysyms),
 *    including Unicode keysyms (0x01000000 | codepoint).
 *
 * 2) Some Ctrl+key combinations map directly to ASCII C0 control bytes
 *    (0x00-0x1F) plus DEL (0x7F), such as Ctrl+C -> ETX and Ctrl+[ -> ESC.
 *
 * 3) Non-printable/navigation/function keys are typically emitted as ANSI/xterm
 *    escape sequences beginning with ESC (0x1B), including CSI sequences
 *    (ESC [ ...), and xterm-style modifier parameters.
 *    This includes arrow/navigation keys, function keys, and keypad variants;
 *    when Shift/Alt/Ctrl/Meta are held, xterm-style modifier parameters are
 *    appended to encode those combinations in the resulting sequence so
 *    terminal applications can distinguish and bind keyboard shortcuts.
 *
 * Keyboard emulation strategy:
 *     For all terminal types, keyboard emulation supports xterm-style extended
 *     behavior for function keys (F1-F12), arrow keys (Up, Down, Left, Right),
 *     cursor editing keys (Home, End, Insert, Delete, Page Up, Page Down), keypad
 *     keys, and Ctrl/Shift/Alt/Meta modifier encoding via xterm-style CSI modifier
 *     parameters (`ESC [ ... ; <mod> ...`), even if the configured terminal type
 *     does not natively support these extended features.
 *
 *     The default terminal type (`linux`) preserves legacy Linux console keycodes
 *     for F1-F5, keypad F1-F4, and Home/End to match the sequences expected by
 *     `$TERM=linux` terminfo entries.
 *
 * References:
 * - X.Org keysyms: https://cgit.freedesktop.org/xorg/proto/x11proto/tree/keysymdef.h
 * - xterm control sequences: https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
 * - ASCII control codes (C0/DEL): https://www.rfc-editor.org/rfc/rfc20
 */

/*
 * Mask used to detect the X11 Unicode keysym encoding.
 *
 * See X11 keysymdef.h.
 */
#define GUAC_KEYSYM_UNICODE_MASK    0xFFFF0000

/*
 * Prefix used by X11 to encode a Unicode codepoint as a keysym:
 * 0x01000000 | codepoint.
 *
 * See X11 keysymdef.h.
 */
#define GUAC_KEYSYM_UNICODE_PREFIX  0x01000000

/*
 * X11 keypad keysyms begin at 0xFF80 (XK_KP_Space). Subtracting this offset
 * allows keypad keysyms to be mapped onto their corresponding ASCII values
 * where applicable.
 */
#define GUAC_KEYSYM_KP_ASCII_OFFSET 0xFF80

/**
 * Canonical X11 keysyms used by the terminal keyboard emulation logic.
 */
typedef enum guac_terminal_keysym {

    /* Modifier keys */
    GUAC_KEYSYM_SHIFT_L = 0xFFE1,           /* Left Shift modifier key.                    */
    GUAC_KEYSYM_SHIFT_R = 0xFFE2,           /* Right Shift modifier key.                   */
    GUAC_KEYSYM_CTRL_L = 0xFFE3,            /* Left Control modifier key.                  */
    GUAC_KEYSYM_CTRL_R = 0xFFE4,            /* Right Control modifier key.                 */
    GUAC_KEYSYM_META_L = 0xFFE7,            /* Left Meta modifier key.                     */
    GUAC_KEYSYM_META_R = 0xFFE8,            /* Right Meta modifier key.                    */
    GUAC_KEYSYM_ALT_L = 0xFFE9,             /* Left Alt modifier key.                      */
    GUAC_KEYSYM_ALT_R = 0xFFEA,             /* Right Alt modifier key.                     */
    GUAC_KEYSYM_SUPER_L = 0xFFEB,           /* Left Super key, such as Windows/Command.    */
    GUAC_KEYSYM_SUPER_R = 0xFFEC,           /* Right Super key, such as Windows/Command.   */
    GUAC_KEYSYM_HYPER_L = 0xFFED,           /* Left Hyper modifier key.                    */
    GUAC_KEYSYM_HYPER_R = 0xFFEE,           /* Right Hyper modifier key.                   */
    GUAC_KEYSYM_MODE_SWITCH = 0xFF7E,       /* Keyboard group or layout switch key.        */
    GUAC_KEYSYM_ISO_LEVEL3_SHIFT = 0xFE03,  /* ISO Level 3 shift key, often AltGr/Option.  */
    GUAC_KEYSYM_ISO_LEVEL5_SHIFT = 0xFE11,  /* ISO Level 5 shift key.                      */

    /* Arrow/navigation direction keys */
    GUAC_KEYSYM_LEFT = 0xFF51,              /* Left arrow key.                             */
    GUAC_KEYSYM_UP = 0xFF52,                /* Up arrow key.                               */
    GUAC_KEYSYM_RIGHT = 0xFF53,             /* Right arrow key.                            */
    GUAC_KEYSYM_DOWN = 0xFF54,              /* Down arrow key.                             */

    /* Keypad arrow/navigation variants */
    GUAC_KEYSYM_KP_LEFT = 0xFF96,           /* Keypad left arrow key.                      */
    GUAC_KEYSYM_KP_UP = 0xFF97,             /* Keypad up arrow key.                        */
    GUAC_KEYSYM_KP_RIGHT = 0xFF98,          /* Keypad right arrow key.                     */
    GUAC_KEYSYM_KP_DOWN = 0xFF99,           /* Keypad down arrow key.                      */

    /* Function keys */
    GUAC_KEYSYM_F1 = 0xFFBE,                /* F1 function key.                            */
    GUAC_KEYSYM_F2 = 0xFFBF,                /* F2 function key.                            */
    GUAC_KEYSYM_F3 = 0xFFC0,                /* F3 function key.                            */
    GUAC_KEYSYM_F4 = 0xFFC1,                /* F4 function key.                            */
    GUAC_KEYSYM_F5 = 0xFFC2,                /* F5 function key.                            */
    GUAC_KEYSYM_F6 = 0xFFC3,                /* F6 function key.                            */
    GUAC_KEYSYM_F7 = 0xFFC4,                /* F7 function key.                            */
    GUAC_KEYSYM_F8 = 0xFFC5,                /* F8 function key.                            */
    GUAC_KEYSYM_F9 = 0xFFC6,                /* F9 function key.                            */
    GUAC_KEYSYM_F10 = 0xFFC7,               /* F10 function key.                           */
    GUAC_KEYSYM_F11 = 0xFFC8,               /* F11 function key.                           */
    GUAC_KEYSYM_F12 = 0xFFC9,               /* F12 function key.                           */

    /* Keypad function-key variants */
    GUAC_KEYSYM_KP_F1 = 0xFF91,             /* Keypad F1 function key.                     */
    GUAC_KEYSYM_KP_F2 = 0xFF92,             /* Keypad F2 function key.                     */
    GUAC_KEYSYM_KP_F3 = 0xFF93,             /* Keypad F3 function key.                     */
    GUAC_KEYSYM_KP_F4 = 0xFF94,             /* Keypad F4 function key.                     */

    /* Basic control/edit keys */
    GUAC_KEYSYM_BACKSPACE = 0xFF08,         /* Backspace key.                              */
    GUAC_KEYSYM_TAB = 0xFF09,               /* Tab key.                                    */
    GUAC_KEYSYM_KP_TAB = 0xFF89,            /* Keypad Tab key.                             */
    GUAC_KEYSYM_LINE_FEED = 0xFF0A,         /* Line Feed key.                              */
    GUAC_KEYSYM_ENTER = 0xFF0D,             /* Enter key.                                  */
    GUAC_KEYSYM_KP_ENTER = 0xFF8D,          /* Keypad Enter key.                           */
    GUAC_KEYSYM_ESCAPE = 0xFF1B,            /* Escape key.                                 */

    /* Navigation/editing keys and keypad variants */
    GUAC_KEYSYM_HOME = 0xFF50,              /* Home key.                                   */
    GUAC_KEYSYM_KP_HOME = 0xFF95,           /* Keypad Home key.                            */
    GUAC_KEYSYM_PAGE_UP = 0xFF55,           /* Page Up key.                                */
    GUAC_KEYSYM_KP_PAGE_UP = 0xFF9A,        /* Keypad Page Up key.                         */
    GUAC_KEYSYM_PAGE_DOWN = 0xFF56,         /* Page Down key.                              */
    GUAC_KEYSYM_KP_PAGE_DOWN = 0xFF9B,      /* Keypad Page Down key.                       */
    GUAC_KEYSYM_END = 0xFF57,               /* End key.                                    */
    GUAC_KEYSYM_KP_END = 0xFF9C,            /* Keypad End key.                             */
    GUAC_KEYSYM_INSERT = 0xFF63,            /* Insert key.                                 */
    GUAC_KEYSYM_KP_INSERT = 0xFF9E,         /* Keypad Insert key.                          */
    GUAC_KEYSYM_DELETE = 0xFFFF,            /* Delete key.                                 */
    GUAC_KEYSYM_KP_DELETE = 0xFF9F,         /* Keypad Delete key.                          */

    /* Keypad printable range helpers */
    GUAC_KEYSYM_KP_MULTIPLY = 0xFFAA,       /* Keypad multiply key.                        */
    GUAC_KEYSYM_KP_EQUAL = 0xFFBD,          /* Keypad equals key.                          */
    GUAC_KEYSYM_KP_9 = 0xFFB9               /* Keypad 9 key.                               */
} guac_terminal_keysym;

/**
 * Sets the given range of columns to the given character.
 */
static void __guac_terminal_set_columns(guac_terminal* terminal, int row,
        int start_column, int end_column, guac_terminal_char* character) {

    guac_terminal_display_set_columns(terminal->display, row + terminal->scroll_offset,
            start_column, end_column, character);

    guac_terminal_buffer_set_columns(terminal->current_buffer, row,
            start_column, end_column, character);

    /* Clear selection if region is modified */
    guac_terminal_select_touch(terminal, row, start_column, row, end_column);

}

/**
 * Returns the number of rows available within the terminal buffer, taking
 * changes to the desired scrollback size into account. Regardless of the
 * true buffer length, only the number of rows that should be made available
 * will be returned.
 *
 * @param term
 *     The terminal whose effective buffer length should be retrieved.
 *
 * @return
 *     The number of rows effectively available within the terminal buffer,
 *     taking changes to the desired scrollback size into account.
 */
static int guac_terminal_effective_buffer_length(guac_terminal* term) {

    int scrollback = term->requested_scrollback;

    /* Limit available scrollback to defined maximum */
    if (scrollback > term->max_scrollback)
        scrollback = term->max_scrollback;

    /* There must always be at least enough scrollback to cover the visible
     * terminal display */
    else if (scrollback < term->term_height)
        scrollback = term->term_height;

    /* If the buffer contains more rows than requested, pretend it only
     * contains the requested number of rows */
    return guac_terminal_buffer_effective_length(term->current_buffer, scrollback);

}

int guac_terminal_get_available_scroll(guac_terminal* term) {
    return guac_terminal_effective_buffer_length(term) - term->term_height;
}

int guac_terminal_get_rows(guac_terminal* term) {
    return term->term_height;
}

int guac_terminal_get_columns(guac_terminal* term) {
    return term->term_width;
}

void guac_terminal_reset(guac_terminal* term) {

    int row;

    /* Set current state */
    term->char_handler = guac_terminal_echo;
    term->active_char_set = 0;
    term->char_mapping[0] =
    term->char_mapping[1] = NULL;

    /* Reset cursor location */
    term->cursor_row = term->visible_cursor_row = term->saved_cursor_row = 0;
    term->cursor_col = term->visible_cursor_col = term->saved_cursor_col = 0;
    term->cursor_visible = true;

    /* Clear scrollback, buffer, and scroll region */
    guac_terminal_buffer_reset(term->current_buffer);
    term->scroll_start = 0;
    term->scroll_end = term->term_height - 1;
    term->scroll_offset = 0;

    /* Reset scrollbar bounds */
    guac_terminal_scrollbar_set_bounds(term->scrollbar, 0, 0);
    guac_terminal_scrollbar_set_value(term->scrollbar, -term->scroll_offset);

    /* Reset flags */
    term->text_selected = false;
    term->selection_committed = false;
    term->application_cursor_keys = false;
    term->application_keypad_keys = false;
    term->automatic_carriage_return = false;
    term->insert_mode = false;

    /* Reset tabs */
    term->tab_interval = 8;
    memset(term->custom_tabs, 0, sizeof(term->custom_tabs));

    /* Reset character attributes */
    term->current_attributes = term->default_char.attributes;

    /* Reset display palette */
    guac_terminal_display_reset_palette(term->display);

    /* Clear terminal with a row length of term_width-1
     * to avoid exceed the size of the display layer */
    for (row=0; row<term->term_height; row++)
        guac_terminal_set_columns(term, row, 0, term->term_width-1, &(term->default_char));

}

/**
 * Paints or repaints the background of the terminal display. This painting
 * occurs beneath the actual terminal and scrollbar layers, and thus will not
 * overwrite any text or other content currently on the screen. This is only
 * necessary to paint over parts of the terminal background which may otherwise
 * be transparent (the default layer background).
 *
 * @param terminal
 *     The terminal whose background should be painted or repainted.
 *
 * @param socket
 *     The socket over which instructions required to paint / repaint the
 *     terminal background should be send.
 */
static void guac_terminal_repaint_default_layer(guac_terminal* terminal,
        guac_socket* socket) {

    int width = terminal->width;
    int height = terminal->height;
    guac_terminal_display* display = terminal->display;

    /* Get background color */
    const guac_terminal_color* color = &display->default_background;

    /* Reset size */
    guac_protocol_send_size(socket, GUAC_DEFAULT_LAYER, width, height);

    /* Paint background color */
    guac_protocol_send_rect(socket, GUAC_DEFAULT_LAYER, 0, 0, width, height);
    guac_protocol_send_cfill(socket, GUAC_COMP_OVER, GUAC_DEFAULT_LAYER,
            color->red, color->green, color->blue, 0xFF);

}

/**
 * Automatically and continuously renders frames of terminal data while the
 * associated guac_client is running.
 *
 * @param data
 *     A pointer to the guac_terminal that should be continuously rendered
 *     while its associated guac_client is running.
 *
 * @return
 *     Always NULL.
 */
void* guac_terminal_thread(void* data) {

    /* Thread name terminal: renders the terminal emulator display and
     * processes output from the remote. */
    guac_thread_name_set("term-render");

    guac_terminal* terminal = (guac_terminal*) data;
    guac_client* client = terminal->client;

    /* Render frames only while client is running */
    while (client->state == GUAC_CLIENT_RUNNING) {

        /* Stop rendering if an error occurs */
        if (guac_terminal_render_frame(terminal))
            break;

        /* Signal end of frame */
        guac_client_end_frame(client);
        guac_socket_flush(client->socket);

    }

    /* The client has stopped or an error has occurred */
    return NULL;

}

guac_terminal_options* guac_terminal_options_create(
        int width, int height, int dpi) {

    guac_terminal_options* options = guac_mem_zalloc(sizeof(guac_terminal_options));

    /* Set all required parameters */
    options->width = width;
    options->height = height;
    options->dpi = dpi;

    /* Set default values for all other parameters */
    options->clipboard_buffer_size = GUAC_COMMON_CLIPBOARD_MIN_LENGTH;
    options->disable_copy = GUAC_TERMINAL_DEFAULT_DISABLE_COPY;
    options->max_scrollback = GUAC_TERMINAL_DEFAULT_MAX_SCROLLBACK;
    options->font_name = GUAC_TERMINAL_DEFAULT_FONT_NAME;
    options->font_size = GUAC_TERMINAL_DEFAULT_FONT_SIZE;
    options->color_scheme = GUAC_TERMINAL_DEFAULT_COLOR_SCHEME;
    options->backspace = GUAC_TERMINAL_DEFAULT_BACKSPACE;
    options->linux_console_keys = true;

    return options;
}

/**
 * Calculate the available height and width in characters for text display in
 * the terminal and store the results in the pointer arguments.
 *
 * @param terminal
 *     The terminal provides character width and height for calculations.
 *
 * @param height
 *     The outer height of the terminal, in pixels.
 *
 * @param width
 *     The outer width of the terminal, in pixels.
 *
 * @param rows
 *     Pointer to the calculated height of the terminal for text display,
 *     in characters.
 *
 * @param columns
 *     Pointer to the calculated width of the terminal for text display,
 *     in characters.
 */
static void calculate_rows_and_columns(guac_terminal* term,
    int height, int width, int *rows, int *columns) {

    int margin = term->display->margin;
    int char_width = term->display->char_width;
    int char_height = term->display->char_height;

    /* Calculate available display area */
    int available_width = width - GUAC_TERMINAL_SCROLLBAR_WIDTH - 2 * margin;
    if (available_width < 0)
        available_width = 0;

    int available_height = height - 2 * margin;
    if (available_height < 0)
        available_height = 0;

    /* Calculate dimensions */
    *rows    = available_height / char_height;
    *columns = available_width / char_width;

    /* Keep height within predefined maximum */
    if (*rows > GUAC_TERMINAL_MAX_ROWS)
        *rows = GUAC_TERMINAL_MAX_ROWS;

    /* Keep width within predefined maximum */
    if (*columns > GUAC_TERMINAL_MAX_COLUMNS)
        *columns = GUAC_TERMINAL_MAX_COLUMNS;
}

/**
 * Calculate the available height and width in pixels of the terminal for text
 * display in the terminal and store the results in the pointer arguments.
 *
 * @param terminal
 *     The terminal provides character width and height for calculations.
 *
 * @param rows
 *     The available height of the terminal for text display, in characters.
 *
 * @param columns
 *     The available width of the terminal for text display, in characters.
 *
 * @param height
 *     Pointer to the calculated available height of the terminal for text
 *     display, in pixels.
 *
 * @param width
 *     Pointer to the calculated available width of the terminal for text
 *     display, in pixels.
 */
static void calculate_height_and_width(guac_terminal* term,
    int rows, int columns, int *height, int *width) {

    int margin = term->display->margin;
    int char_width = term->display->char_width;
    int char_height = term->display->char_height;

    /* Recalculate height if max rows reached */
    if (rows == GUAC_TERMINAL_MAX_ROWS) {
        int available_height = GUAC_TERMINAL_MAX_ROWS * char_height;
        *height = available_height + 2 * margin;
    }

    /* Recalculate width if max columns reached */
    if (columns == GUAC_TERMINAL_MAX_COLUMNS) {
        int available_width = GUAC_TERMINAL_MAX_COLUMNS * char_width;
        *width = available_width + GUAC_TERMINAL_SCROLLBAR_WIDTH + 2 * margin;
    }
}

guac_terminal* guac_terminal_create(guac_client* client,
        guac_terminal_options* options) {

    /* The width and height may need to be changed from what's requested */
    int width = options->width;
    int height = options->height;

    /* Build default character using default colors */
    guac_terminal_char default_char = {
        .value = 0,
        .attributes = {
            .bold        = false,
            .half_bright = false,
            .reverse     = false,
            .underscore  = false
        },
        .width = 1
    };

    /* Initialized by guac_terminal_parse_color_scheme. */
    guac_terminal_color (*default_palette)[256] = (guac_terminal_color(*)[256])
            guac_mem_alloc(sizeof(guac_terminal_color[256]));

    guac_terminal_parse_color_scheme(client, options->color_scheme,
                                     &default_char.attributes.foreground,
                                     &default_char.attributes.background,
                                     default_palette);

    guac_terminal* term = guac_mem_alloc(sizeof(guac_terminal));
    term->started = false;
    term->client = client;
    term->upload_path_handler = NULL;
    term->file_download_handler = NULL;

    /* Copy initially-provided color scheme and font details */
    term->color_scheme = guac_strdup(options->color_scheme);
    term->font_name = guac_strdup(options->font_name);
    term->font_size = options->font_size;

    /* Init modified flag and conditional */
    guac_flag_init(&term->modified);

    /* Maximum and requested scrollback are initially the same */
    term->max_scrollback = options->max_scrollback;
    term->requested_scrollback = options->max_scrollback;

    /* Allocate enough space for maximum scrollback, bumping up internal
     * storage as necessary to allow screen to be resized to maximum height */
    int initial_scrollback = options->max_scrollback;
    if (initial_scrollback < GUAC_TERMINAL_MAX_ROWS)
        initial_scrollback = GUAC_TERMINAL_MAX_ROWS;

    /* Init current and alternate buffer */
    term->current_buffer = term->normal_buffer = guac_terminal_buffer_alloc(initial_scrollback, &default_char);
    term->alternate_buffer = guac_terminal_buffer_alloc(GUAC_TERMINAL_MAX_ROWS, &default_char);

    /* Init display */
    term->display = guac_terminal_display_alloc(client,
            options->font_name, options->font_size, options->dpi,
            &default_char.attributes.foreground,
            &default_char.attributes.background,
            (guac_terminal_color(*)[256]) default_palette);

    /* Fail if display init failed */
    if (term->display == NULL) {
        guac_client_log(client, GUAC_LOG_DEBUG, "Display initialization failed");
        guac_mem_free(term);
        return NULL;
    }

    /* Init common cursor */
    term->cursor = guac_common_cursor_alloc(client);

    /* Init terminal state */
    term->current_attributes = default_char.attributes;
    term->default_char = default_char;
    term->clipboard = guac_common_clipboard_alloc(options->clipboard_buffer_size);
    term->disable_copy = options->disable_copy;
    term->linux_console_keys = options->linux_console_keys;

    /* Calculate available text display area by character size */
    int rows, columns;
    calculate_rows_and_columns(term, height, width, &rows, &columns);

    /* Calculate available display area in pixels */
    int adjusted_height = height;
    int adjusted_width = width;
    calculate_height_and_width(term, rows, columns,
        &adjusted_height, &adjusted_width);

    /* Set size of available screen area */
    term->outer_height = height;
    term->outer_width = width;

    /* Set rows and columns size */
    term->term_height = rows;
    term->term_width  = columns;

    /* Set pixel size */
    term->height = adjusted_height;
    term->width = adjusted_width;

    /* Open STDIN pipe */
    if (pipe(term->stdin_pipe_fd)) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Unable to open pipe for STDIN";
        guac_mem_free(term);
        return NULL;
    }

    /* Read input from keyboard by default */
    term->input_stream = NULL;

    /* Init pipe stream (output to display by default) */
    term->pipe_stream = NULL;

    /* No typescript by default */
    term->typescript = NULL;

    /* Init terminal lock */
    pthread_mutex_init(&(term->lock), NULL);

    /* Repaint and resize overall display */
    guac_terminal_repaint_default_layer(term, term->client->socket);
    guac_terminal_display_resize(term->display,
            term->term_width, term->term_height);

    /* Allocate scrollbar */
    term->scrollbar = guac_terminal_scrollbar_alloc(term->client, GUAC_DEFAULT_LAYER,
            term->outer_width, term->outer_height, term->term_height);

    /* Associate scrollbar with this terminal */
    term->scrollbar->data = term;
    term->scrollbar->scroll_handler = guac_terminal_scroll_handler;

    /* Init terminal */
    guac_terminal_reset(term);

    /* All mouse buttons are released */
    term->mouse_mask = 0;

    /* All keyboard modifiers are released */
    term->mod_alt   =
    term->mod_ctrl  =
    term->mod_meta  =
    term->mod_shift = 0;

    /* Initialize mouse cursor */
    term->current_cursor = GUAC_TERMINAL_CURSOR_BLANK;
    guac_common_cursor_set_blank(term->cursor);

    /* Start terminal thread */
    if (pthread_create(&(term->thread), NULL,
                guac_terminal_thread, (void*) term)) {
        guac_terminal_free(term);
        return NULL;
    }

    /* Configure backspace */
    term->backspace = options->backspace;

    /* Configure the family of codes for function keys and the keypad */
    if (options->func_keys_and_keypad == NULL ||
            options->func_keys_and_keypad[0] == 0 ||
            strcmp(options->func_keys_and_keypad, "esc") == 0) {
        term->func_keys_and_keypad = GUAC_TERMINAL_FUNC_KEYS_AND_KEYPAD_ESC;
    }
    else if (strcmp(options->func_keys_and_keypad, "vt100") == 0) {
        term->func_keys_and_keypad = GUAC_TERMINAL_FUNC_KEYS_AND_KEYPAD_VT100;
    }

    /* Initialize mouse latest click time and counter */
    term->click_timer = 0;
    term->click_counter = 0;

    return term;

}

void guac_terminal_start(guac_terminal* term) {
    term->started = true;
    guac_terminal_notify(term);
}

void guac_terminal_stop(guac_terminal* term) {

    /* Close input pipe and set fds to invalid */
    if (term->stdin_pipe_fd[1] != -1) {
        close(term->stdin_pipe_fd[1]);
        term->stdin_pipe_fd[1] = -1;
    }
    if (term->stdin_pipe_fd[0] != -1) {
        close(term->stdin_pipe_fd[0]);
        term->stdin_pipe_fd[0] = -1;
    }
}

void guac_terminal_free(guac_terminal* term) {

    /* Close user input pipe */
    guac_terminal_stop(term);

    /* Wait for render thread to finish */
    pthread_join(term->thread, NULL);

    /* Close and flush any open pipe stream */
    guac_terminal_pipe_stream_close(term);

    /* Close and flush any active typescript */
    guac_terminal_typescript_free(term->typescript);

    /* Free scrollbar */
    guac_terminal_scrollbar_free(term->scrollbar);

    /* Free display */
    guac_terminal_display_free(term->display);

    /* Free buffers */
    guac_terminal_buffer_free(term->normal_buffer);
    guac_terminal_buffer_free(term->alternate_buffer);

    /* Free copies of font and color scheme information */
    guac_mem_free_const(term->color_scheme);
    guac_mem_free_const(term->font_name);

    /* Free clipboard */
    guac_common_clipboard_free(term->clipboard);

    /* Free the terminal itself */
    pthread_mutex_destroy(&term->lock);
    guac_mem_free(term);

}

/**
 * Waits for the terminal state to be modified, returning only when the
 * specified timeout has elapsed or a frame flush is desired. Note that the
 * modified flag of the terminal will only be reset if no data remains to be
 * read from STDOUT.
 *
 * @param terminal
 *    The terminal to wait on.
 *
 * @param msec_timeout
 *    The maximum amount of time to wait, in milliseconds.
 *
 * @return
 *    Non-zero if the terminal has been modified, zero if the timeout has
 *    elapsed without the terminal being modified.
 */
static int guac_terminal_wait(guac_terminal* terminal, int msec_timeout) {

    int retval = guac_flag_timedwait_and_lock(&terminal->modified,
            GUAC_TERMINAL_MODIFIED, msec_timeout);

    /* Rest terminal modified state */
    if (retval) {
        guac_flag_clear(&terminal->modified, GUAC_TERMINAL_MODIFIED);
        guac_flag_unlock(&terminal->modified);
    }

    return retval;

}

int guac_terminal_render_frame(guac_terminal* terminal) {

    guac_client* client = terminal->client;

    int wait_result;

    /* Wait for data to be available */
    wait_result = guac_terminal_wait(terminal, 1000);
    if (wait_result || !terminal->started) {

        guac_timestamp frame_start = client->last_sent_timestamp;

        do {

            /* Calculate time remaining in frame */
            guac_timestamp frame_end = guac_timestamp_current();
            int frame_remaining = frame_start + GUAC_TERMINAL_FRAME_DURATION
                                - frame_end;

            /* Wait again if frame remaining */
            if (frame_remaining > 0 || !terminal->started)
                wait_result = guac_terminal_wait(terminal,
                        GUAC_TERMINAL_FRAME_TIMEOUT);
            else
                break;

        } while (client->state == GUAC_CLIENT_RUNNING
                && (wait_result > 0 || !terminal->started));

        /* Flush terminal */
        guac_terminal_lock(terminal);
        guac_terminal_flush(terminal);
        guac_terminal_unlock(terminal);

    }

    return 0;

}

int guac_terminal_read_stdin(guac_terminal* terminal, char* c, int size) {
    int stdin_fd = terminal->stdin_pipe_fd[0];
    int retval;

    GUAC_RETRY_EINTR(retval, read(stdin_fd, c, size));
    return retval;
}

void guac_terminal_notify(guac_terminal* terminal) {

    /* Signal modification */
    guac_flag_set(&terminal->modified, GUAC_TERMINAL_MODIFIED);

}

int guac_terminal_printf(guac_terminal* terminal, const char* format, ...) {

    int written;

    va_list ap;
    char buffer[1024];

    /* Print to buffer */
    va_start(ap, format);
    written = vsnprintf(buffer, sizeof(buffer)-1, format, ap);
    va_end(ap);

    if (written < 0)
        return written;

    /* Write to STDOUT */
    return guac_terminal_write(terminal, buffer, written);

}

char* guac_terminal_prompt(guac_terminal* terminal, const char* title,
        bool echo) {

    char buffer[1024];

    int pos;
    char in_byte;

    /* Prompting implicitly requires user input */
    guac_terminal_start(terminal);

    /* Print title */
    guac_terminal_printf(terminal, "%s", title);

    /* Read bytes until newline */
    pos = 0;
    while (guac_terminal_read_stdin(terminal, &in_byte, 1) == 1) {

        /* Backspace */
        if (in_byte == 0x7F) {
            if (pos > 0) {
                guac_terminal_printf(terminal, "\b \b");
                pos--;
            }
        }

        /* CR (end of input */
        else if (in_byte == 0x0D) {
            guac_terminal_printf(terminal, "\r\n");
            break;
        }

        /* Otherwise, store byte if there is room */
        else if (pos < sizeof(buffer) - 1) {

            /* Store character, update buffers */
            buffer[pos++] = in_byte;

            /* Print character if echoing */
            if (echo)
                guac_terminal_printf(terminal, "%c", in_byte);
            else
                guac_terminal_printf(terminal, "*");

        }

        /* Ignore all other input */

    }

    /* Terminate string */
    buffer[pos] = 0;

    /* Return newly-allocated string containing result */
    return guac_strdup(buffer);

}

int guac_terminal_set(guac_terminal* term, int row, int col, int codepoint) {

    /* Calculate width in columns */
    int width = wcwidth(codepoint);
    if (width < 0)
        width = 1;

    /* Do nothing if glyph is empty */
    else if (width == 0)
        return 0;

    /* Build character with current attributes */
    guac_terminal_char guac_char = {
        .value      = codepoint,
        .attributes = term->current_attributes,
        .width      = width
    };

    guac_terminal_set_columns(term, row, col, col + width - 1, &guac_char);

    return 0;

}

void guac_terminal_commit_cursor(guac_terminal* term) {

    /* If no change, done */
    if (term->cursor_visible && term->visible_cursor_row == term->cursor_row && term->visible_cursor_col == term->cursor_col)
        return;

    /* Clear cursor if it was visible */
    if (term->visible_cursor_row != -1 && term->visible_cursor_col != -1) {

        guac_terminal_buffer_set_cursor(term->current_buffer, term->visible_cursor_row, term->visible_cursor_col, false);

        guac_terminal_char* characters;
        int length = guac_terminal_buffer_get_columns(term->current_buffer, &characters, NULL, term->visible_cursor_row);
        if (term->visible_cursor_col < length)
            guac_terminal_display_set_columns(term->display, term->visible_cursor_row + term->scroll_offset,
                    term->visible_cursor_col, term->visible_cursor_col, &characters[term->visible_cursor_col]);

    }

    /* Set cursor if should be visible */
    if (term->cursor_visible) {

        guac_terminal_buffer_set_cursor(term->current_buffer, term->cursor_row, term->cursor_col, true);

        guac_terminal_char* characters;
        int length = guac_terminal_buffer_get_columns(term->current_buffer, &characters, NULL, term->cursor_row);
        if (term->cursor_col < length)
            guac_terminal_display_set_columns(term->display, term->cursor_row + term->scroll_offset,
                    term->cursor_col, term->cursor_col, &characters[term->cursor_col]);

        term->visible_cursor_row = term->cursor_row;
        term->visible_cursor_col = term->cursor_col;

    }

    /* Otherwise set visible position to a sentinel value */
    else {
        term->visible_cursor_row = -1;
        term->visible_cursor_col = -1;
    }

    return;

}

int guac_terminal_write(guac_terminal* term, const char* buffer, int length) {

    guac_terminal_lock(term);
    for (int written = 0; written < length; written++) {

        /* Read and advance to next character */
        char current = *(buffer++);

        /* Write character to typescript, if any */
        if (term->typescript != NULL)
            guac_terminal_typescript_write(term->typescript, current);

        /* Handle character and its meaning */
        term->char_handler(term, current);

    }
    guac_terminal_unlock(term);

    guac_terminal_notify(term);
    return length;

}

void guac_terminal_scroll_up(guac_terminal* term,
        int start_row, int end_row, int amount) {

    if (amount <= 0)
        return;

    if (amount >= end_row - start_row + 1)
        amount = end_row - start_row + 1;

    /* If scrolling entire display, update scroll offset */
    if (start_row == 0 && end_row == term->term_height - 1) {

        /* Scroll up visibly */
        guac_terminal_display_copy_rows(term->display, start_row + amount, end_row, -amount);

        /* Advance and increase buffer length by scroll amount */
        guac_terminal_buffer_scroll_up(term->current_buffer, amount, true);

        /* Reset scrollbar bounds */
        guac_terminal_scrollbar_set_bounds(term->scrollbar,
                -guac_terminal_get_available_scroll(term), 0);

        /* Update cursor location if within region */
        if (term->visible_cursor_row >= start_row &&
            term->visible_cursor_row <= end_row)
            term->visible_cursor_row -= amount;

        /* Update selected region */
        if (term->text_selected) {
            term->selection_start_row -= amount;
            term->selection_end_row -= amount;
        }

    }

    /* Otherwise, just copy row data upwards */
    else
        guac_terminal_copy_rows(term, start_row + amount, end_row, -amount);

    /* Clear new area */
    guac_terminal_clear_range(term,
            end_row - amount + 1, 0,
            end_row, term->term_width - 1);

}

void guac_terminal_scroll_down(guac_terminal* term,
        int start_row, int end_row, int amount) {

    guac_terminal_copy_rows(term, start_row, end_row - amount, amount);

    /* Clear new area */
    guac_terminal_clear_range(term,
            start_row, 0,
            start_row + amount - 1, term->term_width - 1);

}

int guac_terminal_clear_columns(guac_terminal* term,
        int row, int start_col, int end_col) {

    /* Build space */
    guac_terminal_char blank;
    blank.value = 0;
    blank.attributes = term->current_attributes;
    blank.width = 1;

    /* Clear */
    guac_terminal_set_columns(term,
        row, start_col, end_col, &blank);

    return 0;

}

int guac_terminal_clear_range(guac_terminal* term,
        int start_row, int start_col,
        int end_row, int end_col) {

    /* If not at far left, must clear sub-region to far right */
    if (start_col > 0) {

        /* Clear from start_col to far right */
        guac_terminal_clear_columns(term,
            start_row, start_col, term->term_width - 1);

        /* One less row to clear */
        start_row++;
    }

    /* If not at far right, must clear sub-region to far left */
    if (end_col < term->term_width - 1) {

        /* Clear from far left to end_col */
        guac_terminal_clear_columns(term, end_row, 0, end_col);

        /* One less row to clear */
        end_row--;

    }

    /* Remaining region now guaranteed rectangular. Clear, if possible */
    if (start_row <= end_row) {

        int row;
        for (row=start_row; row<=end_row; row++) {

            /* Clear entire row */
            guac_terminal_clear_columns(term, row, 0, term->term_width - 1);

        }

    }

    return 0;

}

/**
 * Returns whether the given character would be visible relative to the
 * background of the given terminal.
 *
 * @param term
 *     The guac_terminal to test the character against.
 *
 * @param c
 *     The character being tested.
 *
 * @return
 *     true if the given character is different from the terminal background,
 *     false otherwise.
 */
static bool guac_terminal_is_visible(guac_terminal* term,
        guac_terminal_char* c) {

    /* Continuation characters are NEVER visible */
    if (c->value == GUAC_CHAR_CONTINUATION)
        return false;

    /* Characters with glyphs are ALWAYS visible */
    if (guac_terminal_has_glyph(c->value))
        return true;

    const guac_terminal_color* background;

    /* Determine actual background color of character */
    if (c->attributes.reverse != c->attributes.cursor)
        background = &c->attributes.foreground;
    else
        background = &c->attributes.background;

    /* Blank characters are visible if their background color differs from that
     * of the terminal */
    return guac_terminal_colorcmp(background,
            &term->default_char.attributes.background) != 0;

}

void guac_terminal_scroll_display_down(guac_terminal* terminal,
        int scroll_amount) {

    int start_row, end_row;
    int dest_row;
    int row, column;

    /* Limit scroll amount by size of scrollback buffer */
    if (scroll_amount > terminal->scroll_offset)
        scroll_amount = terminal->scroll_offset;

    /* If not scrolling at all, don't bother trying */
    if (scroll_amount <= 0)
        return;

    /* Shift screen up */
    if (terminal->term_height > scroll_amount)
        guac_terminal_display_copy_rows(terminal->display,
                scroll_amount, terminal->term_height - 1,
                -scroll_amount);

    /* Advance by scroll amount */
    terminal->scroll_offset -= scroll_amount;
    guac_terminal_scrollbar_set_value(terminal->scrollbar, -terminal->scroll_offset);

    /* Get row range */
    end_row   = terminal->term_height - terminal->scroll_offset - 1;
    start_row = end_row - scroll_amount + 1;
    dest_row  = terminal->term_height - scroll_amount;

    /* Draw new rows from scrollback */
    for (row=start_row; row<=end_row; row++) {

        /* Get row from scrollback */
        guac_terminal_char* characters;
        int length = guac_terminal_buffer_get_columns(terminal->current_buffer, &characters, NULL, row);

        /* Clear row */
        guac_terminal_display_set_columns(terminal->display,
                dest_row, 0, terminal->display->width, &(terminal->default_char));

        /* Draw row */
        guac_terminal_char* current = characters;
        for (column = 0; column < length; column++) {

            /* Only draw if not blank */
            if (guac_terminal_is_visible(terminal, current))
                guac_terminal_display_set_columns(terminal->display, dest_row, column, column, current);

            current++;

        }

        /* Next row */
        dest_row++;

    }

    guac_terminal_notify(terminal);

}

void guac_terminal_scroll_display_up(guac_terminal* terminal,
        int scroll_amount) {

    int start_row, end_row;
    int dest_row;
    int row, column;

    /* Limit scroll amount by size of scrollback buffer */
    int available_scroll = guac_terminal_get_available_scroll(terminal);
    if (terminal->scroll_offset + scroll_amount > available_scroll)
        scroll_amount = available_scroll - terminal->scroll_offset;

    /* If not scrolling at all, don't bother trying */
    if (scroll_amount <= 0)
        return;

    /* Shift screen down */
    if (terminal->term_height > scroll_amount)
        guac_terminal_display_copy_rows(terminal->display,
                0, terminal->term_height - scroll_amount - 1,
                scroll_amount);

    /* Advance by scroll amount */
    terminal->scroll_offset += scroll_amount;
    guac_terminal_scrollbar_set_value(terminal->scrollbar, -terminal->scroll_offset);

    /* Get row range */
    start_row = -terminal->scroll_offset;
    end_row   = start_row + scroll_amount - 1;
    dest_row  = 0;

    /* Draw new rows from scrollback */
    for (row=start_row; row<=end_row; row++) {

        /* Get row from scrollback */
        guac_terminal_char* characters;
        int length = guac_terminal_buffer_get_columns(terminal->current_buffer, &characters, NULL, row);

        /* Clear row */
        guac_terminal_display_set_columns(terminal->display,
                dest_row, 0, terminal->display->width, &(terminal->default_char));

        /* Draw row */
        guac_terminal_char* current = characters;
        for (column = 0; column < length; column++) {

            /* Only draw if not blank */
            if (guac_terminal_is_visible(terminal, current))
                guac_terminal_display_set_columns(terminal->display, dest_row, column, column, current);

            current++;

        }

        /* Next row */
        dest_row++;

    }

    guac_terminal_notify(terminal);

}

void guac_terminal_copy_columns(guac_terminal* terminal, int row,
        int start_column, int end_column, int offset) {

    guac_terminal_display_copy_columns(terminal->display, row + terminal->scroll_offset,
            start_column, end_column, offset);

    guac_terminal_buffer_copy_columns(terminal->current_buffer, row,
            start_column, end_column, offset);

    /* Clear selection if region is modified */
    guac_terminal_select_touch(terminal, row, start_column, row, end_column);

    /* Update cursor location if within region */
    if (row == terminal->visible_cursor_row &&
            terminal->visible_cursor_col >= start_column &&
            terminal->visible_cursor_col <= end_column)
        terminal->visible_cursor_col += offset;

}

void guac_terminal_copy_rows(guac_terminal* terminal,
        int start_row, int end_row, int offset) {

    guac_terminal_display_copy_rows(terminal->display,
            start_row + terminal->scroll_offset, end_row + terminal->scroll_offset, offset);

    guac_terminal_buffer_copy_rows(terminal->current_buffer,
            start_row, end_row, offset);

    /* Clear selection if region is modified */
    guac_terminal_select_touch(terminal, start_row, 0, end_row,
            terminal->term_width);

    /* Update cursor location if within region */
    if (terminal->visible_cursor_row >= start_row &&
        terminal->visible_cursor_row <= end_row)
        terminal->visible_cursor_row += offset;

}

void guac_terminal_set_columns(guac_terminal* terminal, int row,
        int start_column, int end_column, guac_terminal_char* character) {

    __guac_terminal_set_columns(terminal, row, start_column, end_column, character);

    /* If visible cursor in current row, preserve state */
    if (row == terminal->visible_cursor_row
            && terminal->visible_cursor_col >= start_column
            && terminal->visible_cursor_col <= end_column) {

        /* Create copy of character with cursor attribute set */
        guac_terminal_char cursor_character = *character;
        cursor_character.attributes.cursor = true;

        __guac_terminal_set_columns(terminal, row,
                terminal->visible_cursor_col, terminal->visible_cursor_col, &cursor_character);

    }

}

static void __guac_terminal_redraw_rect(guac_terminal* term, int start_row, int start_col, int end_row, int end_col) {

    int row, col;

    /* Redraw region */
    for (row=start_row; row<=end_row; row++) {

        guac_terminal_char* characters;
        int length = guac_terminal_buffer_get_columns(term->current_buffer, &characters, NULL, row - term->scroll_offset);

        /* Clear row */
        guac_terminal_display_set_columns(term->display,
                row, start_col, end_col, &(term->default_char));

        /* Copy characters */
        for (col=start_col; col <= end_col && col < length; col++) {

            /* Only redraw if not blank */
            guac_terminal_char* c = &characters[col];
            if (guac_terminal_is_visible(term, c))
                guac_terminal_display_set_columns(term->display, row, col, col, c);

        }

    }

}

/**
 * Internal terminal resize routine. Accepts width/height in CHARACTERS
 * (not pixels like the public function).
 *
 * @param term
 *     The terminal being resized.
 *
 * @param width
 *     The new width of the terminal, in characters.
 *
 * @param height
 *     The new height of the terminal, in characters.
 */
static void __guac_terminal_resize(guac_terminal* term, int width, int height) {

    /* If height is decreasing, shift display up */
    if (height < term->term_height) {

        int shift_amount;

        /* Get number of rows actually occupying terminal space */
        int used_height = guac_terminal_effective_buffer_length(term);
        if (used_height > term->term_height)
            used_height = term->term_height;

        shift_amount = used_height - height;

        /* If the new terminal bottom covers N rows, shift up N rows */
        if (shift_amount > 0) {

            guac_terminal_display_copy_rows(term->display,
                    shift_amount, term->display->height - 1, -shift_amount);

            /* Update buffer top and cursor row based on shift */
            guac_terminal_buffer_scroll_up(term->current_buffer, shift_amount, false);
            term->cursor_row  -= shift_amount;
            if (term->visible_cursor_row != -1)
                term->visible_cursor_row -= shift_amount;

            /* Redraw characters within old region */
            __guac_terminal_redraw_rect(term, height - shift_amount, 0, height-1, width-1);

        }

    }

    /* Resize display */
    guac_terminal_display_flush(term->display);
    guac_terminal_display_resize(term->display, width, height);

    /* Redraw any characters on right if widening */
    if (width > term->term_width)
        __guac_terminal_redraw_rect(term, 0, term->term_width-1, height-1, width-1);

    /* If height is increasing, shift display down */
    if (height > term->term_height) {

        /* If undisplayed rows exist in the buffer, shift them into view */
        int available_scroll = guac_terminal_get_available_scroll(term);
        if (available_scroll > 0) {

            /* If the new terminal bottom reveals N rows, shift down N rows */
            int shift_amount = height - term->term_height;

            /* The maximum amount we can shift is the number of undisplayed rows */
            if (shift_amount > available_scroll)
                shift_amount = available_scroll;

            /* Update buffer top and cursor row based on shift */
            guac_terminal_buffer_scroll_down(term->current_buffer, shift_amount);
            term->cursor_row  += shift_amount;
            if (term->visible_cursor_row != -1)
                term->visible_cursor_row += shift_amount;

            /* If scrolled enough, use scroll to fulfill entire resize */
            if (term->scroll_offset >= shift_amount) {

                term->scroll_offset -= shift_amount;
                guac_terminal_scrollbar_set_value(term->scrollbar, -term->scroll_offset);

                /* Draw characters from scroll at bottom */
                __guac_terminal_redraw_rect(term, term->term_height, 0, term->term_height + shift_amount - 1, width-1);

            }

            /* Otherwise, fulfill with as much scroll as possible */
            else {

                /* Draw characters from scroll at bottom */
                __guac_terminal_redraw_rect(term, term->term_height, 0, term->term_height + term->scroll_offset - 1, width-1);

                /* Update shift_amount and scroll based on new rows */
                shift_amount -= term->scroll_offset;
                term->scroll_offset = 0;
                guac_terminal_scrollbar_set_value(term->scrollbar, -term->scroll_offset);

                /* If anything remains, move screen as necessary */
                if (shift_amount > 0) {

                    guac_terminal_display_copy_rows(term->display,
                            0, term->display->height - shift_amount - 1, shift_amount);

                    /* Draw characters at top from scroll */
                    __guac_terminal_redraw_rect(term, 0, 0, shift_amount - 1, width-1);

                    /* Draw characters from scroll at bottom */
                    __guac_terminal_redraw_rect(term, term->display->height - shift_amount,
                            0, term->display->height - 1, width-1);

                }

            }

        } /* end if undisplayed rows exist */

    }

    /* Keep cursor on screen */
    if (term->cursor_row < 0)       term->cursor_row = 0;
    if (term->cursor_row >= height) term->cursor_row = height-1;
    if (term->cursor_col < 0)       term->cursor_col = 0;
    if (term->cursor_col >= width)  term->cursor_col = width-1;

    /* Commit new dimensions */
    term->term_width = width;
    term->term_height = height;

}

int guac_terminal_resize(guac_terminal* terminal, int width, int height) {

    guac_terminal_display* display = terminal->display;
    guac_client* client = display->client;

    /* Acquire exclusive access to terminal */
    guac_terminal_lock(terminal);

    /* Calculate available text display area by character size */
    int rows, columns;
    calculate_rows_and_columns(terminal, height, width, &rows, &columns);

    /* Calculate available display area in pixels */
    int adjusted_height = height;
    int adjusted_width = width;
    calculate_height_and_width(terminal, rows, columns,
        &adjusted_height, &adjusted_width);

    /* Set size of available screen area */
    terminal->outer_height = height;
    terminal->outer_width = width;

    /* Set pixel size */
    terminal->height = adjusted_height;
    terminal->width = adjusted_width;

    /* Resize default layer to given pixel dimensions */
    guac_terminal_repaint_default_layer(terminal, client->socket);

    /* Resize terminal if row/column dimensions have changed */
    if (columns != terminal->term_width || rows != terminal->term_height) {
        /* Resize terminal and set the columns and rows on the terminal struct */
        __guac_terminal_resize(terminal, columns, rows);

        /* Reset scroll region */
        terminal->scroll_end = rows - 1;
    }

    /* Notify scrollbar of resize */
    guac_terminal_scrollbar_parent_resized(terminal->scrollbar,
        terminal->outer_width, terminal->outer_height, terminal->term_height);
    guac_terminal_scrollbar_set_bounds(terminal->scrollbar,
            -guac_terminal_get_available_scroll(terminal), 0);

    /* Release terminal */
    guac_terminal_unlock(terminal);

    guac_terminal_notify(terminal);
    return 0;

}

void guac_terminal_flush(guac_terminal* terminal) {

    /* Flush typescript if in use */
    if (terminal->typescript != NULL)
        guac_terminal_typescript_flush(terminal->typescript);

    /* Flush pipe stream if automatic flushing is enabled */
    if (terminal->pipe_stream_flags & GUAC_TERMINAL_PIPE_AUTOFLUSH)
        guac_terminal_pipe_stream_flush(terminal);

    /* Flush display state */
    guac_terminal_select_redraw(terminal);
    guac_terminal_commit_cursor(terminal);
    guac_terminal_display_flush(terminal->display);
    guac_terminal_scrollbar_flush(terminal->scrollbar);

}

void guac_terminal_lock(guac_terminal* terminal) {
    pthread_mutex_lock(&(terminal->lock));
}

void guac_terminal_unlock(guac_terminal* terminal) {
    pthread_mutex_unlock(&(terminal->lock));
}

int guac_terminal_send_data(guac_terminal* term, const char* data, int length) {

    /* Block all other sources of input if input is coming from a stream */
    if (term->input_stream != NULL)
        return 0;

    return guac_terminal_write_all(term->stdin_pipe_fd[1], data, length);

}

int guac_terminal_send_string(guac_terminal* term, const char* data) {

    /* Block all other sources of input if input is coming from a stream */
    if (term->input_stream != NULL)
        return 0;

    return guac_terminal_write_all(term->stdin_pipe_fd[1], data, strlen(data));

}

/**
 * Returns non-zero if the given keysym is an arrow key, including keypad
 * arrow variants.
 *
 * @param keysym
 *     The X11 keysym to test.
 *
 * @return
 *     Non-zero if the keysym is an arrow key, zero otherwise.
 */
static int __guac_terminal_is_arrow_keysym(int keysym) {
    return keysym == GUAC_KEYSYM_LEFT || keysym == GUAC_KEYSYM_KP_LEFT
        || keysym == GUAC_KEYSYM_UP || keysym == GUAC_KEYSYM_KP_UP
        || keysym == GUAC_KEYSYM_RIGHT || keysym == GUAC_KEYSYM_KP_RIGHT
        || keysym == GUAC_KEYSYM_DOWN || keysym == GUAC_KEYSYM_KP_DOWN;
}

/**
 * Returns non-zero if the given keysym is a function key (F1-F12), including
 * keypad function-key variants (KP_F1-KP_F4).
 *
 * @param keysym
 *     The X11 keysym to test.
 *
 * @return
 *     Non-zero if the keysym is a function key, zero otherwise.
 */
static int __guac_terminal_is_function_keysym(int keysym) {
    return keysym == GUAC_KEYSYM_F1 || keysym == GUAC_KEYSYM_KP_F1
        || keysym == GUAC_KEYSYM_F2 || keysym == GUAC_KEYSYM_KP_F2
        || keysym == GUAC_KEYSYM_F3 || keysym == GUAC_KEYSYM_KP_F3
        || keysym == GUAC_KEYSYM_F4 || keysym == GUAC_KEYSYM_KP_F4
        || keysym == GUAC_KEYSYM_F5
        || keysym == GUAC_KEYSYM_F6
        || keysym == GUAC_KEYSYM_F7
        || keysym == GUAC_KEYSYM_F8
        || keysym == GUAC_KEYSYM_F9
        || keysym == GUAC_KEYSYM_F10
        || keysym == GUAC_KEYSYM_F11
        || keysym == GUAC_KEYSYM_F12;
}

/**
 * Returns non-zero if the given keysym is a cursor editing key (Home, End,
 * Insert, Delete, Page Up, or Page Down), including keypad variants.
 *
 * @param keysym
 *     The X11 keysym to test.
 *
 * @return
 *     Non-zero if the keysym is a cursor editing key, zero otherwise.
 */
static int __guac_terminal_is_editing_keysym(int keysym) {
    return keysym == GUAC_KEYSYM_HOME    || keysym == GUAC_KEYSYM_KP_HOME
        || keysym == GUAC_KEYSYM_END     || keysym == GUAC_KEYSYM_KP_END
        || keysym == GUAC_KEYSYM_INSERT  || keysym == GUAC_KEYSYM_KP_INSERT
        || keysym == GUAC_KEYSYM_DELETE  || keysym == GUAC_KEYSYM_KP_DELETE
        || keysym == GUAC_KEYSYM_PAGE_UP || keysym == GUAC_KEYSYM_KP_PAGE_UP
        || keysym == GUAC_KEYSYM_PAGE_DOWN || keysym == GUAC_KEYSYM_KP_PAGE_DOWN;
}

/**
 * Returns the xterm-style modifier parameter for CSI key sequences, encoding
 * the current modifier state of the terminal. The value is 1 plus the bitmask
 * of active modifiers: Shift=1, Alt=2, Ctrl=4, Meta=8.
 *
 * @param term
 *     The terminal whose current modifier state should be encoded.
 *
 * @return
 *     The encoded modifier parameter. Always at least 1.
 */
static int __guac_terminal_key_modifier_param(guac_terminal* term) {
    return 1 + (term->mod_shift ? 1 : 0)
             + (term->mod_alt   ? 2 : 0)
             + (term->mod_ctrl  ? 4 : 0)
             + (term->mod_meta  ? 8 : 0);
}

/**
 * Returns non-zero if any modifier key (Shift, Ctrl, Alt, or Meta) is
 * currently held in the given terminal.
 *
 * @param term
 *     The terminal whose modifier state should be tested.
 *
 * @return
 *     Non-zero if any modifier is held, zero otherwise.
 */
static int __guac_terminal_any_modifier(guac_terminal* term) {
    return term->mod_shift || term->mod_ctrl || term->mod_alt || term->mod_meta;
}

/**
 * Sends the xterm-style CSI sequence for a modified arrow key. The resulting
 * sequence has the form "ESC [ 1 ; mod suffix", where the suffix is A, B, C,
 * or D for Up, Down, Right, or Left, respectively.
 *
 * @param term
 *     The terminal from which key input is being sent. Current modifier state
 *     is read from this terminal.
 *
 * @param keysym
 *     The X11 keysym of the arrow key being sent.
 *
 * @return
 *     Zero if the key was sent successfully, non-zero on error.
 */
static int __guac_terminal_send_modified_arrow(guac_terminal* term, int keysym) {

    char suffix;

    if (keysym == GUAC_KEYSYM_LEFT || keysym == GUAC_KEYSYM_KP_LEFT) suffix = 'D';
    else if (keysym == GUAC_KEYSYM_UP || keysym == GUAC_KEYSYM_KP_UP) suffix = 'A';
    else if (keysym == GUAC_KEYSYM_RIGHT || keysym == GUAC_KEYSYM_KP_RIGHT) suffix = 'C';
    else if (keysym == GUAC_KEYSYM_DOWN || keysym == GUAC_KEYSYM_KP_DOWN) suffix = 'B';
    else return 0;

    return guac_terminal_sendf(term, "\x1B[1;%i%c",
                               __guac_terminal_key_modifier_param(term), suffix);

}

/**
 * Sends the CSI sequence for a modified cursor editing key (Home, End,
 * Insert, Delete, Page Up, or Page Down). Home and End use H/F suffixes in
 * xterm mode or tilde sequences in linux-console mode. All other supported
 * keys use the form "ESC [ param ; mod ~".
 *
 * @param term
 *     The terminal from which key input is being sent. Current modifier state
 *     and linux_console_keys flag are read from this terminal.
 *
 * @param keysym
 *     The X11 keysym of the cursor editing key being sent.
 *
 * @return
 *     Zero if the key was sent successfully, non-zero on error.
 */
static int __guac_terminal_send_modified_editing(guac_terminal* term, int keysym) {

    int mod = __guac_terminal_key_modifier_param(term);

    /* Home and End use letter suffixes in xterm, tilde params in linux console */
    if (keysym == GUAC_KEYSYM_HOME || keysym == GUAC_KEYSYM_KP_HOME)
        return term->linux_console_keys
            ? guac_terminal_sendf(term, "\x1B[1;%i~", mod)
            : guac_terminal_sendf(term, "\x1B[1;%iH", mod);

    if (keysym == GUAC_KEYSYM_END || keysym == GUAC_KEYSYM_KP_END)
        return term->linux_console_keys
            ? guac_terminal_sendf(term, "\x1B[4;%i~", mod)
            : guac_terminal_sendf(term, "\x1B[1;%iF", mod);

    int param;

    if (keysym == GUAC_KEYSYM_INSERT || keysym == GUAC_KEYSYM_KP_INSERT) param = 2;
    else if (keysym == GUAC_KEYSYM_DELETE || keysym == GUAC_KEYSYM_KP_DELETE) param = 3;
    else if (keysym == GUAC_KEYSYM_PAGE_UP || keysym == GUAC_KEYSYM_KP_PAGE_UP) param = 5;
    else if (keysym == GUAC_KEYSYM_PAGE_DOWN || keysym == GUAC_KEYSYM_KP_PAGE_DOWN) param = 6;
    else return 0;

    return guac_terminal_sendf(term, "\x1B[%i;%i~", param, mod);

}

/**
 * Sends the xterm-style CSI sequence for a modified function key. Modified
 * F1-F4 use the form "ESC [ 1 ; mod suffix", with suffix P, Q, R, or S.
 * Modified F5-F12 use the form "ESC [ param ; mod ~".
 *
 * Example:
 *   Alt+F1 => ESC [ 1 ; 3 P
 *   Ctrl+F6 => ESC [ 17 ; 5 ~
 *
 * Reference:
 *   https://invisible-island.net/xterm/ctlseqs/ctlseqs.html
 *
 * @param term
 *     The terminal from which key input is being sent. Current modifier state
 *     is read from this terminal.
 *
 * @param keysym
 *     The X11 keysym of the function key being sent.
 *
 * @return
 *     Zero if the key was sent successfully, non-zero on error.
 */
static int __guac_terminal_send_modified_function(guac_terminal* term, int keysym) {

    char suffix = 0;
    int param = 0;

    if (keysym == GUAC_KEYSYM_F1 || keysym == GUAC_KEYSYM_KP_F1) suffix = 'P';
    else if (keysym == GUAC_KEYSYM_F2 || keysym == GUAC_KEYSYM_KP_F2) suffix = 'Q';
    else if (keysym == GUAC_KEYSYM_F3 || keysym == GUAC_KEYSYM_KP_F3) suffix = 'R';
    else if (keysym == GUAC_KEYSYM_F4 || keysym == GUAC_KEYSYM_KP_F4) suffix = 'S';
    else if (keysym == GUAC_KEYSYM_F5) param = 15;
    else if (keysym == GUAC_KEYSYM_F6) param = 17;
    else if (keysym == GUAC_KEYSYM_F7) param = 18;
    else if (keysym == GUAC_KEYSYM_F8) param = 19;
    else if (keysym == GUAC_KEYSYM_F9) param = 20;
    else if (keysym == GUAC_KEYSYM_F10) param = 21;
    /* Intentional gap */
    else if (keysym == GUAC_KEYSYM_F11) param = 23;
    else if (keysym == GUAC_KEYSYM_F12) param = 24;
    else return 0;

    int mod = __guac_terminal_key_modifier_param(term);

    /* Modified F1-F4 use CSI with letter suffix (P/Q/R/S); modified F5-F12
     * use CSI tilde with numeric param. The `else return 0` above guarantees
     * that exactly one of suffix or param is non-zero when this point is
     * reached. */
    if (suffix != 0)
        return guac_terminal_sendf(term, "\x1B[1;%i%c", mod, suffix);

    return guac_terminal_sendf(term, "\x1B[%i;%i~", param, mod);

}

static int __guac_terminal_send_key(guac_terminal* term, int keysym, int pressed) {

    /* Ignore user input if terminal is not started */
    if (!term->started) {
        guac_client_log(term->client, GUAC_LOG_DEBUG, "Ignoring user input "
                "while terminal has not yet started.");
        return 0;
    }

    /* Hide mouse cursor if not already hidden */
    if (term->current_cursor != GUAC_TERMINAL_CURSOR_BLANK) {
        term->current_cursor = GUAC_TERMINAL_CURSOR_BLANK;
        guac_common_cursor_set_blank(term->cursor);
        guac_terminal_notify(term);
    }

    /* Track modifiers */
    /*
     * Super (Windows/Command) and Hyper are treated as Meta since terminals don't
     * have separate modifier bits for them.
     *
     * MODE_SWITCH and the ISO level selectors are intentionally not treated as
     * Alt. On international layouts these are often used to produce printable
     * third-/fifth-level characters, and folding them into Alt would inject an
     * unwanted ESC prefix into normal text input.
     */
    if (keysym == GUAC_KEYSYM_CTRL_L || keysym == GUAC_KEYSYM_CTRL_R)
        term->mod_ctrl = pressed;
    else if (keysym == GUAC_KEYSYM_META_L || keysym == GUAC_KEYSYM_META_R
            || keysym == GUAC_KEYSYM_SUPER_L || keysym == GUAC_KEYSYM_SUPER_R
            || keysym == GUAC_KEYSYM_HYPER_L || keysym == GUAC_KEYSYM_HYPER_R)
        term->mod_meta = pressed;
    else if (keysym == GUAC_KEYSYM_ALT_L || keysym == GUAC_KEYSYM_ALT_R)
        term->mod_alt = pressed;
    else if (keysym == GUAC_KEYSYM_SHIFT_L || keysym == GUAC_KEYSYM_SHIFT_R)
        term->mod_shift = pressed;

    /* If key pressed */
    else if (pressed) {

        /* Ctrl+Shift+V or Cmd+v (mac style) shortcuts for paste */
        if ((keysym == 'V' && term->mod_ctrl) || (keysym == 'v' && term->mod_meta))
            return guac_terminal_send_data(term, term->clipboard->buffer, term->clipboard->length);

        /* If Shift+Tab (Backtab), send the appropriate escape sequence */
        if (term->mod_shift && keysym == 0xFF09) {
            return guac_terminal_send_string(term, "\x1B[Z");
        }

        /*
         * Ctrl+Shift+C and Cmd+c shortcuts for copying are not handled, as
         * selecting text in the terminal automatically copies it. To avoid
         * attempts to use these shortcuts causing unexpected results in the
         * terminal, these are just ignored.
         */
        if ((keysym == 'C' && term->mod_ctrl) || (keysym == 'c' && term->mod_meta))
            return 0;

        /* Shortcuts for scrolling history with shift */
        if (term->mod_shift) {

            /* Home */
            if (keysym == GUAC_TERMINAL_KEY_HOME || keysym == GUAC_TERMINAL_KEY_KP_HOME) {
                guac_terminal_scroll_display_up(term, term->max_scrollback);
                return 0;
            }

            /* End */
            if (keysym == GUAC_TERMINAL_KEY_END || keysym == GUAC_TERMINAL_KEY_KP_END) {
                guac_terminal_scroll_display_down(term, term->max_scrollback);
                return 0;
            }

            /* Page up */
            if (keysym == GUAC_TERMINAL_KEY_PAGEUP || keysym == GUAC_TERMINAL_KEY_KP_PAGEUP) {
                guac_terminal_scroll_display_up(term, term->term_height);
                return 0;
            }

            /* Page down */
            if (keysym == GUAC_TERMINAL_KEY_PAGEDOWN || keysym == GUAC_TERMINAL_KEY_KP_PAGEDOWN) {
                guac_terminal_scroll_display_down(term, term->term_height);
                return 0;
            }

            /* Up */
            if (keysym == GUAC_TERMINAL_KEY_UP || keysym == GUAC_TERMINAL_KEY_KP_UP) {
                guac_terminal_scroll_display_up(term, 1);
                return 0;
            }

            /* Down */
            if (keysym == GUAC_TERMINAL_KEY_DOWN || keysym == GUAC_TERMINAL_KEY_KP_DOWN) {
                guac_terminal_scroll_display_down(term, 1);
                return 0;
            }

        }

        /* Reset scroll */
        if (term->scroll_offset != 0)
            guac_terminal_scroll_display_down(term, term->scroll_offset);

        /* If alt being held, also send escape character */
        if (term->mod_alt)
            guac_terminal_send_string(term, GUAC_TERMINAL_ASCII_ESCAPE);

        /* Translate Ctrl+letter to control code */
        if (term->mod_ctrl) {

            char data;

            /* Keysyms for '@' through '_' are all conveniently in C0 order */
            if (keysym >= '@' && keysym <= '_')
                data = (char) (keysym - '@');

            /* Handle lowercase as well */
            else if (keysym >= 'a' && keysym <= 'z')
                data = (char) (keysym - 'a' + 1);

            /* Ctrl+? is DEL (0x7f) */
            else if (keysym == '?')
                data = 0x7F;

            /* Map Ctrl+2 to same result as Ctrl+@ */
            else if (keysym == '2')
                data = 0x00;

            /* Map Ctrl+3 through Ctrl-7 to the remaining C0 characters such that Ctrl+6 is the same as Ctrl+^ */
            else if (keysym >= '3' && keysym <= '7')
                data = (char) (keysym - '3' + 0x1B);

            /* CTRL+Left: return to previous word  */
            else if (keysym == GUAC_TERMINAL_KEY_LEFT || keysym == GUAC_TERMINAL_KEY_KP_LEFT)
                return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_PREV_WORD);

            /* CTRL+Right: go to next word */
            else if (keysym == GUAC_TERMINAL_KEY_RIGHT || keysym == GUAC_TERMINAL_KEY_KP_RIGHT)
                return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_NEXT_WORD);

            /* CTRL+Backspace: remove word (map to CTRL+w) */
            else if (keysym == GUAC_TERMINAL_KEY_BACKSPACE)
                data = (char) 23;

            /* CTRL+Supr: remove word to right */
            else if (keysym == GUAC_TERMINAL_KEY_DELETE || keysym == GUAC_TERMINAL_KEY_KP_DELETE)
                return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_DELETE_WORD);

            else {
                /* No C0 mapping — encode as CSI modifier sequence if applicable */
                if (__guac_terminal_is_arrow_keysym(keysym))
                    return __guac_terminal_send_modified_arrow(term, keysym);
                if (__guac_terminal_is_function_keysym(keysym))
                    return __guac_terminal_send_modified_function(term, keysym);
                if (__guac_terminal_is_editing_keysym(keysym))
                    return __guac_terminal_send_modified_editing(term, keysym);
                return 0;

            return guac_terminal_send_data(term, &data, 1);

        }

        /* Translate Unicode to UTF-8 */
        else if ((keysym >= 0x00
                  && keysym <= 0xFF) || ((keysym & GUAC_KEYSYM_UNICODE_MASK) == GUAC_KEYSYM_UNICODE_PREFIX)) {

            int length;
            char data[5];

            length = guac_terminal_encode_utf8(keysym & 0xFFFF, data);
            return guac_terminal_send_data(term, data, length);

        }

        /* Typeable keys of number pad */
        else if (keysym >= 0xFFAA && keysym <= 0xFFB9) {
            char value = keysym - 0xFF80;
            if (term->func_keys_and_keypad == GUAC_TERMINAL_FUNC_KEYS_AND_KEYPAD_VT100 &&
                    term->application_keypad_keys) {
                /* https://vt100.net/docs/vt100-ug/chapter3.html */
                if (value >= '0' && value <= '9') {
                    char data[4];
                    data[0] = '\x1B';
                    data[1] = 'O';
                    data[2] = 'p' + value - '0';
                    data[3] = 0;
                    /* We send sequences from \x1BOp to \x1BOy here */
                    return guac_terminal_send_string(term, data);
                }
                else {
                    switch (value) {
                        case '-':
                            return guac_terminal_send_string(term, "\x1BOm");
                        case '.':
                            return guac_terminal_send_string(term, "\x1BOn");
                        /*
                        * There is no comma on the regular keypad, so we use '*' to send
                        * the appropriate code.
                        */
                        case '*':
                            return guac_terminal_send_string(term, "\x1BOl");
                        /*
                        * At least in some environments JS does not differentiate regular
                        * and keypad ENTER and its location property is always 0. So we
                        * use '+' to send the appropriate ENTER escape code.
                        */
                        case '+':
                            return guac_terminal_send_string(term, "\x1BOM");
                        /* Just a typeable key */
                        default:
                            guac_terminal_send_data(term, &value, sizeof(value));
                            break;
                    }
                }
            }
            else {
                guac_terminal_send_data(term, &value, sizeof(value));
            }
        }

        /* Non-printable keys */
        else {

            /* Backspace can vary based on configuration of terminal by client. */
            if (keysym == GUAC_TERMINAL_KEY_BACKSPACE) {
                char backspace_str[] = { term->backspace, '\0' };
                return guac_terminal_send_string(term, backspace_str);
            }

            if (keysym == 0xFF09 || keysym == 0xFF89) return guac_terminal_send_string(term, "\x09"); /* Tab */
            if (keysym == 0xFF0A) return guac_terminal_send_string(term, "\x0A");                     /* Line Feed */
            if (keysym == 0xFF0D || keysym == 0xFF8D) return guac_terminal_send_string(term, "\x0D"); /* Enter */
            if (keysym == 0xFF1B) return guac_terminal_send_string(term, "\x1B");                     /* Esc */

            /* Tab */
            if (keysym == GUAC_TERMINAL_KEY_TAB || keysym == GUAC_TERMINAL_KEY_KP_TAB)
                return guac_terminal_send_string(term, GUAC_TERMINAL_ASCII_TAB);

            /* Enter */
            if (keysym == GUAC_TERMINAL_KEY_ENTER || keysym == GUAC_TERMINAL_KEY_KP_ENTER)
                return guac_terminal_send_string(term, GUAC_TERMINAL_ASCII_CR);

            /* Esc */
            if (keysym == GUAC_TERMINAL_KEY_ESCAPE)
                return guac_terminal_send_string(term, GUAC_TERMINAL_ASCII_ESCAPE); 

            /* Home */
            if (keysym == GUAC_TERMINAL_KEY_HOME || keysym == GUAC_TERMINAL_KEY_KP_HOME)
                return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_HOME);

            if (keysym == GUAC_KEYSYM_HOME || keysym == GUAC_KEYSYM_KP_HOME)
                return guac_terminal_send_string(term, term->linux_console_keys ? "\x1B[1~" : "\x1B[H");

            if (keysym == GUAC_KEYSYM_END || keysym == GUAC_KEYSYM_KP_END)
                return guac_terminal_send_string(term, term->linux_console_keys ? "\x1B[4~" : "\x1B[F");

            /* Arrow keys w/ modifiers */
            if (__guac_terminal_any_modifier(term) && __guac_terminal_is_arrow_keysym(keysym))
                return __guac_terminal_send_modified_arrow(term, keysym);

            /* Cursor editing keys w/ modifiers */
            if (__guac_terminal_any_modifier(term) && __guac_terminal_is_editing_keysym(keysym))
                return __guac_terminal_send_modified_editing(term, keysym);

            /* Function keys w/ modifiers */
            if (__guac_terminal_any_modifier(term) && __guac_terminal_is_function_keysym(keysym))
                return __guac_terminal_send_modified_function(term, keysym);

            /* Application cursor mode (DECCKM) affects only arrow keys.
             * Home and End use their own sequences above and are unaffected. */
            if (term->application_cursor_keys) {
                /* Left */
                if (keysym == GUAC_TERMINAL_KEY_LEFT || keysym == GUAC_TERMINAL_KEY_KP_LEFT)
                    return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_APP_CURSOR_LEFT);

                /* Up */
                if (keysym == GUAC_TERMINAL_KEY_UP || keysym == GUAC_TERMINAL_KEY_KP_UP)
                    return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_APP_CURSOR_UP);

                /* Right */
                if (keysym == GUAC_TERMINAL_KEY_RIGHT || keysym == GUAC_TERMINAL_KEY_KP_RIGHT)
                    return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_APP_CURSOR_RIGHT);

                /* Down */
                if (keysym == GUAC_TERMINAL_KEY_DOWN || keysym == GUAC_TERMINAL_KEY_KP_DOWN)
                    return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_APP_CURSOR_DOWN);
            }
            else {

                /* Left */
                if (keysym == GUAC_TERMINAL_KEY_LEFT || keysym == GUAC_TERMINAL_KEY_KP_LEFT)
                    return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_CURSOR_LEFT);

                /* Up */
                if (keysym == GUAC_TERMINAL_KEY_UP || keysym == GUAC_TERMINAL_KEY_KP_UP)
                    return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_CURSOR_UP);

                /* Right */
                if (keysym == GUAC_TERMINAL_KEY_RIGHT || keysym == GUAC_TERMINAL_KEY_KP_RIGHT)
                    return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_CURSOR_RIGHT);

                /* Down */
                if (keysym == GUAC_TERMINAL_KEY_DOWN || keysym == GUAC_TERMINAL_KEY_KP_DOWN)
                    return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_CURSOR_DOWN);
            }

            /* Page up */
            if (keysym == GUAC_TERMINAL_KEY_PAGEUP || keysym == GUAC_TERMINAL_KEY_KP_PAGEUP)
                return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_PAGEUP);

            /* Page down */
            if (keysym == GUAC_TERMINAL_KEY_PAGEDOWN || keysym == GUAC_TERMINAL_KEY_KP_PAGEDOWN)
                return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_PAGEDOWN);

            /* End */
            if (keysym == GUAC_TERMINAL_KEY_END || keysym == GUAC_TERMINAL_KEY_KP_END)
                return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_END);

            /* Insert */
            if (keysym == GUAC_TERMINAL_KEY_INSERT || keysym == GUAC_TERMINAL_KEY_KP_INSERT)
                return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_INSERT);

            /* F1  */
            if (term->func_keys_and_keypad == GUAC_TERMINAL_FUNC_KEYS_AND_KEYPAD_VT100) {
                /* https://vt100.net/docs/vt100-ug/chapter3.html */
                if (keysym == 0xFFBE || keysym == 0xFF91) return guac_terminal_send_string(term, "\x1BOP"); /* F1  */
                if (keysym == 0xFFBF || keysym == 0xFF92) return guac_terminal_send_string(term, "\x1BOQ"); /* F2  */
                if (keysym == 0xFFC0 || keysym == 0xFF93) return guac_terminal_send_string(term, "\x1BOR"); /* F3  */
                if (keysym == 0xFFC1 || keysym == 0xFF94) return guac_terminal_send_string(term, "\x1BOS"); /* F4  */
                /* Send this escape code, although the original VT100 did not have F5 */
                if (keysym == 0xFFC2) return guac_terminal_send_string(term, "\x1B[15~"); /* F5  */
            }
            else {
                if (keysym == GUAC_TERMINAL_KEY_F1 || keysym == GUAC_TERMINAL_KEY_KP_F1)
                    return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_F1);

                /* F2  */
                if (keysym == GUAC_TERMINAL_KEY_F2 || keysym == GUAC_TERMINAL_KEY_KP_F2)
                    return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_F2);

                /* F3  */
                if (keysym == GUAC_TERMINAL_KEY_F3 || keysym == GUAC_TERMINAL_KEY_KP_F3)
                    return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_F3);

                /* F4  */
                if (keysym == GUAC_TERMINAL_KEY_F4 || keysym == GUAC_TERMINAL_KEY_KP_F4)
                    return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_F4);

                /* F5  */
                if (keysym == GUAC_TERMINAL_KEY_F5)
                    return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_F5);

            }
            /* F6  */
            if (keysym == GUAC_TERMINAL_KEY_F6)
                return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_F6);

            /* F7  */ 
            if (keysym == GUAC_TERMINAL_KEY_F7)
                return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_F7);

            /* F8  */
            if (keysym == GUAC_TERMINAL_KEY_F8)
                return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_F8);

            /* F9  */
            if (keysym == GUAC_TERMINAL_KEY_F9)
                return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_F9);

            /* F10 */
            if (keysym == GUAC_TERMINAL_KEY_F10)
                return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_F10);

            /* F11 */
            if (keysym == GUAC_TERMINAL_KEY_F11)
                return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_F11);

            /* F12 */
            if (keysym == GUAC_TERMINAL_KEY_F12)
                return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_F12);

            /* Delete */
            if (keysym == GUAC_TERMINAL_KEY_DELETE || keysym == GUAC_TERMINAL_KEY_KP_DELETE)
                return guac_terminal_send_string(term, GUAC_TERMINAL_ESC_SEQ_DELETE);

            /* Ignore unknown keys */
            guac_client_log(term->client, GUAC_LOG_DEBUG,
                    "Ignoring unknown keysym: 0x%X", keysym);
        }

    }

    return 0;

}

int guac_terminal_send_key(guac_terminal* term, int keysym, int pressed) {

    int result;

    guac_terminal_lock(term);
    result = __guac_terminal_send_key(term, keysym, pressed);
    guac_terminal_unlock(term);

    return result;

}

/**
 * Determines if the given character is part of a word.
 * Match these chars :[0-9A-Za-z\$\-\.\/_~] and accented letters.
 * This allows a path, variable name or IP address to be treated as a word.
 *
 * @param ascii_char
 *     The character to check.
 *
 * @return
 *     true if match a "word" char,
 *     false otherwise.
 */
static bool guac_terminal_is_part_of_word(int ascii_char) {
    return ((ascii_char >= '0' && ascii_char <= '9') ||
            (ascii_char >= 'A' && ascii_char <= 'Z') ||
            (ascii_char >= 'a' && ascii_char <= 'z') ||
            (ascii_char >= GUAC_TERMINAL_LATIN1_CAPITAL_AGRAVE &&
             ascii_char <= GUAC_TERMINAL_LATIN1_Y_UMLAUT) ||
            (ascii_char == '$') ||
            (ascii_char == '-') ||
            (ascii_char == '.') ||
            (ascii_char == '/') ||
            (ascii_char == '_') ||
            (ascii_char == '~'));
}

/**
 * Determines if the given character is part of URI.
 * Match these chars :[%\&\+:;,=\?\!\*\\\(\)\[\]#] and word chars.
 *
 * @param ascii_char
 *     The character to check.
 *
 * @return
 *     true if match a "word" or "uri" char,
 *     false otherwise.
 */
static bool guac_terminal_is_part_of_uri(int ascii_char) {
    return (guac_terminal_is_part_of_word(ascii_char) ||
            (ascii_char == '%') ||
            (ascii_char == '&') ||
            (ascii_char == '+') ||
            (ascii_char == ':') ||
            (ascii_char == ';') ||
            (ascii_char == ',') ||
            (ascii_char == '=') ||
            (ascii_char == '?') ||
            (ascii_char == '!') ||
            (ascii_char == '*') ||
            (ascii_char == '\\') ||
            (ascii_char == '(') ||
            (ascii_char == ')') ||
            (ascii_char == '[') ||
            (ascii_char == ']') ||
            (ascii_char == '#'));
}

/**
 * Determines if the given character is part of blank block.
 *
 * @param ascii_char
 *     The character to check.
 *
 * @return
 *     true if match space (char 0x20) or NULL (char 0x00),
 *     false otherwise.
 */
static bool guac_terminal_is_blank(int ascii_char) {
    return (ascii_char == '\0' || ascii_char == ' ');
}

/**
 * Set the row tail/head and col tail/head according to the value of current
 * row/col and selection_initial_row/selection_initial_column. This aims to
 * keep the initially selected word when dragging mouse after a double click.
 *
 * @param terminal
 *     The terminal that received a double click event.
 *
 * @param col
 *     The column where is currently the mouse.
 *
 * @param row
 *     The row where is currently the mouse.
 *
 * @param col_head
 *      Pointer where to write the calculated head column.
 *
 * @param col_tail
 *      Pointer where to write the calculated tail column.
 *
 * @param row_head
 *      Pointer where to write the calculated head row.
 *
 * @param row_tail
 *      Pointer where to write the calculated tail row.
 */
static void guac_terminal_word_initial_position(guac_terminal* terminal,
        int col, int row, int* col_head, int* col_tail, int* row_head, int* row_tail) {

    /* The mouse is still in the intial row */
    if (row == terminal->selection_initial_row) {

        /* Mouse on the right of initial column */
        if (col > terminal->selection_initial_column)
            *col_head = terminal->selection_initial_column;

        /* Mouse on the left of initial column */
        else
            *col_tail = terminal->selection_initial_column;
    }

    /* Use initial row as bottom-right of the selection and go up/left */
    if (row < terminal->selection_initial_row) {
        *row_tail = terminal->selection_initial_row;
        *col_tail = terminal->selection_initial_column;
    }

    /* Use initial row/col as top-left of the selection and go down/right */
    if (row > terminal->selection_initial_row) {
        *row_head = terminal->selection_initial_row;
        *col_head = terminal->selection_initial_column;
    }
}

/**
 * Get word/URI/blank boundaries on a terminal buffer depending on given position:
 * existing selection to update or mouse position. Gets the boundaries in the
 * left/up direction and then in the right/down direction, continuing if the
 * row is wrapped. A sequence of the same character whatever it is will be treated
 * as a word.
 *
 * @param buffer
 *     The terminal buffer where to search word boundaries.
 *
 * @param detector
 *     Pointer to the desired function for detecting boundaries.
 *
 * @param col_head
 *      Pointer where to write the head column of the word.
 *
 * @param col_tail
 *      Pointer where to write the tail column of the word.
 *
 * @param row_head
 *      Pointer where to write the head row of the word.
 *
 * @param row_tail
 *      Pointer where to write the tail row of the word.
 */
static void guac_terminal_get_word_bounds(guac_terminal_buffer* buffer, bool (*detector)(int),
        int* col_head, int* col_tail, int* row_head, int* row_tail) {

    /* To store buffer row characters */
    guac_terminal_char* characters;
    /* To get wrapped buffer row status */
    bool is_wrapped;
    /* Length of the buffer row */
    int length;
    /* Character read at a position */
    int current_char;
    /* The char behind the given row/col */
    int initial_char;

    /* Detect the correct function regardless of direction */
    bool (*is_part_of)(int) = NULL;

    /* Get word head */
    do {

        /* Buffer row to get */
        int current_row = *row_head;
        int current_col = *col_head;

        /* Bound of screen reached: get previous row */
        if (*col_head == 0)
            current_row--;

        /* Get current buffer row */
        length = guac_terminal_buffer_get_columns(buffer,
                &characters, &is_wrapped, current_row);

        /* If we are on the previous row */
        if (current_row < *row_head) {

            /* Line not wrapped: stop, it's the word boundary */
            if (!is_wrapped)
                break;

            /* Go to last column of this row */
            current_col = length;
        }

        /* Get char of the previous column on current row */
        current_char = characters[current_col-1].value;

        /* Init is_part_of on first iteration according to the char on the 
         * current row and column */
        if (is_part_of == NULL) {
            initial_char = characters[current_col].value;
            is_part_of = guac_terminal_is_blank(initial_char)
                    ? guac_terminal_is_blank
                    : detector;
        }

        /* End of sequence of the same character */
        if (!is_part_of(initial_char) && current_char != initial_char)
            break;

        /* Word boundary reached */
        if (is_part_of(initial_char) && !is_part_of(current_char))
            break;

        /* Store new position on previous row */
        if (current_row < *row_head) {
            *row_head = current_row;
            *col_head = current_col;
        }

    } while (*col_head >= 0 && (*col_head)--);

    /* Reset to detect the correct function while moving forward */
    is_part_of = NULL;

    /* Get word tail */
    do {

        /* Get current buffer row */
        length = guac_terminal_buffer_get_columns(buffer,
                &characters, &is_wrapped, *row_tail);

        /* Bound of screen reached and row is wrapped: get next row */
        if (*col_tail == length - 1 && is_wrapped) {

            /* Get next buffer row */
            bool next_is_wrapped;
            guac_terminal_char* next_characters;
            guac_terminal_buffer_get_columns(buffer,
                    &next_characters, &next_is_wrapped, *row_tail + 1);

            /* Get first char of the next row */
            current_char = next_characters[0].value;

        }

        /* Otherwise, get char of next column on current row */
        else
            current_char = characters[*col_tail+1].value;

        /* Init is_part_of on first iteration according to the char on the 
         * current row and column */
        if (is_part_of == NULL) {
            initial_char = characters[*col_tail].value;
            is_part_of = guac_terminal_is_blank(initial_char)
                    ? guac_terminal_is_blank
                    : detector;
        }

        /* End of sequence of the same character */
        if (!is_part_of(initial_char) && current_char != initial_char)
            break;

        /* Word boundary reached */
        if (is_part_of(initial_char) && !is_part_of(current_char))
            break;

        /* Store new position on next row */
        if (*col_tail == length - 1 && is_wrapped) {
            (*row_tail)++;
            *col_tail = 0;
        }

        /* Or go to next column of current row */
        else
            (*col_tail)++;

    } while (*col_tail <= length);

}

/**
 * Selection of a word during a double click event.
 *  - Fetching the character under the mouse cursor.
 *  - Determining the type of character :
 *      Letter, digit, acceptable symbol within a word,
 *      or space/NULL,
 *      all other chars are treated as single.
 *  - Calculating the word boundaries.
 *  - Visual selection of the found word.
 *  - Adding it to clipboard.
 *
 * @param terminal
 *     The terminal that received a double click event.
 *
 * @param row
 *     The row where is the mouse at the double click event.
 *
 * @param col
 *     The column where is the mouse at the double click event.
 *
 * @param hold
 *      True when user hold left click, false otherwise.
 */
static void guac_terminal_double_click(guac_terminal* terminal, int row, int col, bool hold) {

    /* To store buffer row characters */
    guac_terminal_char* characters;
    /* To get wrapped buffer row status */
    bool is_wrapped;
    /* Length of the buffer row */
    int length;
    /* Character read at a position */
    int current_char;

    /* Position of the detected word. Default = col/row required to select
     * a char if not a word and not blank */
    int word_col_head = col;
    int word_col_tail = col;
    int word_row_head = row;
    int word_row_tail = row;

    /* Position of the detected URI */
    int uri_col_head;
    int uri_col_tail;
    int uri_row_head;
    int uri_row_tail;

    /* User holds left click: update default selection boundaries */
    if (hold)
        guac_terminal_word_initial_position(terminal, col, row, &word_col_head,
                &word_col_tail, &word_row_head, &word_row_tail);

    /* Try to get boundaries of a word */
    guac_terminal_get_word_bounds(terminal->current_buffer, guac_terminal_is_part_of_word,
            &word_col_head, &word_col_tail, &word_row_head, &word_row_tail);

    /* Search for URI only when user don't hold left click
     * to unconditionally extend selection to word pattern */
    if (!hold) {

        /* Begin uri search on previously found word position,
        * this avoids going through the same characters twice */
        uri_col_head = word_col_head;
        uri_col_tail = word_col_tail;
        uri_row_head = word_row_head;
        uri_row_tail = word_row_tail;

        /* Get boundaries of potential URI */
        guac_terminal_get_word_bounds(terminal->current_buffer, guac_terminal_is_part_of_uri,
            &uri_col_head, &uri_col_tail, &uri_row_head, &uri_row_tail);

        /* Check if uri dected */
        if ((uri_col_head != word_col_head || uri_col_tail != word_col_tail ||
             uri_row_head != word_row_head || uri_row_tail != word_row_tail)) {

            /* Temp vars to avoid overwrite uri_row_head and uri_col_head values */
            int tmp_row = uri_row_head;
            int tmp_col = uri_col_head;

            /* Check for the presence of a uri scheme like /^[a-z]*\:\/{2}/ */
            do {

                /* Get first char of first row */
                length = guac_terminal_buffer_get_columns(terminal->current_buffer,
                        &characters, &is_wrapped, tmp_row);
                current_char = characters[tmp_col].value;

                /* [a-z]+ part */
                if (current_char >= 'a' && current_char <= 'z') {

                    /* Go to next col on current row */
                    if (tmp_col != length-1) {
                        tmp_col++;
                        continue;
                    }

                    /* Go to first col of next row */
                    tmp_col = 0;
                    tmp_row++;
                    continue;

                }

                /* Search for URI scheme delimiter `://` */
                if (current_char == ':' &&
                    tmp_col + 2 < length &&
                    characters[tmp_col + 1].value == '/' &&
                    characters[tmp_col + 2].value == '/') {

                    /* Use URI limits instead of word limits */
                    word_col_head = uri_col_head;
                    word_col_tail = uri_col_tail;
                    word_row_head = uri_row_head;
                    word_row_tail = uri_row_tail;
                }

                /* Always exit after non-letter char */
                break;

            } while (tmp_row < uri_row_tail || tmp_col < uri_col_tail);
        }
    }

    /* Select and add to clipboard the "word" */
    guac_terminal_select_start(terminal, word_row_head, word_col_head, GUAC_TERMINAL_COLUMN_SIDE_LEFT);
    guac_terminal_select_update(terminal, word_row_tail, word_col_tail, GUAC_TERMINAL_COLUMN_SIDE_RIGHT);

}

/**
 * Selection of a line during a triple click event.
 *  - Get buffer row boundaries if it has been wrapped.
 *  - Visual selection of the line.
 *  - Adding it to clipboard.
 *
 * @param terminal
 *     The terminal that received a triple click event.
 *
 * @param row
 *     The row where is the mouse at the triple click event.
 *
 * @param hold
 *      True when user hold left click.
 */
static void guac_terminal_triple_click(guac_terminal* terminal, int row, bool hold) {

    /* Temporarily reading previous and next lines */
    guac_terminal_char* characters;
    bool is_wrapped;
    int length;

    /* Final boundary rows */
    int top_row = row;
    int bottom_row = row;

    /* User holds left click */
    if (hold) {

        /* Use initial row as bottom of the selection and go up */
        if (row <= terminal->selection_initial_row)
            bottom_row = terminal->selection_initial_row;

        /* Use initial row as top of the selection and go down */
        if (row > terminal->selection_initial_row)
            top_row = terminal->selection_initial_row;
    }

    /* Get top boundary */
    do {

        /* Read previous buffer row */
        length = guac_terminal_buffer_get_columns(terminal->current_buffer,
                &characters, &is_wrapped, top_row - 1);

    /* Go to the previous row if it is wrapped */
    } while (is_wrapped && top_row--);

    /* Get bottom boundary */
    do {

        /* Read current buffer row */
        length = guac_terminal_buffer_get_columns(terminal->current_buffer,
                &characters, &is_wrapped, bottom_row);

    /* Go to the next row if current row is wrapped */
    } while (is_wrapped && bottom_row++);

    /* Start selection on first col of top_row */
    guac_terminal_select_start(terminal, top_row, 0, GUAC_TERMINAL_COLUMN_SIDE_LEFT);

    /* End selection on last col of bottom_row */
    guac_terminal_select_update(terminal, bottom_row, length - 1, GUAC_TERMINAL_COLUMN_SIDE_RIGHT);
}

static int __guac_terminal_send_mouse(guac_terminal* term, guac_user* user,
        int x, int y, int mask) {

    /* Ignore user input if terminal is not started */
    if (!term->started) {
        guac_client_log(term->client, GUAC_LOG_DEBUG, "Ignoring user input "
                "while terminal has not yet started.");
        return 0;
    }

    /* Determine which buttons were just released and pressed */
    int released_mask =  term->mouse_mask & ~mask;
    int pressed_mask  = ~term->mouse_mask &  mask;

    /* Store current mouse location/state */
    guac_common_cursor_update(term->cursor, user, x, y, mask);

    /* Notify scrollbar, do not handle anything handled by scrollbar */
    if (guac_terminal_scrollbar_handle_mouse(term->scrollbar, x, y, mask)) {

        /* Set pointer cursor if mouse is over scrollbar */
        if (term->current_cursor != GUAC_TERMINAL_CURSOR_POINTER) {
            term->current_cursor = GUAC_TERMINAL_CURSOR_POINTER;
            guac_common_cursor_set_pointer(term->cursor);
            guac_terminal_notify(term);
        }

        guac_terminal_notify(term);
        return 0;

    }

    /* Remove display margin from mouse position without going below 0 */
    y = y >= term->display->margin ? y - term->display->margin : 0;
    x = x >= term->display->margin ? x - term->display->margin : 0;

    term->mouse_mask = mask;

    /* Show mouse cursor if not already shown */
    if (term->current_cursor != GUAC_TERMINAL_CURSOR_IBAR) {
        term->current_cursor = GUAC_TERMINAL_CURSOR_IBAR;
        guac_common_cursor_set_ibar(term->cursor);
        guac_terminal_notify(term);
    }

    /* Paste contents of clipboard on right or middle mouse button up */
    if ((released_mask & GUAC_CLIENT_MOUSE_RIGHT) || (released_mask & GUAC_CLIENT_MOUSE_MIDDLE))
        return guac_terminal_send_data(term, term->clipboard->buffer, term->clipboard->length);

    /* If left mouse button was just released, stop selection */
    if (released_mask & GUAC_CLIENT_MOUSE_LEFT)
        guac_terminal_select_end(term);

    /* Update selection state contextually while the left mouse button is
     * pressed */
    else if (mask & GUAC_CLIENT_MOUSE_LEFT) {

        int row = y / term->display->char_height - term->scroll_offset;
        int col = x / term->display->char_width;

        /* Determine if x is on the left or right half of the column */
        int char_x_offset = x % term->display->char_width;
        guac_terminal_column_side side = char_x_offset < (term->display->char_width / 2) ?
            GUAC_TERMINAL_COLUMN_SIDE_LEFT : GUAC_TERMINAL_COLUMN_SIDE_RIGHT;

        /* If mouse button was already just pressed, start a new selection or
         * resume the existing selection depending on whether shift is held */
        if (pressed_mask & GUAC_CLIENT_MOUSE_LEFT) {
            if (term->mod_shift)
                guac_terminal_select_resume(term, row, col, side);
            else {

                /* Reset click counter if last click was 300ms before */
                if (guac_timestamp_current() - term->click_timer > 300)
                    term->click_counter = 0;

                /* New click time */
                term->click_timer = guac_timestamp_current();

                switch (term->click_counter++) {

                    /* First click = start selection */
                    case 0:
                        /* The rectangular selection is requested by pressing
                         * the ALT key at the start of the selection */
                        term->rectangle_selection = term->mod_alt;

                        /* Save initial mouse position */
                        term->selection_initial_row = row;
                        term->selection_initial_column = col;

                        /* Start selection */
                        guac_terminal_select_start(term, row, col, side);
                        break;

                    /* Second click = word selection */
                    case 1:
                        guac_terminal_double_click(term, row, col, false);
                        break;

                    /* third click or more = line selection */
                    default:
                        guac_terminal_triple_click(term, row, false);
                        break;
                }
            }
        }

        /* Hold left click */
        else

            /* Simply update the existing selection as long as
             * the mouse button is pressed */
            if (term->click_counter <= 1)
                guac_terminal_select_update(term, row, col, side);

            /* Second click + hold = word selection update */
            else if (term->click_counter == 2)
                guac_terminal_double_click(term, row, col, true);

            /* third click or more + hold = line selection update */
            else
                guac_terminal_triple_click(term, row, true);

    }

    /* Scroll up if wheel moved up */
    if (released_mask & GUAC_CLIENT_MOUSE_SCROLL_UP)
        guac_terminal_scroll_display_up(term, GUAC_TERMINAL_WHEEL_SCROLL_AMOUNT);

    /* Scroll down if wheel moved down */
    if (released_mask & GUAC_CLIENT_MOUSE_SCROLL_DOWN)
        guac_terminal_scroll_display_down(term, GUAC_TERMINAL_WHEEL_SCROLL_AMOUNT);

    return 0;

}

int guac_terminal_send_mouse(guac_terminal* term, guac_user* user,
        int x, int y, int mask) {

    int result;

    guac_terminal_lock(term);
    result = __guac_terminal_send_mouse(term, user, x, y, mask);
    guac_terminal_unlock(term);

    return result;

}

void guac_terminal_scroll_handler(guac_terminal_scrollbar* scrollbar, int value) {

    guac_terminal* terminal = (guac_terminal*) scrollbar->data;

    /* Calculate change in scroll offset */
    int delta = -value - terminal->scroll_offset;

    /* Update terminal based on change in scroll offset */
    if (delta < 0)
        guac_terminal_scroll_display_down(terminal, -delta);
    else if (delta > 0)
        guac_terminal_scroll_display_up(terminal, delta);

    /* Update scrollbar value */
    guac_terminal_scrollbar_set_value(scrollbar, value);

}

int guac_terminal_sendf(guac_terminal* term, const char* format, ...) {

    int written;

    va_list ap;
    char buffer[1024];

    /* Block all other sources of input if input is coming from a stream */
    if (term->input_stream != NULL)
        return 0;

    /* Print to buffer */
    va_start(ap, format);
    written = vsnprintf(buffer, sizeof(buffer)-1, format, ap);
    va_end(ap);

    if (written < 0)
        return written;

    /* Write to STDIN */
    return guac_terminal_write_all(term->stdin_pipe_fd[1], buffer, written);

}

void guac_terminal_set_tab(guac_terminal* term, int column) {

    int i;

    /* Search for available space, set if available */
    for (i=0; i<GUAC_TERMINAL_MAX_TABS; i++) {

        /* Set tab if space free */
        if (term->custom_tabs[i] == 0) {
            term->custom_tabs[i] = column+1;
            break;
        }

    }

}

void guac_terminal_unset_tab(guac_terminal* term, int column) {

    int i;

    /* Search for given tab, unset if found */
    for (i=0; i<GUAC_TERMINAL_MAX_TABS; i++) {

        /* Unset tab if found */
        if (term->custom_tabs[i] == column+1) {
            term->custom_tabs[i] = 0;
            break;
        }

    }

}

void guac_terminal_clear_tabs(guac_terminal* term) {
    term->tab_interval = 0;
    memset(term->custom_tabs, 0, sizeof(term->custom_tabs));
}

int guac_terminal_next_tab(guac_terminal* term, int column) {

    int i;

    /* Determine tab stop from interval */
    int tabstop;
    if (term->tab_interval != 0)
        tabstop = (column / term->tab_interval + 1) * term->tab_interval;
    else
        tabstop = term->term_width - 1;

    /* Walk custom tabs, trying to find an earlier occurrence */
    for (i=0; i<GUAC_TERMINAL_MAX_TABS; i++) {

        int custom_tabstop = term->custom_tabs[i] - 1;
        if (custom_tabstop != -1 && custom_tabstop > column && custom_tabstop < tabstop)
            tabstop = custom_tabstop;

    }

    return tabstop;
}

void guac_terminal_pipe_stream_open(guac_terminal* term, const char* name,
        int flags) {

    guac_client* client = term->client;
    guac_socket* socket = client->socket;

    /* Close existing stream, if any */
    guac_terminal_pipe_stream_close(term);

    /* Allocate and assign new pipe stream */
    term->pipe_stream = guac_client_alloc_stream(client);
    term->pipe_buffer_length = 0;
    term->pipe_stream_flags = flags;

    /* Open new pipe stream */
    guac_protocol_send_pipe(socket, term->pipe_stream, "text/plain", name);

    /* Log redirect at debug level */
    guac_client_log(client, GUAC_LOG_DEBUG, "Terminal output now directed to "
            "pipe \"%s\" (flags=%i).", name, flags);

}

void guac_terminal_pipe_stream_write(guac_terminal* term, char c) {

    /* Append byte to buffer only if pipe is open */
    if (term->pipe_stream != NULL) {

        /* Flush buffer if no space is available */
        if (term->pipe_buffer_length == sizeof(term->pipe_buffer))
            guac_terminal_pipe_stream_flush(term);

        /* Append single byte to buffer */
        term->pipe_buffer[term->pipe_buffer_length++] = c;

    }

}

void guac_terminal_pipe_stream_flush(guac_terminal* term) {

    guac_client* client = term->client;
    guac_socket* socket = client->socket;
    guac_stream* pipe_stream = term->pipe_stream;

    /* Write blob if data exists in buffer */
    if (pipe_stream != NULL && term->pipe_buffer_length > 0) {
        guac_protocol_send_blob(socket, pipe_stream,
                term->pipe_buffer, term->pipe_buffer_length);
        term->pipe_buffer_length = 0;
    }

}

void guac_terminal_pipe_stream_close(guac_terminal* term) {

    guac_client* client = term->client;
    guac_socket* socket = client->socket;
    guac_stream* pipe_stream = term->pipe_stream;

    /* Close any existing pipe */
    if (pipe_stream != NULL) {

        /* Write end of stream */
        guac_terminal_pipe_stream_flush(term);
        guac_protocol_send_end(socket, pipe_stream);

        /* Destroy stream */
        guac_client_free_stream(client, pipe_stream);
        term->pipe_stream = NULL;

        /* Log redirect at debug level */
        guac_client_log(client, GUAC_LOG_DEBUG,
                "Terminal output now redirected to display.");

    }

}

int guac_terminal_create_typescript(guac_terminal* term, const char* path,
        const char* name, int create_path, int allow_write_existing) {

    /* Create typescript */
    term->typescript = guac_terminal_typescript_alloc(
            path, name, create_path, allow_write_existing);

    /* Log failure */
    if (term->typescript == NULL) {
        guac_client_log(term->client, GUAC_LOG_ERROR, "Creation of typescript "
                "failed: %s: %s", guac_error_message,
                guac_status_string(guac_error));
        return 1;
    }

    /* If typescript was successfully created, log filenames */
    guac_client_log(term->client, GUAC_LOG_INFO, "Typescript of terminal "
            "session will be saved within \"%s\" to \"%s\". Corresponding "
            "timing file is \"%s\".", path, term->typescript->data_filename,
            term->typescript->timing_filename);

    /* Typescript creation succeeded */
    return 0;

}

/**
 * Synchronize the state of the provided terminal to a subset of users of
 * the provided guac_client using the provided socket.
 *
 * @param client
 *     The client whose users should be synchronized.
 *
 * @param term
 *     The terminal state that should be synchronized to the users.
 *
 * @param socket
 *     The socket that should be used to communicate with the users.
 */
static void __guac_terminal_sync_socket(
        guac_client* client, guac_terminal* term, guac_socket* socket) {

    /* Synchronize display state with new user */
    guac_terminal_repaint_default_layer(term, socket);
    guac_terminal_display_dup(term->display, client, socket);

    /* Synchronize mouse cursor */
    guac_common_cursor_dup(term->cursor, client, socket);

    /* Paint scrollbar for joining users */
    guac_terminal_scrollbar_dup(term->scrollbar, client, socket);

}

void guac_terminal_dup(guac_terminal* term, guac_user* user,
        guac_socket* socket) {

    /* Ignore the user and just use the provided socket directly */
    __guac_terminal_sync_socket(user->client, term, socket);

}

void guac_terminal_sync_users(
        guac_terminal* term, guac_client* client, guac_socket* socket) {

    /* Use the provided socket to synchronize state to the users */
    __guac_terminal_sync_socket(client, term, socket);

}

void guac_terminal_apply_color_scheme(guac_terminal* terminal,
        const char* color_scheme) {

    guac_client* client = terminal->client;
    guac_terminal_char* default_char = &terminal->default_char;
    guac_terminal_display* display = terminal->display;

    /* Reinitialize default terminal colors with values from color scheme */
    guac_terminal_parse_color_scheme(client, color_scheme,
        &default_char->attributes.foreground,
        &default_char->attributes.background,
        display->default_palette);

    /* Reinitialize default attributes of buffer and display */
    guac_terminal_display_reset_palette(display);
    display->default_foreground = default_char->attributes.foreground;
    display->default_background = default_char->attributes.background;

    /* Redraw terminal text and background */
    guac_terminal_redraw_default_layer(terminal);

    /* Acquire exclusive access to terminal */
    guac_terminal_lock(terminal);

    /* Update stored copy of color scheme */
    guac_mem_free_const(terminal->color_scheme);
    terminal->color_scheme = guac_strdup(color_scheme);

    /* Release terminal */
    guac_terminal_unlock(terminal);

    guac_terminal_notify(terminal);

}

const char* guac_terminal_get_color_scheme(guac_terminal* terminal) {
    return terminal->color_scheme;
}

void guac_terminal_apply_font(guac_terminal* terminal, const char* font_name,
        int font_size, int dpi) {

    guac_terminal_display* display = terminal->display;

    if (guac_terminal_display_set_font(display, font_name, font_size, dpi))
        return;

    /* Resize terminal to fit available region, now that font metrics may be
     * different */
    guac_terminal_resize(terminal, terminal->outer_width,
            terminal->outer_height);

    /* Redraw terminal text and background */
    guac_terminal_redraw_default_layer(terminal);

    /* Acquire exclusive access to terminal */
    guac_terminal_lock(terminal);

    /* Update stored copy of font name, if changed */
    if (font_name != NULL) {
        guac_mem_free_const(terminal->font_name);
        terminal->font_name = guac_strdup(font_name);
    }

    /* Update stored copy of font size, if changed */
    if (font_size != -1)
        terminal->font_size = font_size;

    /* Release terminal */
    guac_terminal_unlock(terminal);

    guac_terminal_notify(terminal);

}

void guac_terminal_set_upload_path_handler(guac_terminal* terminal,
        guac_terminal_upload_path_handler* upload_path_handler) {
    terminal->upload_path_handler = upload_path_handler;
}

void guac_terminal_set_file_download_handler(guac_terminal* terminal,
        guac_terminal_file_download_handler* file_download_handler) {
    terminal->file_download_handler = file_download_handler;
}

const char* guac_terminal_get_font_name(guac_terminal* terminal) {
    return terminal->font_name;
}

int guac_terminal_get_font_size(guac_terminal* terminal) {
    return terminal->font_size;
}

int guac_terminal_get_mod_ctrl(guac_terminal* terminal) {
    return terminal->mod_ctrl;
}

void guac_terminal_clipboard_reset(guac_terminal* terminal,
        const char* mimetype) {
    guac_common_clipboard_reset(terminal->clipboard, mimetype);
}

void guac_terminal_clipboard_append(guac_terminal* terminal,
        const char* data, int length) {

    /* Allocate and clear space for the converted data */
    int output_buf_size = terminal->clipboard->available;
    char* output_data = guac_mem_alloc(output_buf_size);
    char* output = output_data;

    /* Convert clipboard contents */
    guac_iconv(GUAC_READ_UTF8_NORMALIZED, &data, length,
            GUAC_WRITE_UTF8, &output, output_buf_size);

    guac_common_clipboard_append(terminal->clipboard, output_data, output - output_data);
    
    guac_mem_free(output_data);
}

void guac_terminal_remove_user(guac_terminal* terminal, guac_user* user) {

    /* Remove the user from the terminal cursor */
    guac_common_cursor_remove_user(terminal->cursor, user);
}

void guac_terminal_redraw_default_layer(guac_terminal* terminal) {

    /* Redraw terminal text and background */
    guac_terminal_repaint_default_layer(terminal, terminal->client->socket);
    __guac_terminal_redraw_rect(terminal, 0, 0,
            terminal->term_height - 1,
            terminal->term_width - 1);
}
