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


#ifndef _GUAC_TERMINAL_H
#define _GUAC_TERMINAL_H

#include "config.h"

#include "buffer.h"
#include "cursor.h"
#include "display.h"
#include "guac_clipboard.h"
#include "scrollbar.h"
#include "types.h"

#include <pthread.h>
#include <stdbool.h>

#include <guacamole/client.h>
#include <guacamole/stream.h>

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
 * The maximum number of bytes to allow within the clipboard.
 */
#define GUAC_TERMINAL_CLIPBOARD_MAX_LENGTH 262144

/**
 * The name of the color scheme having black foreground and white background.
 */
#define GUAC_TERMINAL_SCHEME_BLACK_WHITE "black-white"

/**
 * The name of the color scheme having gray foreground and black background.
 */
#define GUAC_TERMINAL_SCHEME_GRAY_BLACK "gray-black"

/**
 * The name of the color scheme having green foreground and black background.
 */
#define GUAC_TERMINAL_SCHEME_GREEN_BLACK "green-black"

/**
 * The name of the color scheme having white foreground and black background.
 */
#define GUAC_TERMINAL_SCHEME_WHITE_BLACK "white-black"

typedef struct guac_terminal guac_terminal;

/**
 * Handler for characters printed to the terminal. When a character is printed,
 * the current char handler for the terminal is called and given that
 * character.
 */
typedef int guac_terminal_char_handler(guac_terminal* term, unsigned char c);

/**
 * Handler for setting the destination path for file uploads.
 */
typedef void guac_terminal_upload_path_handler(guac_client* client, char* path);

/**
 * Handler for creating an outbound file download stream for a specified file.
 */
typedef guac_stream* guac_terminal_file_download_handler(guac_client* client, char* filename);

/**
 * Represents a terminal emulator which uses a given Guacamole client to
 * render itself.
 */
struct guac_terminal {

    /**
     * The Guacamole client this terminal emulator will use for rendering.
     */
    guac_client* client;

    /**
     * Called whenever the necessary terminal codes are sent to change
     * the path for future file uploads.
     */
    guac_terminal_upload_path_handler* upload_path_handler;

    /**
     * Called whenever the necessary terminal codes are sent to initiate
     * a download of a given remote file.
     */
    guac_terminal_file_download_handler* file_download_handler;

    /**
     * Lock which restricts simultaneous access to this terminal via the root
     * guac_terminal_* functions.
     */
    pthread_mutex_t lock;

    /**
     * Pipe which should be written to (and read from) to provide output to
     * this terminal. Another thread should read from this pipe when writing
     * data to the terminal. It would make sense for the terminal to provide
     * this thread, but for simplicity, that logic is left to the guac
     * message handler (to give the message handler something to block with).
     */
    int stdout_pipe_fd[2];

    /**
     * Pipe which will be the source of user input. When a terminal code
     * generates synthesized user input, that data will be written to
     * this pipe.
     */
    int stdin_pipe_fd[2];

    /**
     * Graphical representation of the current scroll state.
     */
    guac_terminal_scrollbar* scrollbar;

    /**
     * The relative offset of the display. A positive value indicates that
     * many rows have been scrolled into view, zero indicates that no
     * scrolling has occurred. Negative values are illegal.
     */
    int scroll_offset;

    /**
     * The width of the terminal, in characters.
     */
    int term_width;

    /**
     * The height of the terminal, in characters.
     */
    int term_height;

    /**
     * The index of the first row in the scrolling region.
     */
    int scroll_start;

    /**
     * The index of the last row in the scrolling region.
     */
    int scroll_end;

    /**
     * The current row location of the cursor.
     */
    int cursor_row;

    /**
     * The current column location of the cursor.
     */
    int cursor_col;

    /**
     * The row of the rendered cursor.
     */
    int visible_cursor_row;

    /**
     * The column of the rendered cursor.
     */
    int visible_cursor_col;

    /**
     * The row of the saved cursor (ESC 7).
     */
    int saved_cursor_row;

    /**
     * The column of the saved cursor (ESC 7).
     */
    int saved_cursor_col;

    /**
     * The attributes which will be applied to future characters.
     */
    guac_terminal_attributes current_attributes;

    /**
     * The character whose attributes dictate the default attributes
     * of all characters. When new screen space is allocated, this
     * character fills the gaps.
     */
    guac_terminal_char default_char;

    /**
     * Handler which will receive all printed characters, updating the terminal
     * accordingly.
     */
    guac_terminal_char_handler* char_handler;

    /**
     * The difference between the currently-rendered screen and the current
     * state of the terminal.
     */
    guac_terminal_display* display;

    /**
     * Current terminal display state. All characters present on the screen
     * are within this buffer. This has nothing to do with the display, which
     * facilitates transfer of a set of changes to the remote display.
     */
    guac_terminal_buffer* buffer;

    /**
     * Automatically place a tabstop every N characters. If zero, then no
     * tabstops exist automatically.
     */
    int tab_interval;

    /**
     * Array of all tabs set. Each entry is the column number of a tab + 1,
     * or 0 if that tab cell is unset.
     */
    int custom_tabs[GUAC_TERMINAL_MAX_TABS];

    /**
     * Array of arrays of mapped characters, where the character N is located at the N-32
     * position within the array. Each element in a contained array is the corresponding Unicode
     * codepoint. If NULL, a direct mapping from Unicode is used. The entries of the main array
     * correspond to the character set in use (G0, G1, etc.)
     */
    const int* char_mapping[2];

    /**
     * The active character set. For example, 0 for G0, 1 for G1, etc.
     */
    int active_char_set;

    /**
     * Whether text is being selected.
     */
    bool text_selected;

    /**
     * The row that the selection starts at.
     */
    int selection_start_row;

    /**
     * The column that the selection starts at.
     */
    int selection_start_column;

    /**
     * The width of the character at selection start.
     */
    int selection_start_width;

    /**
     * The row that the selection ends at.
     */
    int selection_end_row;

    /**
     * The column that the selection ends at.
     */
    int selection_end_column;

    /**
     * The width of the character at selection end.
     */
    int selection_end_width;

    /**
     * Whether the cursor (arrow) keys should send cursor sequences
     * or application sequences (DECCKM).
     */
    bool application_cursor_keys;

    /**
     * Whether a CR should automatically follow a LF, VT, or FF.
     */
    bool automatic_carriage_return;

    /**
     * Whether insert mode is enabled (DECIM).
     */
    bool insert_mode;

    /**
     * Whether the alt key is currently being held down.
     */
    int mod_alt;

    /**
     * Whether the control key is currently being held down.
     */
    int mod_ctrl;

    /**
     * Whether the shift key is currently being held down.
     */
    int mod_shift;

    /**
     * The current mouse button state.
     */
    int mouse_mask;

    /**
     * The cached pointer cursor.
     */
    guac_terminal_cursor* pointer_cursor;

    /**
     * The cached I-bar cursor.
     */
    guac_terminal_cursor* ibar_cursor;

    /**
     * The cached invisible (blank) cursor.
     */
    guac_terminal_cursor* blank_cursor;

    /**
     * The current cursor, used to avoid re-setting the cursor.
     */
    guac_terminal_cursor* current_cursor;

    /**
     * The current contents of the clipboard.
     */
    guac_common_clipboard* clipboard;

};

/**
 * Creates a new guac_terminal, having the given width and height, and
 * rendering to the given client.
 *
 * @param client
 *     The client to which the terminal will be rendered.
 *
 * @param font_name
 *     The name of the font to use when rendering glyphs.
 *
 * @param font_size
 *     The size of each glyph, in points.
 *
 * @param dpi
 *     The DPI of the display. The given font size will be adjusted to produce
 *     glyphs at the given DPI.
 *
 * @param width
 *     The width of the terminal, in pixels.
 *
 * @param height
 *     The height of the terminal, in pixels.
 *
 * @param color_scheme
 *     The name of the color scheme to use. This string must be one of the
 *     names defined by the GUAC_TERMINAL_SCHEME_* constants. If blank or NULL,
 *     the default scheme of GUAC_TERMINAL_SCHEME_GRAY_BLACK will be used. If
 *     invalid, a warning will be logged, and the terminal will fall back on
 *     GUAC_TERMINAL_SCHEME_GRAY_BLACK.
 *
 * @return
 *     A new guac_terminal having the given font, dimensions, and attributes
 *     which renders all text to the given client.
 */
guac_terminal* guac_terminal_create(guac_client* client,
        const char* font_name, int font_size, int dpi,
        int width, int height, const char* color_scheme);

/**
 * Frees all resources associated with the given terminal.
 */
void guac_terminal_free(guac_terminal* term);

/**
 * Renders a single frame of terminal data. If data is not yet available,
 * this function will block until data is written.
 */
int guac_terminal_render_frame(guac_terminal* terminal);

/**
 * Reads from this terminal's STDIN. Input comes from key and mouse events
 * supplied by calls to guac_terminal_send_key() and
 * guac_terminal_send_mouse(). If input is not yet available, this function
 * will block.
 */
int guac_terminal_read_stdin(guac_terminal* terminal, char* c, int size);

/**
 * Writes to this terminal's STDOUT. This function may block until space
 * is freed in the output buffer by guac_terminal_render_frame().
 */
int guac_terminal_write_stdout(guac_terminal* terminal, const char* c, int size);

/**
 * Notifies the terminal that an event has occurred and the terminal should
 * flush itself when reasonable.
 *
 * @param terminal
 *     The terminal to notify.
 *
 * @return
 *     Zero if notification succeeded, non-zero if an error occurred while
 *     notifying the terminal.
 */
int guac_terminal_notify(guac_terminal* terminal);

/**
 * Reads a single line from this terminal's STDIN. Input is retrieved in
 * the same manner as guac_terminal_read_stdin() and the same restrictions
 * apply.
 */
void guac_terminal_prompt(guac_terminal* terminal, const char* title, char* str, int size, bool echo);

/**
 * Writes the given format string and arguments to this terminal's STDOUT in
 * the same manner as printf(). This function may block until space is
 * freed in the output buffer by guac_terminal_render_frame().
 */
int guac_terminal_printf(guac_terminal* terminal, const char* format, ...);

/**
 * Handles the given key event, sending data, scrolling, pasting clipboard
 * data, etc. as necessary.
 */
int guac_terminal_send_key(guac_terminal* term, int keysym, int pressed);

/**
 * Handles the given mouse event, sending data, scrolling, pasting clipboard
 * data, etc. as necessary.
 */
int guac_terminal_send_mouse(guac_terminal* term, int x, int y, int mask);

/**
 * Handles a scroll event received from the scrollbar associated with a
 * terminal.
 *
 * @param scrollbar
 *     The scrollbar that has been scrolled.
 *
 * @param value
 *     The new value that should be stored within the scrollbar, and
 *     represented within the terminal display.
 */
void guac_terminal_scroll_handler(guac_terminal_scrollbar* scrollbar, int value);

/**
 * Clears the current clipboard contents and sets the mimetype for future
 * contents.
 */
void guac_terminal_clipboard_reset(guac_terminal* term, const char* mimetype);

/**
 * Appends the given data to the current clipboard.
 */
void guac_terminal_clipboard_append(guac_terminal* term, const void* data, int length);


/* INTERNAL FUNCTIONS */


/**
 * Acquires exclusive access to the terminal. Note that enforcing this
 * exclusive access requires that ALL users of the terminal call this
 * function before making further calls to the terminal.
 */
void guac_terminal_lock(guac_terminal* terminal);

/**
 * Releases exclusive access to the terminal.
 */
void guac_terminal_unlock(guac_terminal* terminal);

/**
 * Resets the state of the given terminal, as if it were just allocated.
 */
void guac_terminal_reset(guac_terminal* term);

/**
 * Writes the given string of characters to the terminal.
 */
int guac_terminal_write(guac_terminal* term, const char* c, int size);

/**
 * Sets the character at the given row and column to the specified value.
 */
int guac_terminal_set(guac_terminal* term, int row, int col, int codepoint);

/**
 * Clears the given region within a single row.
 */
int guac_terminal_clear_columns(guac_terminal* term,
        int row, int start_col, int end_col);

/**
 * Clears the given region from right-to-left, top-to-bottom, replacing
 * all characters with the current background color and attributes.
 */
int guac_terminal_clear_range(guac_terminal* term,
        int start_row, int start_col,
        int end_row, int end_col);

/**
 * Scrolls the terminal's current scroll region up by one row.
 */
int guac_terminal_scroll_up(guac_terminal* term,
        int start_row, int end_row, int amount);

/**
 * Scrolls the terminal's current scroll region down by one row.
 */
int guac_terminal_scroll_down(guac_terminal* term,
        int start_row, int end_row, int amount);

/**
 * Commits the current cursor location, updating the visible cursor
 * on the screen.
 */
void guac_terminal_commit_cursor(guac_terminal* term);

/**
 * Scroll down the display by the given amount, replacing the new space with
 * data from the buffer. If not enough data is available, the maximum
 * amount will be scrolled.
 */
void guac_terminal_scroll_display_down(guac_terminal* terminal, int amount);

/**
 * Scroll up the display by the given amount, replacing the new space with data
 * from either the buffer or the terminal buffer.  If not enough data is
 * available, the maximum amount will be scrolled.
 */
void guac_terminal_scroll_display_up(guac_terminal* terminal, int amount);

/**
 * Marks the start of text selection at the given row and column.
 */
void guac_terminal_select_start(guac_terminal* terminal, int row, int column);

/**
 * Updates the end of text selection at the given row and column.
 */
void guac_terminal_select_update(guac_terminal* terminal, int row, int column);

/**
 * Ends text selection, removing any highlight. Character data is stored in the
 * string buffer provided.
 */
void guac_terminal_select_end(guac_terminal* terminal, char* string);


/* LOW-LEVEL TERMINAL OPERATIONS */


/**
 * Copies the given range of columns to a new location, offset from
 * the original by the given number of columns.
 */
void guac_terminal_copy_columns(guac_terminal* terminal, int row,
        int start_column, int end_column, int offset);

/**
 * Copies the given range of rows to a new location, offset from the
 * original by the given number of rows.
 */
void guac_terminal_copy_rows(guac_terminal* terminal,
        int start_row, int end_row, int offset);

/**
 * Sets the given range of columns within the given row to the given
 * character.
 */
void guac_terminal_set_columns(guac_terminal* terminal, int row,
        int start_column, int end_column, guac_terminal_char* character);

/**
 * Resize the terminal to the given dimensions.
 */
int guac_terminal_resize(guac_terminal* term, int width, int height);

/**
 * Flushes all pending operations within the given guac_terminal.
 */
void guac_terminal_flush(guac_terminal* terminal);

/**
 * Sends the given string as if typed by the user. 
 */
int guac_terminal_send_data(guac_terminal* term, const char* data, int length);

/**
 * Sends the given string as if typed by the user. 
 */
int guac_terminal_send_string(guac_terminal* term, const char* data);

/**
 * Sends data through STDIN as if typed by the user, using the format
 * string given and any args (similar to printf).
 */
int guac_terminal_sendf(guac_terminal* term, const char* format, ...);

/**
 * Sets a tabstop in the given column.
 */
void guac_terminal_set_tab(guac_terminal* term, int column);

/**
 * Removes the tabstop at the given column.
 */
void guac_terminal_unset_tab(guac_terminal* term, int column);

/**
 * Removes all tabstops.
 */
void guac_terminal_clear_tabs(guac_terminal* term);

/**
 * Given a column within the given terminal, returns the location of the
 * next tabstop (or the rightmost character, if no more tabstops exist).
 */
int guac_terminal_next_tab(guac_terminal* term, int column);

#endif

