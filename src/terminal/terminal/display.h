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

/**
 * Structures and function definitions related to the graphical display.
 *
 * @file display.h
 */

#include "buffer.h"
#include "palette.h"
#include "terminal.h"
#include "types.h"

#include <guacamole/client.h>
#include <guacamole/display.h>
#include <guacamole/layer.h>
#include <pango/pangocairo.h>

#include <stdbool.h>
#include <stdint.h>

/**
 * The maximum width of any character, in columns.
 */
#define GUAC_TERMINAL_MAX_CHAR_WIDTH 2

/**
 * The size of margins between the console text and the border in mm.
 */
#define GUAC_TERMINAL_MARGINS 2

/**
 * 1 inch is 25.4 millimeters, and we can therefore use the following
 * to create a mm to px formula: (mm × dpi) ÷ 25.4 = px.
 */
#define GUAC_TERMINAL_MM_PER_INCH 25.4

/**
 * The rendering area and state of the text display used by the terminal
 * emulator. The actual changes between successive frames are tracked by an
 * underlying guac_display.
 */
typedef struct guac_terminal_display {

    /**
     * The Guacamole client associated with this terminal emulator having this
     * display.
     */
    guac_client* client;

    /**
     * The width of the screen, in characters.
     */
    int width;

    /**
     * The height of the screen, in characters.
     */
    int height;

    /**
     * The size of margins between the console text and the border in pixels.
     */
    int margin;

    /**
     * The current mouse cursor (the mouse cursor already sent to connected
     * users), to avoid re-setting the cursor image when effectively no change
     * has been made.
     */
    guac_terminal_cursor_type current_cursor;

    /**
     * The mouse cursor that was most recently requested.
     */
    guac_terminal_cursor_type last_requested_cursor;

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
     * The current palette.
     */
    guac_terminal_color palette[256];

    /**
     * The default palette. Use GUAC_TERMINAL_INITIAL_PALETTE if null.
     * Must free on destruction if not null.
     */
    guac_terminal_color (*default_palette)[256];

    /**
     * Default foreground color for all glyphs.
     */
    guac_terminal_color default_foreground;

    /**
     * Default background color for all glyphs and the terminal itself.
     */
    guac_terminal_color default_background;

    /**
     * The Guacamole display that this terminal emulator should render to.
     */
    guac_display* graphical_display;

    /**
     * Layer which contains the actual terminal.
     */
    guac_display_layer* display_layer;

} guac_terminal_display;

/**
 * Allocates a new display having the given text rendering properties and
 * underlying graphical display.
 *
 * @param client
 *     The guac_client associated with the terminal session.
 *
 * @param graphical_display
 *     The guac_display that the new guac_terminal_display should render to.
 *
 * @param font_name
 *     The name of the font to use to render characters.
 *
 * @param font_size
 *     The font size to use when rendering characters, in points.
 *
 * @param dpi
 *     The resolution that characters should be rendered at, in DPI (dots per
 *     inch).
 *
 * @param foreground
 *     The default foreground color to use for characters rendered to the
 *     display.
 *
 * @param background
 *     The default background color to use for characters rendered to the
 *     display.
 *
 * @param palette
 *     The palette to use for all other colors supported by the terminal.
 */
guac_terminal_display* guac_terminal_display_alloc(guac_client* client,
        guac_display* graphical_display,
        const char* font_name, int font_size, int dpi,
        guac_terminal_color* foreground, guac_terminal_color* background,
        guac_terminal_color (*palette)[256]);

/**
 * Frees the given display.
 */
void guac_terminal_display_free(guac_terminal_display* display);

/**
 * Resets the palette of the given display to the initial, default color
 * values, as defined by default_palette or GUAC_TERMINAL_INITIAL_PALETTE.
 *
 * @param display
 *     The display to reset.
 */
void guac_terminal_display_reset_palette(guac_terminal_display* display);

/**
 * Replaces the color in the palette at the given index with the given color.
 * If the index is invalid, the assignment is ignored.
 *
 * @param display
 *     The display whose palette is being changed.
 *
 * @param index
 *     The index of the palette entry to change.
 *
 * @param color
 *     The color to assign to the palette entry having the given index.
 *
 * @returns
 *     Zero if the assignment was successful, non-zero if the assignment
 *     failed.
 */
int guac_terminal_display_assign_color(guac_terminal_display* display,
        int index, const guac_terminal_color* color);

/**
 * Retrieves the color within the palette at the given index, if such a color
 * exists. If the index is invalid, no color is retrieved.
 *
 * @param display
 *     The display whose palette contains the color to be retrieved.
 *
 * @param index
 *     The index of the palette entry to retrieve.
 *
 * @param color
 *     A pointer to a guac_terminal_color structure which should receive the
 *     color retrieved from the palette.
 *
 * @returns
 *     Zero if the color was successfully retrieved, non-zero otherwise.
 */
int guac_terminal_display_lookup_color(guac_terminal_display* display,
        int index, guac_terminal_color* color);

/**
 * Resize the terminal to the given dimensions.
 */
void guac_terminal_display_resize(guac_terminal_display* display, int width, int height);

/**
 * Alters the font of the terminal display. The available display area and the
 * regular grid of character cells will be resized as necessary to compensate
 * for any changes in font metrics.
 *
 * If successful, the terminal itself MUST be manually resized to take into
 * account the new character dimensions, and MUST be manually redrawn. Failing
 * to do so will result in graphical artifacts.
 *
 * @param display
 *     The display whose font family and/or size are being changed.
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
 *
 * @return
 *     Zero if the font was successfully changed, non-zero otherwise.
 */
int guac_terminal_display_set_font(guac_terminal_display* display,
        const char* font_name, int font_size, int dpi);

/**
 * Renders the contents of the given buffer to the given terminal display. All
 * characters within the buffer that fit within the display region will be
 * rendered.
 *
 * @param display
 *     The terminal display receiving the buffer contents.
 *
 * @param buffer
 *     The buffer to render to the display.
 *
 * @param scroll_offset
 *     The number of rows from the scrollback buffer that the user has scrolled
 *     into view.
 *
 * @param default_char
 *     The character that should be used to populate any character cell that
 *     has not received any terminal output.
 *
 * @param cursor_visible
 *     Whether the cursor is currently visible (ie: has not been hidden using
 *     console codes that specifically hide the cursor). This does NOT refer to
 *     whether the cursor is within the display region, which is handled
 *     automatically.
 *
 * @oaran cursor_row
 *     The current row position of the cursor.
 *
 * @oaran cursor_col
 *     The current column position of the cursor.
 *
 * @param text_selected
 *     Whether the user has selected text.
 *
 * @param selection_start_row
 *     The row number where the user started their selection. This value only
 *     has meaning if text_selected is true. There is no requirement that the
 *     start row be less than the end row.
 *
 * @param selection_start_col
 *     The column number where the user started their selection. This value
 *     only has meaning if text_selected is true. There is no requirement that
 *     the start column be less than the end column.
 *
 * @param selection_end_row
 *     The row number where the user ended their selection. This value only has
 *     meaning if text_selected is true. There is no requirement that the end
 *     row be greated than the start row.
 *
 * @param selection_end_col
 *     The column number where the user ended their selection. This value only
 *     has meaning if text_selected is true. There is no requirement that the
 *     end column be greater than the start column.
 */
void guac_terminal_display_render_buffer(guac_terminal_display* display,
        guac_terminal_buffer* buffer, int scroll_offset,
        guac_terminal_char* default_char,
        bool cursor_visible, int cursor_row, int cursor_col,
        bool text_selected, int selection_start_row, int selection_start_col,
        int selection_end_row, int selection_end_col);

/**
 * Set the mouse cursor icon. If different from the mouse cursor in effect at
 * the time of the previous guac_display frame, the requested cursor will take
 * effect the next time the terminal display is flushed.
 *
 * @param display
 *     The display to set the cursor of.
 *
 * @param cursor
 *     The cursor to assign.
 */
void guac_terminal_display_set_cursor(guac_terminal_display* display,
        guac_terminal_cursor_type cursor);

#endif

