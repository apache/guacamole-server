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


#ifndef _GUAC_TERMINAL_DISPLAY_H
#define _GUAC_TERMINAL_DISPLAY_H

#include "config.h"

#include "common/surface.h"
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
 * The index of black within the terminal color palette.
 */
#define GUAC_TERMINAL_COLOR_BLACK 0

/**
 * The index of low-intensity red within the terminal color palette.
 */
#define GUAC_TERMINAL_COLOR_DARK_RED 1

/**
 * The index of low-intensity green within the terminal color palette.
 */
#define GUAC_TERMINAL_COLOR_DARK_GREEN 2

/**
 * The index of brown within the terminal color palette.
 */
#define GUAC_TERMINAL_COLOR_BROWN 3

/**
 * The index of low-intensity blue within the terminal color palette.
 */
#define GUAC_TERMINAL_COLOR_DARK_BLUE 4

/**
 * The index of low-intensity magenta (purple) within the terminal color
 * palette.
 */
#define GUAC_TERMINAL_COLOR_PURPLE 5

/**
 * The index of low-intensity cyan (teal) within the terminal color palette.
 */
#define GUAC_TERMINAL_COLOR_TEAL 6

/**
 * The index of low-intensity white (gray) within the terminal color palette.
 */
#define GUAC_TERMINAL_COLOR_GRAY 7

/**
 * The index of bright black (dark gray) within the terminal color palette.
 */
#define GUAC_TERMINAL_COLOR_DARK_GRAY 8

/**
 * The index of bright red within the terminal color palette.
 */
#define GUAC_TERMINAL_COLOR_RED 9

/**
 * The index of bright green within the terminal color palette.
 */
#define GUAC_TERMINAL_COLOR_GREEN 10

/**
 * The index of bright brown (yellow) within the terminal color palette.
 */
#define GUAC_TERMINAL_COLOR_YELLOW 11

/**
 * The index of bright blue within the terminal color palette.
 */
#define GUAC_TERMINAL_COLOR_BLUE 12

/**
 * The index of bright magenta within the terminal color palette.
 */
#define GUAC_TERMINAL_COLOR_MAGENTA 13

/**
 * The index of bright cyan within the terminal color palette.
 */
#define GUAC_TERMINAL_COLOR_CYAN 14

/**
 * The index of bright white within the terminal color palette.
 */
#define GUAC_TERMINAL_COLOR_WHITE 15

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
     * Default foreground color for all glyphs.
     */
    int default_foreground;

    /**
     * Default background color for all glyphs and the terminal itself.
     */
    int default_background;

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
 * Initializes and syncs the current terminal display state for the given user
 * that has just joined the connection, sending the necessary instructions to
 * completely recreate and redraw the terminal rendering over the given socket.
 *
 * @param display
 *     The terminal display to sync to the given user.
 *
 * @param user
 *     The user that has just joined the connection.
 *
 * @param socket
 *     The socket over which any necessary instructions should be sent.
 */
void guac_terminal_display_dup(guac_terminal_display* display, guac_user* user,
        guac_socket* socket);

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

