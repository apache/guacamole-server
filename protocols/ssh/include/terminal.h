
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

#ifndef _SSH_GUAC_TERMINAL_H
#define _SSH_GUAC_TERMINAL_H

#include <stdbool.h>
#include <pthread.h>

#include <pango/pangocairo.h>

#include <guacamole/client.h>

#define GUAC_SSH_WHEEL_SCROLL_AMOUNT 3

typedef struct guac_terminal guac_terminal;

/**
 * Handler for characters printed to the terminal. When a character is printed,
 * the current char handler for the terminal is called and given that
 * character.
 */
typedef int guac_terminal_char_handler(guac_terminal* term, char c);

/**
 * An RGB color, where each component ranges from 0 to 255.
 */
typedef struct guac_terminal_color {

    /**
     * The red component of this color.
     */
    int red;

    /**
     * The green component of this color.
     */
    int green;

    /**
     * The blue component of this color.
     */
    int blue;

} guac_terminal_color;

/**
 * Terminal attributes, as can be applied to a single character.
 */
typedef struct guac_terminal_attributes {

    /**
     * Whether the character should be rendered bold.
     */
    bool bold;

    /**
     * Whether the character should be rendered with reversed colors
     * (background becomes foreground and vice-versa).
     */
    bool reverse;

    /**
     * Whether the associated character is selected.
     */
    bool selected;

    /**
     * Whether to render the character with underscore.
     */
    bool underscore;

    /**
     * The foreground color of this character, as a palette index.
     */
    int foreground;

    /**
     * The background color of this character, as a palette index.
     */
    int background;

} guac_terminal_attributes;

/**
 * The available color palette. All integer colors within structures
 * here are indices into this palette.
 */
extern const guac_terminal_color guac_terminal_palette[16];

/**
 * Represents a single character for display in a terminal, including actual
 * character value, foreground color, and background color.
 */
typedef struct guac_terminal_char {

    /**
     * The character value of the character to display.
     */
    char value;

    /**
     * The attributes of the character to display.
     */
    guac_terminal_attributes attributes;

} guac_terminal_char;

/**
 * All available terminal operations which affect character cells.
 */
typedef enum guac_terminal_operation_type {

    /**
     * Operation which does nothing.
     */
    GUAC_CHAR_NOP,

    /**
     * Operation which copies a character from a given row/column coordinate.
     */
    GUAC_CHAR_COPY,

    /**
     * Operation which sets the character and attributes.
     */
    GUAC_CHAR_SET

} guac_terminal_operation_type;

/**
 * A pairing of a guac_terminal_operation_type and all parameters required by
 * that operation type.
 */
typedef struct guac_terminal_operation {

    /**
     * The type of operation to perform.
     */
    guac_terminal_operation_type type;

    /**
     * The character (and attributes) to set the current location to. This is
     * only applicable to GUAC_CHAR_SET.
     */
    guac_terminal_char character;

    /**
     * The row to copy a character from. This is only applicable to
     * GUAC_CHAR_COPY.
     */
    int row;

    /**
     * The column to copy a character from. This is only applicable to
     * GUAC_CHAR_COPY.
     */
    int column;

} guac_terminal_operation;

/**
 * Set of all pending operations for the currently-visible screen area.
 */
typedef struct guac_terminal_delta {

    /**
     * Array of all operations pending for the visible screen area.
     */
    guac_terminal_operation* operations;

    /**
     * Scratch area of same size as the operations buffer, facilitating copies
     * of overlapping regions.
     */
    guac_terminal_operation* scratch;

    /**
     * The width of the screen, in characters.
     */
    int width;

    /**
     * The height of the screen, in characters.
     */
    int height;

} guac_terminal_delta;

/**
 * A single variable-length row of terminal data.
 */
typedef struct guac_terminal_scrollback_row {

    /**
     * Array of guac_terminal_char representing the contents of the row.
     */
    guac_terminal_char* characters;

    /**
     * The length of this row in characters. This is the number of initialized
     * characters in the buffer, usually equal to the number of characters
     * in the screen width at the time this row was created.
     */
    int length;

    /**
     * The number of elements in the characters array. After the length
     * equals this value, the array must be resized.
     */
    int available;

} guac_terminal_scrollback_row;

/**
 * A scrollback buffer containing a constant number of arbitrary-length rows.
 * New rows can be appended to the buffer, with the oldest row replaced with
 * the new row.
 */
typedef struct guac_terminal_scrollback_buffer {

    /**
     * Array of scrollback buffer rows. This array functions as a ring buffer.
     * When a new row needs to be appended, the top reference is moved down
     * and the old top row is replaced.
     */
    guac_terminal_scrollback_row* scrollback;

    /**
     * The number of rows in the scrollback buffer. This is the total capacity
     * of the buffer.
     */
    int rows;

    /**
     * The row to replace when adding a new row to the scrollback.
     */
    int top;

    /**
     * The number of rows currently stored in the scrollback buffer.
     */
    int length;

} guac_terminal_scrollback_buffer;

/**
 * Dynamically-resizable character buffer.
 */
typedef struct guac_terminal_buffer {

    /**
     * Array of characters.
     */
    guac_terminal_char* characters;

    /**
     * The width of this buffer in characters.
     */
    int width;

    /**
     * The height of this buffer in characters.
     */
    int height;

} guac_terminal_buffer;

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
     * Lock which restricts simultaneous access to this terminal via the root
     * guac_terminal_* functions.
     */
    pthread_mutex_t lock;

    /**
     * The description of the font to use for rendering.
     */
    PangoFontDescription* font_desc;

    /**
     * Index of next glyph to create
     */
    int next_glyph;

    /**
     * Index of locations for each glyph in the stroke and fill layers.
     */
    int glyphs[256];

    /**
     * Color of glyphs in copy buffer
     */
    int glyph_foreground;

    /**
     * Color of glyphs in copy buffer
     */
    int glyph_background;

    /**
     * A single wide layer holding each glyph, with each glyph only
     * colored with foreground color (background remains transparent).
     */
    guac_layer* glyph_stroke;

    /**
     * A single wide layer holding each glyph, with each glyph properly
     * colored with foreground and background color (no transparency at all).
     */
    guac_layer* filled_glyphs;

    /**
     * The scrollback buffer.
     */
    guac_terminal_scrollback_buffer* scrollback;

    /**
     * The relative offset of the display. A positive value indicates that
     * many rows have been scrolled into view, zero indicates that no
     * scrolling has occurred. Negative values are illegal.
     */
    int scroll_offset;

    /**
     * The width of each character, in pixels.
     */
    int char_width;

    /**
     * The height of each character, in pixels.
     */
    int char_height;

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
     * The attributes which will be applied to future characters.
     */
    guac_terminal_attributes current_attributes;

    /**
     * The attributes which will be applied to characters by default, unless
     * other attributes are explicitly specified.
     */
    guac_terminal_attributes default_attributes;

    /**
     * Handler which will receive all printed characters, updating the terminal
     * accordingly.
     */
    guac_terminal_char_handler* char_handler;

    /**
     * The difference between the currently-rendered screen and the current
     * state of the terminal.
     */
    guac_terminal_delta* delta;

    /**
     * Current terminal display state. All characters present on the screen
     * are within this buffer. This has nothing to do with the delta, which
     * facilitates transfer of a set of changes to the remote display.
     */
    guac_terminal_buffer* buffer;

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
     * The row that the selection ends at.
     */
    int selection_end_row;

    /**
     * The column that the selection ends at.
     */
    int selection_end_column;

};

/**
 * Creates a new guac_terminal, having the given width and height, and
 * rendering to the given client.
 */
guac_terminal* guac_terminal_create(guac_client* client,
        int width, int height);

/**
 * Frees all resources associated with the given terminal.
 */
void guac_terminal_free(guac_terminal* term);

/**
 * Writes the given string of characters to the terminal.
 */
int guac_terminal_write(guac_terminal* term, const char* c, int size);

/**
 * Sets the character at the given row and column to the specified value.
 */
int guac_terminal_set(guac_terminal* term, int row, int col, char c);

/**
 * Copies a rectangular region of characters which may overlap with the
 * destination.
 */
int guac_terminal_copy(guac_terminal* term,
        int src_row, int src_col, int rows, int cols,
        int dst_row, int dst_col);

/**
 * Clears a rectangular region of characters, replacing them with the
 * current background color and attributes.
 */
int guac_terminal_clear(guac_terminal* term,
        int row, int col, int rows, int cols);

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
 * Toggles the reverse attribute of the character at the given location.
 */
int guac_terminal_toggle_reverse(guac_terminal* term, int row, int col);

/**
 * Allocates a new guac_terminal_delta.
 */
guac_terminal_delta* guac_terminal_delta_alloc(int width, int height);

/**
 * Frees the given guac_terminal_delta.
 */
void guac_terminal_delta_free(guac_terminal_delta* delta);

/**
 * Resizes the given guac_terminal_delta to the given dimensions.
 */
void guac_terminal_delta_resize(guac_terminal_delta* delta,
    int width, int height);

/**
 * Stores a set operation at the given location.
 */
void guac_terminal_delta_set(guac_terminal_delta* delta, int r, int c,
        guac_terminal_char* character);

/**
 * Stores a rectangle of copy operations, copying existing operations as
 * necessary.
 */
void guac_terminal_delta_copy(guac_terminal_delta* delta,
        int dst_row, int dst_column,
        int src_row, int src_column,
        int w, int h);

/**
 * Sets a rectangle of character data to the given character value.
 */
void guac_terminal_delta_set_rect(guac_terminal_delta* delta,
        int row, int column, int w, int h,
        guac_terminal_char* character);

/**
 * Flushes all pending operations within the given guac_client_delta to the
 * given guac_terminal.
 */
void guac_terminal_delta_flush(guac_terminal_delta* delta,
        guac_terminal* terminal);

/**
 * Allocates a new character buffer having the given dimensions.
 */
guac_terminal_buffer* guac_terminal_buffer_alloc(int width, int height);

/**
 * Resizes the given character buffer to the given dimensions.
 */
void guac_terminal_buffer_resize(guac_terminal_buffer* buffer, 
        int width, int height);

/**
 * Sets the character at the given location within the buffer to the given
 * value.
 */
void guac_terminal_buffer_set(guac_terminal_buffer* buffer, int r, int c,
        guac_terminal_char* character);

/**
 * Copies a rectangle of character data within the buffer. The source and
 * destination may overlap.
 */
void guac_terminal_buffer_copy(guac_terminal_buffer* buffer,
        int dst_row, int dst_column,
        int src_row, int src_column,
        int w, int h);

/**
 * Sets a rectangle of character data to the given character value.
 */
void guac_terminal_buffer_set_rect(guac_terminal_buffer* buffer,
        int row, int column, int w, int h,
        guac_terminal_char* character);

/**
 * Frees the given character buffer.
 */
void guac_terminal_buffer_free(guac_terminal_buffer* buffer);

/**
 * Allocates a new scrollback buffer having the given number of rows.
 */
guac_terminal_scrollback_buffer*
    guac_terminal_scrollback_buffer_alloc(int rows);

/**
 * Frees the given scrollback buffer.
 */
void guac_terminal_scrollback_buffer_free(
    guac_terminal_scrollback_buffer* buffer);

/**
 * Pushes the given number of rows into the scrollback, maintaining display
 * position within the scrollback as possible.
 */
void guac_terminal_scrollback_buffer_append(
    guac_terminal_scrollback_buffer* buffer,
    guac_terminal* terminal, int rows);

/**
 * Returns the row within the scrollback at the given location. The index
 * of the row given is a negative number, denoting the number of rows into
 * the past to look.
 */
guac_terminal_scrollback_row* guac_terminal_scrollback_buffer_get_row(
    guac_terminal_scrollback_buffer* buffer, int row);

/**
 * Scroll down the display by the given amount, replacing the new space with
 * data from the scrollback. If not enough data is available, the maximum
 * amount will be scrolled.
 */
void guac_terminal_scroll_display_down(guac_terminal* terminal, int amount);

/**
 * Scroll up the display by the given amount, replacing the new space with data
 * from either the scrollback or the terminal buffer.  If not enough data is
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
 * Ends text selection, removing any highlight.
 */
void guac_terminal_select_end(guac_terminal* terminal);

/**
 * Returns a row of character data, whether that data be from the scrollback buffer or the main backing buffer. The length
 * parameter given here is a pointer to the int variable that should receive the length of the character array returned.
 */
guac_terminal_char* guac_terminal_get_row(guac_terminal* terminal, int row, int* length);

#endif

