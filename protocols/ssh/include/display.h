
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

#ifndef _SSH_GUAC_DISPLAY_H
#define _SSH_GUAC_DISPLAY_H

#include <pango/pangocairo.h>
#include <guacamole/client.h>

#include "types.h"

/**
 * The available color palette. All integer colors within structures
 * here are indices into this palette.
 */
extern const guac_terminal_color guac_terminal_palette[16];

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
typedef struct guac_terminal_display {

    /**
     * The Guacamole client this display will use for rendering.
     */
    guac_client* client;

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

    /**
     * The description of the font to use for rendering.
     */
    PangoFontDescription* font_desc;

    /**
     * The width of each character, in pixels.
     */
    int char_width;

    /**
     * The height of each character, in pixels.
     */
    int char_height;

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

} guac_terminal_display;

/**
 * Allocates a new display having the given dimensions.
 */
guac_terminal_display* guac_terminal_display_alloc(guac_client* client, int width, int height,
        int foreground, int background);

/**
 * Frees the given display.
 */
void guac_terminal_display_free(guac_terminal_display* display);

/**
 * Copies the given range of columns to a new location, offset from
 * the original by the given number of columns.
 */
void guac_terminal_display_copy_columns(guac_terminal_display* display, int row,
        int start_column, int end_column, int offset);

/**
 * Copies the given range of rows to a new location, offset from the
 * original by the given number of rows.
 */
void guac_terminal_display_copy_rows(guac_terminal_display* display, int src_row, int rows,
        int start_row, int end_row, int offset);

/**
 * Sets the given range of columns within the given row to the given
 * character.
 */
void guac_terminal_display_set_columns(guac_terminal_display* display, int row,
        int start_column, int end_column, guac_terminal_char* character);

/**
 * Resize the terminal to the given dimensions.
 */
void guac_terminal_display_resize(guac_terminal_display* display, int rows, int cols);

/**
 * Flushes all pending operations within the given guac_terminal_display.
 */
void guac_terminal_display_flush(guac_terminal_display* display);

#endif

