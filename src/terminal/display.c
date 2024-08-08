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

#include "config.h"
#include "guacamole/rect.h"
#include "terminal/common.h"
#include "terminal/display.h"
#include "terminal/palette.h"
#include "terminal/terminal.h"
#include "terminal/terminal-priv.h"
#include "terminal/types.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include <cairo/cairo.h>
#include <glib-object.h>
#include <guacamole/client.h>
#include <guacamole/display.h>
#include <guacamole/mem.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <pango/pangocairo.h>

/**
 * The palette index of the color to use when highlighting selected text.
 */
#define GUAC_TERMINAL_HIGHLIGHT_COLOR 4

/**
 * Calculates the approximate luminance of the given color, where 0 represents
 * no luminance and 255 represents full luminance.
 *
 * @param color
 *     The color to calculate the luminance of.
 *
 * @return
 *     The approximate luminance of the given color, on a scale of 0 through
 *     255 inclusive.
 */
static int guac_terminal_color_luminance(const guac_terminal_color* color) {

    /*
     * Y = 0.2126 R + 0.7152 G + 0.0722 B
     *
     * Here we multiply all coefficients by 16 to approximate luminance without
     * having to resort to floating point, rounding to the nearest integer that
     * minimizes error but still totals 16 when added to the other
     * coefficients.
     */

    return (3 * color->red + 12 * color->green + color->blue) / 16;

}

/**
 * Given the foreground and background colors of a character, updates those
 * colors to represent the fact that the character has been highlighted
 * (selected by the user).
 *
 * @param display
 *     The terminal display containing the character.
 *
 * @param glyph_foreground
 *     The foreground color of the character. The contents of this color may be
 *     modified to represent the effect of the highlight.
 *
 * @param glyph_background
 *     The background color of the character. The contents of this color may be
 *     modified to represent the effect of the highlight.
 */
static void guac_terminal_display_apply_highlight(guac_terminal_display* display,
        guac_terminal_color* glyph_foreground, guac_terminal_color* glyph_background) {

    guac_terminal_color highlight;
    guac_terminal_display_lookup_color(display, GUAC_TERMINAL_HIGHLIGHT_COLOR, &highlight);

    highlight.red   = (highlight.red   + glyph_background->red)   / 2;
    highlight.green = (highlight.green + glyph_background->green) / 2;
    highlight.blue  = (highlight.blue  + glyph_background->blue)  / 2;

    int foreground_lum = guac_terminal_color_luminance(glyph_foreground);
    int background_lum = guac_terminal_color_luminance(glyph_background);
    int highlight_lum = guac_terminal_color_luminance(&highlight);

    /* Replace background color for highlight color only if it's closer in
     * perceived luminance to the backgrund color than it is to the
     * foreground color (to preserve roughly the same degree of contrast) */
    if (abs(foreground_lum - highlight_lum) >= abs(background_lum - highlight_lum)) {
        *glyph_background = highlight;
    }

    /* If the highlight color can't be used while preserving contrast,
     * simply inverting the colors will do the job */
    else {
        guac_terminal_color temp = *glyph_background;
        *glyph_background = *glyph_foreground;
        *glyph_foreground = temp;
    }

}

/**
 * Given current attributes of a character, assigns foreground and background
 * colors to represent that character state.
 *
 * @param display
 *     The terminal display containing the character.
 *
 * @param attributes
 *     All attributes associated with the character (bold, foreground color,
 *     background color, etc.).
 *
 * @param is_cursor
 *     Whether the terminal cursor is currently on top of the character.
 *
 * @param is_selected
 *     Whether the user currently has this character selected.
 *
 * @param glyph_foreground
 *     A pointer to the guac_terminal_color that should receive the foreground
 *     color of the character.
 *
 * @param glyph_background
 *     A pointer to the guac_terminal_color that should receive the background
 *     color of the character.
 */
static void guac_terminal_display_apply_render_attributes(guac_terminal_display* display,
        guac_terminal_attributes* attributes, bool is_cursor, bool is_selected,
        guac_terminal_color* glyph_foreground,
        guac_terminal_color* glyph_background) {

    const guac_terminal_color* background;
    const guac_terminal_color* foreground;

    /* Swap foreground and background color to represent reverse video and the
     * cursor (this means that reverse and is_cursor cancel each other out) */
    if (is_cursor ? !attributes->reverse : attributes->reverse) {
        background = &attributes->foreground;
        foreground = &attributes->background;
    }
    else {
        foreground = &attributes->foreground;
        background = &attributes->background;
    }

    /* Represent bold with the corresponding intense (brighter) color */
    if (attributes->bold && !attributes->half_bright
            && foreground->palette_index >= GUAC_TERMINAL_FIRST_DARK
            && foreground->palette_index <= GUAC_TERMINAL_LAST_DARK) {
        foreground = &display->palette[foreground->palette_index
            + GUAC_TERMINAL_INTENSE_OFFSET];
    }

    *glyph_foreground = *foreground;
    guac_terminal_display_lookup_color(display,
            foreground->palette_index, glyph_foreground);

    *glyph_background = *background;
    guac_terminal_display_lookup_color(display,
            background->palette_index, glyph_background);

    /* Modify color if half-bright (low intensity) */
    if (attributes->half_bright && !attributes->bold) {
        glyph_foreground->red   /= 2;
        glyph_foreground->green /= 2;
        glyph_foreground->blue  /= 2;
    }

    /* Apply highlight if selected (NOTE: We re-swap foreground/background
     * again here if the cursor is selected, as the sudden appearance of
     * foreground color for an otherwise invisible character is surprising
     * behavior) */
    if (is_selected) {
        if (is_cursor)
            guac_terminal_display_apply_highlight(display, glyph_background, glyph_foreground);
        else
            guac_terminal_display_apply_highlight(display, glyph_foreground, glyph_background);
    }

}

/**
 * Renders a single character at the given row and column. The character is
 * rendered immediately to the underlying guac_display and will be sent to
 * connected users when the next guac_display frame is completed.
 *
 * @param display
 *     The teriminal display receiving the character.
 *
 * @param row
 *     The row coordinate of the character, where 0 is the top-most row. While
 *     negative values generally represent rows of the scrollback buffer,
 *     supplying negative values here would result in rendering outside the
 *     visible display area and would be nonsensical.
 *
 * @param col
 *     The column coordinate of the character, where 0 is the left-most column.
 *
 * @param c
 *     The character to render.
 *
 * @param is_cursor
 *     Whether the terminal cursor is currently on top of the character.
 *
 * @param is_selected
 *     Whether the user currently has this character selected.
 */
static void guac_terminal_display_render_glyph(guac_terminal_display* display, int row, int col,
        guac_terminal_char* c, bool is_cursor, bool is_selected) {

    /* Use space if no glyph */
    int codepoint = c->value;
    if (!guac_terminal_has_glyph(codepoint))
        codepoint = ' ';

    /* Calculate width in columns */
    int width = wcwidth(codepoint);
    if (width < 0)
        width = 1;

    /* Do nothing if glyph is empty */
    if (width == 0)
        return;

    /* Convert to UTF-8 */
    char utf8[4];
    int bytes = guac_terminal_encode_utf8(codepoint, utf8);

    int glyph_x = display->char_width * col;
    int glyph_y = display->char_height * row;
    int glyph_width = width * display->char_width;
    int glyph_height = display->char_height;

    int ideal_layout_width = glyph_width * PANGO_SCALE;
    int ideal_layout_height = glyph_height * PANGO_SCALE;

    guac_display_layer_cairo_context* context = guac_display_layer_open_cairo(display->display_layer);
    cairo_t* cairo = context->cairo;

    cairo_identity_matrix(cairo);
    cairo_translate(cairo, glyph_x, glyph_y);

    guac_terminal_color foreground, background;
    guac_terminal_display_apply_render_attributes(display, &c->attributes,
            is_cursor, is_selected, &foreground, &background);

    /* Fill background */
    cairo_set_source_rgb(cairo,
            background.red   / 255.0,
            background.green / 255.0,
            background.blue  / 255.0);

    cairo_rectangle(cairo, 0, 0, glyph_width, glyph_height);
    cairo_fill(cairo);

    /* Get layout */
    PangoLayout* layout = pango_cairo_create_layout(cairo);
    pango_layout_set_font_description(layout, display->font_desc);
    pango_layout_set_text(layout, utf8, bytes);
    pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);

    int layout_width, layout_height;
    pango_layout_get_size(layout, &layout_width, &layout_height);

    /* If layout bigger than available space, scale it back */
    if (layout_width > ideal_layout_width || layout_height > ideal_layout_height) {

        double scale = fmin(ideal_layout_width  / (double) layout_width,
                            ideal_layout_height / (double) layout_height);

        cairo_scale(cairo, scale, scale);

        /* Update layout to reflect scaled surface */
        pango_layout_set_width(layout, ideal_layout_width / scale);
        pango_layout_set_height(layout, ideal_layout_height / scale);
        pango_cairo_update_layout(cairo, layout);

    }

    /* Draw */
    cairo_set_source_rgb(cairo,
            foreground.red   / 255.0,
            foreground.green / 255.0,
            foreground.blue  / 255.0);

    cairo_move_to(cairo, 0.0, 0.0);
    pango_cairo_show_layout(cairo, layout);

    /* Free all */
    g_object_unref(layout);

    guac_rect char_rect;
    guac_rect_init(&char_rect, glyph_x, glyph_y, glyph_width, glyph_height);
    guac_rect_extend(&context->dirty, &char_rect);

    guac_display_layer_close_cairo(display->display_layer, context);

}

/**
 * Calculate the size of margins around the terminal based on DPI.
 *
 * @param dpi
 *     The resolution of the display in DPI.
 *
 * @return
 *     Calculated size of margin in pixels.
 */
static int get_margin_by_dpi(int dpi) {
    return dpi * GUAC_TERMINAL_MARGINS / GUAC_TERMINAL_MM_PER_INCH;
}

guac_terminal_display* guac_terminal_display_alloc(guac_client* client,
        guac_display* graphical_display,
        const char* font_name, int font_size, int dpi,
        guac_terminal_color* foreground, guac_terminal_color* background,
        guac_terminal_color (*palette)[256]) {

    /* Allocate display */
    guac_terminal_display* display = guac_mem_alloc(sizeof(guac_terminal_display));
    display->client = client;

    /* Initially no font loaded */
    display->font_desc = NULL;
    display->char_width = 0;
    display->char_height = 0;

    /* Create default surface */
    display->graphical_display = graphical_display;
    display->display_layer = guac_display_alloc_layer(display->graphical_display, 1);

    /* Use blank (invisible) cursor by default */
    display->current_cursor = display->last_requested_cursor = GUAC_TERMINAL_CURSOR_BLANK;
    guac_display_set_cursor(display->graphical_display, GUAC_DISPLAY_CURSOR_NONE);

    /* Never use lossy compression for terminal contents */
    guac_display_layer_set_lossless(display->display_layer, 1);

    /* Calculate margin size by DPI */
    display->margin = get_margin_by_dpi(dpi);

    /* Offset the Default Layer to make margins even on all sides */
    guac_display_layer_move(display->display_layer, display->margin, display->margin);

    display->default_foreground = *foreground;
    display->default_background = *background;
    display->default_palette = palette;

    /* Initially empty */
    display->width = 0;
    display->height = 0;

    /* Attempt to load font */
    if (guac_terminal_display_set_font(display, font_name, font_size, dpi)) {
        guac_client_abort(display->client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "Unable to set initial font \"%s\"", font_name);
        guac_mem_free(display);
        return NULL;
    }

    return display;

}

void guac_terminal_display_free(guac_terminal_display* display) {

    /* Free text rendering surface */
    guac_display_free_layer(display->display_layer);

    /* Free font description */
    pango_font_description_free(display->font_desc);

    /* Free default palette. */
    guac_mem_free(display->default_palette);

    /* Free display */
    guac_mem_free(display);

}

void guac_terminal_display_reset_palette(guac_terminal_display* display) {

    /* Reinitialize palette with default values */
    if (display->default_palette) {
        memcpy(display->palette, *display->default_palette,
               sizeof(GUAC_TERMINAL_INITIAL_PALETTE));
        return;
    }

    memcpy(display->palette, GUAC_TERMINAL_INITIAL_PALETTE,
            sizeof(GUAC_TERMINAL_INITIAL_PALETTE));

}

int guac_terminal_display_assign_color(guac_terminal_display* display,
        int index, const guac_terminal_color* color) {

    /* Assignment fails if out-of-bounds */
    if (index < 0 || index > 255)
        return 1;

    /* Copy color components */
    display->palette[index].red   = color->red;
    display->palette[index].green = color->green;
    display->palette[index].blue  = color->blue;

    /* Color successfully stored */
    return 0;

}

int guac_terminal_display_lookup_color(guac_terminal_display* display,
        int index, guac_terminal_color* color) {

    /* Use default foreground if foreground pseudo-index is given */
    if (index == GUAC_TERMINAL_COLOR_FOREGROUND) {
        *color = display->default_foreground;
        return 0;
    }

    /* Use default background if background pseudo-index is given */
    if (index == GUAC_TERMINAL_COLOR_BACKGROUND) {
        *color = display->default_background;
        return 0;
    }

    /* Lookup fails if out-of-bounds */
    if (index < 0 || index > 255)
        return 1;

    /* Copy color definition */
    *color = display->palette[index];
    return 0;

}

void guac_terminal_display_resize(guac_terminal_display* display, int width, int height) {

    /* Resize display only if dimensions have changed */
    if (width == display->width && height == display->height)
        return;

    /* Set width and height */
    display->width = width;
    display->height = height;

}

void guac_terminal_display_set_cursor(guac_terminal_display* display,
        guac_terminal_cursor_type cursor) {
    display->last_requested_cursor = cursor;
}

void guac_terminal_display_render_buffer(guac_terminal_display* display,
        guac_terminal_buffer* buffer, int scroll_offset,
        guac_terminal_char* default_char,
        bool cursor_visible, int cursor_row, int cursor_col,
        bool text_selected, int selection_start_row, int selection_start_col,
        int selection_end_row, int selection_end_col) {

    if (selection_start_row > selection_end_row) {

        int old_end_row = selection_end_row;
        selection_end_row = selection_start_row;
        selection_start_row = old_end_row;

        int old_end_col = selection_end_col;
        selection_end_col = selection_start_col;
        selection_start_col = old_end_col;

    }
    else if (selection_start_row == selection_end_row && selection_start_col > selection_end_col) {
        int old_end_col = selection_end_col;
        selection_end_col = selection_start_col;
        selection_start_col = old_end_col;
    }

    if (display->current_cursor != display->last_requested_cursor) {

        switch (display->last_requested_cursor) {

            case GUAC_TERMINAL_CURSOR_BLANK:
                guac_display_set_cursor(display->graphical_display, GUAC_DISPLAY_CURSOR_NONE);
                break;

            case GUAC_TERMINAL_CURSOR_IBAR:
                guac_display_set_cursor(display->graphical_display, GUAC_DISPLAY_CURSOR_IBAR);
                break;

            case GUAC_TERMINAL_CURSOR_POINTER:
                guac_display_set_cursor(display->graphical_display, GUAC_DISPLAY_CURSOR_POINTER);
                break;

        }

        display->current_cursor = display->last_requested_cursor;

    }

    guac_display_layer_resize(display->display_layer,
            display->char_width  * display->width,
            display->char_height * display->height);

    /* Redraw region */
    for (int row = 0; row < display->height; row++) {

        int adjusted_row = row - scroll_offset;

        guac_terminal_char* characters;
        unsigned int length = guac_terminal_buffer_get_columns(buffer, &characters, NULL, adjusted_row);

        /* Copy characters */
        for (int col = 0; col < display->width; col++) {

            bool is_cursor = cursor_visible
                && adjusted_row == cursor_row
                && col == cursor_col;

            bool is_selected = text_selected
                && adjusted_row >= selection_start_row
                && adjusted_row <= selection_end_row
                && (col >= selection_start_col || adjusted_row != selection_start_row)
                && (col <= selection_end_col   || adjusted_row != selection_end_row);

            if (col < length)
                guac_terminal_display_render_glyph(display, row, col,
                        &characters[col], is_cursor, is_selected);
            else
                guac_terminal_display_render_glyph(display, row, col,
                        default_char, is_cursor, is_selected);

        }

    }

}

int guac_terminal_display_set_font(guac_terminal_display* display,
        const char* font_name, int font_size, int dpi) {

    PangoFontDescription* font_desc;

    /* Build off existing font description if possible */
    if (display->font_desc != NULL)
        font_desc = pango_font_description_copy(display->font_desc);

    /* Create new font description if there is nothing to copy */
    else {
        font_desc = pango_font_description_new();
        pango_font_description_set_weight(font_desc, PANGO_WEIGHT_NORMAL);
    }

    /* Optionally update font name */
    if (font_name != NULL)
        pango_font_description_set_family(font_desc, font_name);

    /* Optionally update size */
    if (font_size != -1) {
        pango_font_description_set_size(font_desc,
                font_size * PANGO_SCALE * dpi / 96);
    }

    PangoFontMap* font_map = pango_cairo_font_map_get_default();
    PangoContext* context = pango_font_map_create_context(font_map);

    /* Load font from font map */
    PangoFont* font = pango_font_map_load_font(font_map, context, font_desc);
    if (font == NULL) {
        guac_client_log(display->client, GUAC_LOG_INFO, "Unable to load "
                "font \"%s\"", pango_font_description_get_family(font_desc));
        pango_font_description_free(font_desc);
        return 1;
    }

    /* Get metrics from loaded font */
    PangoFontMetrics* metrics = pango_font_get_metrics(font, NULL);
    if (metrics == NULL) {
        guac_client_log(display->client, GUAC_LOG_INFO, "Unable to get font "
                "metrics for font \"%s\"",
                pango_font_description_get_family(font_desc));
        pango_font_description_free(font_desc);
        return 1;
    }

    /* Save effective size of current display */
    int pixel_width = display->width * display->char_width;
    int pixel_height = display->height * display->char_height;

    /* Calculate character dimensions using metrics */
    display->char_width =
        pango_font_metrics_get_approximate_digit_width(metrics) / PANGO_SCALE;
    display->char_height =
        (pango_font_metrics_get_descent(metrics)
            + pango_font_metrics_get_ascent(metrics)) / PANGO_SCALE;

    /* Atomically replace old font description */
    PangoFontDescription* old_font_desc = display->font_desc;
    display->font_desc = font_desc;
    pango_font_description_free(old_font_desc);

    /* Recalculate dimensions which will fit within current surface */
    int new_width = pixel_width / display->char_width;
    int new_height = pixel_height / display->char_height;

    /* Resize display if dimensions have changed */
    if (new_width != display->width || new_height != display->height)
        guac_terminal_display_resize(display, new_width, new_height);


    return 0;

}
