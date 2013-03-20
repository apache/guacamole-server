
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

#include <pango/pangocairo.h>

#include <guacamole/client.h>

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
 * Represents a terminal emulator which uses a given Guacamole client to
 * render itself.
 */
struct guac_terminal {

    /**
     * The Guacamole client this terminal emulator will use for rendering.
     */
    guac_client* client;

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
     * Array of scrollback buffer rows, where each row is an array of 
     * characters.
     */
    guac_terminal_char** scrollback;

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
     * Simple cursor layer until scrollback, etc. is implemented.
     */
    guac_layer* cursor_layer;

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

int guac_terminal_set_colors(guac_terminal* term,
        int foreground, int background);

int guac_terminal_set(guac_terminal* term, int row, int col, char c);

int guac_terminal_copy(guac_terminal* term,
        int src_row, int src_col, int rows, int cols,
        int dst_row, int dst_col);

int guac_terminal_clear(guac_terminal* term,
        int row, int col, int rows, int cols, int background_color);

int guac_terminal_scroll_up(guac_terminal* term,
        int start_row, int end_row, int amount);

int guac_terminal_scroll_down(guac_terminal* term,
        int start_row, int end_row, int amount);

int guac_terminal_clear_range(guac_terminal* term,
        int start_row, int start_col,
        int end_row, int end_col, int background_color);

#endif

