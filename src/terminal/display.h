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


#ifndef _GUAC_TERMINAL_DISPLAY_H
#define _GUAC_TERMINAL_DISPLAY_H

#include "config.h"

#include "guac_surface.h"
#include "types.h"

#include <guacamole/client.h>
#include <guacamole/layer.h>
#include <pango/pangocairo.h>

#include <stdbool.h>

/**
 * The maximum width of any character, in columns.
 */
#define GUAC_TERMINAL_MAX_CHAR_WIDTH 2

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
     * Color of glyphs in copy buffer
     */
    int glyph_foreground;

    /**
     * Color of glyphs in copy buffer
     */
    int glyph_background;

    /**
     * The surface containing the actual terminal.
     */
    guac_common_surface* display_surface;

    /**
     * Layer which contains the actual terminal.
     */
    guac_layer* display_layer;

    /**
     * Sub-layer of display layer which highlights selected text.
     */
    guac_layer* select_layer;

    /**
     * Whether text is being selected.
     */
    bool text_selected;

    /**
     * Whether the selection is finished, and will no longer be modified. A
     * committed selection remains highlighted for reference, but the
     * highlight will be removed when the display changes.
     */
    bool selection_committed;

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

} guac_terminal_display;

/**
 * Allocates a new display having the given default foreground and background
 * colors.
 */
guac_terminal_display* guac_terminal_display_alloc(guac_client* client,
        const char* font_name, int font_size, int dpi,
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
void guac_terminal_display_copy_rows(guac_terminal_display* display,
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
void guac_terminal_display_resize(guac_terminal_display* display, int width, int height);

/**
 * Flushes all pending operations within the given guac_terminal_display.
 */
void guac_terminal_display_flush(guac_terminal_display* display);

/**
 * Draws the text selection rectangle from the given coordinates to the given end coordinates.
 */
void guac_terminal_display_select(guac_terminal_display* display,
        int start_row, int start_col, int end_row, int end_col);

/**
 * Commits the select rectangle, allowing the display to clear it when
 * necessary.
 */
void guac_terminal_display_commit_select(guac_terminal_display* display);

#endif

