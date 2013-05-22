
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

#include <stdlib.h>
#include <string.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>

#include "common.h"
#include "types.h"
#include "display.h"

const guac_terminal_color guac_terminal_palette[16] = {

    /* Normal colors */
    {0x00, 0x00, 0x00}, /* Black   */
    {0x99, 0x3E, 0x3E}, /* Red     */
    {0x3E, 0x99, 0x3E}, /* Green   */
    {0x99, 0x99, 0x3E}, /* Brown   */
    {0x3E, 0x3E, 0x99}, /* Blue    */
    {0x99, 0x3E, 0x99}, /* Magenta */
    {0x3E, 0x99, 0x99}, /* Cyan    */
    {0x99, 0x99, 0x99}, /* White   */

    /* Intense colors */
    {0x3E, 0x3E, 0x3E}, /* Black   */
    {0xFF, 0x67, 0x67}, /* Red     */
    {0x67, 0xFF, 0x67}, /* Green   */
    {0xFF, 0xFF, 0x67}, /* Brown   */
    {0x67, 0x67, 0xFF}, /* Blue    */
    {0xFF, 0x67, 0xFF}, /* Magenta */
    {0x67, 0xFF, 0xFF}, /* Cyan    */
    {0xFF, 0xFF, 0xFF}, /* White   */

};

/**
 * Clears the currently-selected region, removing the highlight.
 */
static void __guac_terminal_display_clear_select(guac_terminal_display* display) {

    guac_socket* socket = display->client->socket;
    guac_layer* select_layer = display->select_layer;

    guac_protocol_send_rect(socket, select_layer, 0, 0, 1, 1);
    guac_protocol_send_cfill(socket, GUAC_COMP_SRC, select_layer,
            0x00, 0x00, 0x00, 0x00);

    guac_socket_flush(socket);

    /* Text is no longer selected */
    display->text_selected =
    display->selection_committed = false;

}

/**
 * Returns whether at least one character within the given range is selected.
 */
static bool __guac_terminal_display_selected_contains(guac_terminal_display* display,
        int start_row, int start_column, int end_row, int end_column) {

    /* If test range starts after highlight ends, does not intersect */
    if (start_row > display->selection_end_row)
        return false;

    if (start_row == display->selection_end_row
            && start_column > display->selection_end_column)
        return false;

    /* If test range ends before highlight starts, does not intersect */
    if (end_row < display->selection_start_row)
        return false;

    if (end_row == display->selection_start_row
            && end_column < display->selection_start_column)
        return false;

    /* Otherwise, does intersect */
    return true;

}

/* Maps any codepoint onto a number between 0 and 511 inclusive */
int __guac_terminal_hash_codepoint(int codepoint) {

    /* If within one byte, just return codepoint */
    if (codepoint <= 0xFF)
        return codepoint;

    /* Otherwise, map to next 256 values */
    return (codepoint & 0xFF) + 0x100;

}

/**
 * Returns the location of the given character in the glyph cache layer,
 * sending it first if necessary. The location returned is in characters,
 * and thus must be multiplied by the glyph width to obtain the actual
 * location within the glyph cache layer.
 */
int __guac_terminal_get_glyph(guac_terminal_display* display, int codepoint) {

    guac_socket* socket = display->client->socket;
    int location;

    int bytes;
    char utf8[4];

    /* Use foreground color */
    const guac_terminal_color* color =
        &guac_terminal_palette[display->glyph_foreground];

    /* Use background color */
    const guac_terminal_color* background =
        &guac_terminal_palette[display->glyph_background];

    cairo_surface_t* surface;
    cairo_t* cairo;
   
    PangoLayout* layout;

    /* Get codepoint hash */
    int hashcode = __guac_terminal_hash_codepoint(codepoint);

    /* If something already stored here, either same codepoint or collision */
    if (display->glyphs[hashcode].location) {

        location = display->glyphs[hashcode].location - 1;

        /* If match, return match. */
        if (display->glyphs[hashcode].codepoint == codepoint)
            return location;

        /* Otherwise, reuse location */

    }

    /* If no collision, allocate new glyph location */
    else
        location = display->next_glyph++;

    /* Convert to UTF-8 */
    bytes = guac_terminal_encode_utf8(codepoint, utf8);

    /* Prepare surface */
    surface = cairo_image_surface_create(
            CAIRO_FORMAT_ARGB32,
            display->char_width, display->char_height);
    cairo = cairo_create(surface);

    /* Get layout */
    layout = pango_cairo_create_layout(cairo);
    pango_layout_set_font_description(layout, display->font_desc);
    pango_layout_set_text(layout, utf8, bytes);

    /* Draw */
    cairo_set_source_rgba(cairo,
            color->red   / 255.0,
            color->green / 255.0,
            color->blue  / 255.0,
            1.0 /* alpha */ );

    cairo_move_to(cairo, 0.0, 0.0);
    pango_cairo_show_layout(cairo, layout);

    /* Free all */
    g_object_unref(layout);
    cairo_destroy(cairo);

    /* Clear existing glyph (if any) */
    guac_protocol_send_rect(socket, display->glyph_stroke,
            location * display->char_width, 0,
            display->char_width, display->char_height);

    guac_protocol_send_cfill(socket, GUAC_COMP_ROUT, display->glyph_stroke,
            0x00, 0x00, 0x00, 0xFF);

    /* Send glyph */
    guac_protocol_send_png(socket, GUAC_COMP_OVER, display->glyph_stroke, location * display->char_width, 0, surface);

    /* Update filled glyphs */
    guac_protocol_send_rect(socket, display->filled_glyphs,
            location * display->char_width, 0,
            display->char_width, display->char_height);

    guac_protocol_send_cfill(socket, GUAC_COMP_OVER, display->filled_glyphs,
            background->red,
            background->green,
            background->blue,
            0xFF);

    guac_protocol_send_copy(socket, display->glyph_stroke,
            location * display->char_width, 0, display->char_width, display->char_height,
            GUAC_COMP_OVER, display->filled_glyphs, location * display->char_width, 0);

    display->glyphs[hashcode].location = location+1;
    display->glyphs[hashcode].codepoint = codepoint;

    cairo_surface_destroy(surface);

    /* Return glyph */
    return location;

}

/**
 * Sets the attributes of the glyph cache layer such that future copies from
 * this layer will display as expected.
 */
int __guac_terminal_set_colors(guac_terminal_display* display,
        guac_terminal_attributes* attributes) {

    guac_socket* socket = display->client->socket;
    const guac_terminal_color* background_color;
    int background, foreground;

    /* Handle reverse video */
    if (attributes->reverse != attributes->cursor) {
        background = attributes->foreground;
        foreground = attributes->background;
    }
    else {
        foreground = attributes->foreground;
        background = attributes->background;
    }

    /* Handle bold */
    if (attributes->bold && foreground <= 7)
        foreground += 8;

    /* Get background color */
    background_color = &guac_terminal_palette[background];

    /* If foreground different from current, colorize */
    if (foreground != display->glyph_foreground) {

        /* Get color */
        const guac_terminal_color* color =
            &guac_terminal_palette[foreground];

        /* Colorize letter */
        guac_protocol_send_rect(socket, display->glyph_stroke,
            0, 0,
            display->char_width * display->next_glyph, display->char_height);

        guac_protocol_send_cfill(socket, GUAC_COMP_ATOP, display->glyph_stroke,
            color->red,
            color->green,
            color->blue,
            255);

    }

    /* If any color change at all, update filled */
    if (foreground != display->glyph_foreground
            || background != display->glyph_background) {

        /* Set background */
        guac_protocol_send_rect(socket, display->filled_glyphs,
            0, 0,
            display->char_width * display->next_glyph, display->char_height);

        guac_protocol_send_cfill(socket, GUAC_COMP_OVER, display->filled_glyphs,
            background_color->red,
            background_color->green,
            background_color->blue,
            255);

        /* Copy stroke */
        guac_protocol_send_copy(socket, display->glyph_stroke,

            0, 0,
            display->char_width * display->next_glyph, display->char_height,

            GUAC_COMP_OVER, display->filled_glyphs,
            0, 0);

    }

    display->glyph_foreground = foreground;
    display->glyph_background = background;

    return 0;

}

/**
 * Sends the given character to the terminal at the given row and column,
 * rendering the charater immediately. This bypasses the guac_terminal_display
 * mechanism and is intended for flushing of updates only.
 */
int __guac_terminal_set(guac_terminal_display* display, int row, int col, int codepoint) {

    guac_socket* socket = display->client->socket;
    int location = __guac_terminal_get_glyph(display, codepoint); 

    return guac_protocol_send_copy(socket,
        display->filled_glyphs,
        location * display->char_width, 0, display->char_width, display->char_height,
        GUAC_COMP_OVER, GUAC_DEFAULT_LAYER,
        display->char_width * col,
        display->char_height * row);

}


guac_terminal_display* guac_terminal_display_alloc(guac_client* client, int foreground, int background) {

    PangoFontMap* font_map;
    PangoFont* font;
    PangoFontMetrics* metrics;
    PangoContext* context;

    /* Allocate display */
    guac_terminal_display* display = malloc(sizeof(guac_terminal_display));
    display->client = client;

    memset(display->glyphs, 0, sizeof(display->glyphs));
    display->glyph_stroke = guac_client_alloc_buffer(client);
    display->filled_glyphs = guac_client_alloc_buffer(client);

    display->select_layer = guac_client_alloc_layer(client);

    /* Get font */
    display->font_desc = pango_font_description_new();
    pango_font_description_set_family(display->font_desc, "monospace");
    pango_font_description_set_weight(display->font_desc, PANGO_WEIGHT_NORMAL);
    pango_font_description_set_size(display->font_desc, 12*PANGO_SCALE);

    font_map = pango_cairo_font_map_get_default();
    context = pango_font_map_create_context(font_map);

    font = pango_font_map_load_font(font_map, context, display->font_desc);
    if (font == NULL) {
        guac_client_log_error(display->client, "Unable to get font.");
        return NULL;
    }

    metrics = pango_font_get_metrics(font, NULL);
    if (metrics == NULL) {
        guac_client_log_error(display->client, "Unable to get font metrics.");
        return NULL;
    }

    display->glyph_foreground = foreground;
    display->glyph_background = background;

    /* Calculate character dimensions */
    display->char_width =
        pango_font_metrics_get_approximate_digit_width(metrics) / PANGO_SCALE;
    display->char_height =
        (pango_font_metrics_get_descent(metrics)
            + pango_font_metrics_get_ascent(metrics)) / PANGO_SCALE;

    /* Initially empty */
    display->width = 0;
    display->height = 0;
    display->operations = NULL;

    /* Initially nothing selected */
    display->text_selected =
    display->selection_committed = false;

    return display;

}

void guac_terminal_display_free(guac_terminal_display* display) {

    /* Free operations buffers */
    free(display->operations);

    /* Free display */
    free(display);

}

void guac_terminal_display_copy_columns(guac_terminal_display* display, int row,
        int start_column, int end_column, int offset) {

    int i;
    guac_terminal_operation* src_current;
    guac_terminal_operation* current;

    /* Ignore operations outside display bounds */
    if (row < 0 || row >= display->height)
        return;

    /* Fit range within bounds */
    start_column = guac_terminal_fit_to_range(start_column,          0, display->width - 1);
    end_column   = guac_terminal_fit_to_range(end_column,            0, display->width - 1);
    start_column = guac_terminal_fit_to_range(start_column + offset, 0, display->width - 1) - offset;
    end_column   = guac_terminal_fit_to_range(end_column   + offset, 0, display->width - 1) - offset;

    src_current = &(display->operations[row * display->width + start_column]);
    current = &(display->operations[row * display->width + start_column + offset]);

    /* Move data */
    memmove(current, src_current,
        (end_column - start_column + 1) * sizeof(guac_terminal_operation));

    /* Update operations */
    for (i=start_column; i<=end_column; i++) {

        /* If no operation here, set as copy */
        if (current->type == GUAC_CHAR_NOP) {
            current->type = GUAC_CHAR_COPY;
            current->row = row;
            current->column = i;
        }

        /* Next column */
        current++;

    }

    /* If selection visible and committed, clear if update touches selection */
    if (display->text_selected && display->selection_committed &&
        __guac_terminal_display_selected_contains(display, row, start_column, row, end_column))
            __guac_terminal_display_clear_select(display);

}

void guac_terminal_display_copy_rows(guac_terminal_display* display,
        int start_row, int end_row, int offset) {

    int row, col;
    guac_terminal_operation* src_current_row;
    guac_terminal_operation* current_row;

    /* Fit range within bounds */
    start_row = guac_terminal_fit_to_range(start_row,          0, display->height - 1);
    end_row   = guac_terminal_fit_to_range(end_row,            0, display->height - 1);
    start_row = guac_terminal_fit_to_range(start_row + offset, 0, display->height - 1) - offset;
    end_row   = guac_terminal_fit_to_range(end_row   + offset, 0, display->height - 1) - offset;

    src_current_row = &(display->operations[start_row * display->width]);
    current_row = &(display->operations[(start_row + offset) * display->width]);

    /* Move data */
    memmove(current_row, src_current_row,
        (end_row - start_row + 1) * sizeof(guac_terminal_operation) * display->width);

    /* Update operations */
    for (row=start_row; row<=end_row; row++) {

        guac_terminal_operation* current = current_row;
        for (col=0; col<display->width; col++) {

            /* If no operation here, set as copy */
            if (current->type == GUAC_CHAR_NOP) {
                current->type = GUAC_CHAR_COPY;
                current->row = row;
                current->column = col;
            }

            /* Next column */
            current++;

        }

        /* Next row */
        current_row += display->width;

    }

    /* If selection visible and committed, clear if update touches selection */
    if (display->text_selected && display->selection_committed &&
        __guac_terminal_display_selected_contains(display, start_row, 0, end_row, display->width - 1))
            __guac_terminal_display_clear_select(display);

}

void guac_terminal_display_set_columns(guac_terminal_display* display, int row,
        int start_column, int end_column, guac_terminal_char* character) {

    int i;
    guac_terminal_operation* current;

    /* Ignore operations outside display bounds */
    if (row < 0 || row >= display->height)
        return;

    /* Fit range within bounds */
    start_column = guac_terminal_fit_to_range(start_column, 0, display->width - 1);
    end_column   = guac_terminal_fit_to_range(end_column,   0, display->width - 1);

    current = &(display->operations[row * display->width + start_column]);

    /* For each column in range */
    for (i=start_column; i<=end_column; i++) {

        /* Set operation */
        current->type      = GUAC_CHAR_SET;
        current->character = *character;

        /* Next column */
        current++;
    }

    /* If selection visible and committed, clear if update touches selection */
    if (display->text_selected && display->selection_committed &&
        __guac_terminal_display_selected_contains(display, row, start_column, row, end_column))
            __guac_terminal_display_clear_select(display);

}

void guac_terminal_display_resize(guac_terminal_display* display, int width, int height) {

    guac_terminal_operation* current;
    int x, y;

    /* Fill with background color (index 0) */
    guac_terminal_char fill = {
        .value = 0,
        .attributes = {
            .foreground = 0,
            .background = 0
        }
    };

    /* Free old operations buffer */
    if (display->operations != NULL)
        free(display->operations);

    /* Alloc operations */
    display->operations = malloc(width * height *
            sizeof(guac_terminal_operation));

    /* Init each operation buffer row */
    current = display->operations;
    for (y=0; y<height; y++) {

        /* Init entire row to NOP */
        for (x=0; x<width; x++) {

            /* If on old part of screen, do not clear */
            if (x < display->width && y < display->height)
                current->type = GUAC_CHAR_NOP;

            /* Otherwise, clear contents first */
            else {
                current->type = GUAC_CHAR_SET;
                current->character  = fill;
            }

            current++;

        }

    }

    /* Set width and height */
    display->width = width;
    display->height = height;

    /* Send initial display size */
    guac_protocol_send_size(display->client->socket,
            GUAC_DEFAULT_LAYER,
            display->char_width  * width,
            display->char_height * height);

    guac_protocol_send_size(display->client->socket,
            display->select_layer,
            display->char_width  * width,
            display->char_height * height);

    /* If selection visible and committed, clear */
    if (display->text_selected && display->selection_committed)
        __guac_terminal_display_clear_select(display);

}

void __guac_terminal_display_flush_copy(guac_terminal_display* display) {

    guac_terminal_operation* current = display->operations;
    int row, col;

    /* For each operation */
    for (row=0; row<display->height; row++) {
        for (col=0; col<display->width; col++) {

            /* If operation is a copy operation */
            if (current->type == GUAC_CHAR_COPY) {

                /* The determined bounds of the rectangle of contiguous
                 * operations */
                int detected_right = -1;
                int detected_bottom = row;

                /* The current row or column within a rectangle */
                int rect_row, rect_col;

                /* The dimensions of the rectangle as determined */
                int rect_width, rect_height;

                /* The expected row and column source for the next copy
                 * operation (if adjacent to current) */
                int expected_row, expected_col;

                /* Current row within a subrect */
                guac_terminal_operation* rect_current_row;

                /* Determine bounds of rectangle */
                rect_current_row = current;
                expected_row = current->row;
                for (rect_row=row; rect_row<display->height; rect_row++) {

                    guac_terminal_operation* rect_current = rect_current_row;
                    expected_col = current->column;

                    /* Find width */
                    for (rect_col=col; rect_col<display->width; rect_col++) {

                        /* If not identical operation, stop */
                        if (rect_current->type != GUAC_CHAR_COPY
                                || rect_current->row != expected_row
                                || rect_current->column != expected_col)
                            break;

                        /* Next column */
                        rect_current++;
                        expected_col++;

                    }

                    /* If too small, cannot append row */
                    if (rect_col-1 < detected_right)
                        break;

                    /* As row has been accepted, update rect_row of rect */
                    detected_bottom = rect_row;

                    /* For now, only set rect_col bound if uninitialized */
                    if (detected_right == -1)
                        detected_right = rect_col - 1;

                    /* Next row */
                    rect_current_row += display->width;
                    expected_row++;

                }

                /* Calculate dimensions */
                rect_width  = detected_right  - col + 1;
                rect_height = detected_bottom - row + 1;

                /* Mark rect as NOP (as it has been handled) */
                rect_current_row = current;
                expected_row = current->row;
                for (rect_row=0; rect_row<rect_height; rect_row++) {
                    
                    guac_terminal_operation* rect_current = rect_current_row;
                    expected_col = current->column;

                    for (rect_col=0; rect_col<rect_width; rect_col++) {

                        /* Mark copy operations as NOP */
                        if (rect_current->type == GUAC_CHAR_COPY
                                && rect_current->row == expected_row
                                && rect_current->column == expected_col)
                            rect_current->type = GUAC_CHAR_NOP;

                        /* Next column */
                        rect_current++;
                        expected_col++;

                    }

                    /* Next row */
                    rect_current_row += display->width;
                    expected_row++;

                }

                /* Send copy */
                guac_protocol_send_copy(display->client->socket,

                        GUAC_DEFAULT_LAYER,
                        current->column * display->char_width,
                        current->row * display->char_height,
                        rect_width * display->char_width,
                        rect_height * display->char_height,

                        GUAC_COMP_OVER,
                        GUAC_DEFAULT_LAYER,
                        col * display->char_width,
                        row * display->char_height);

            } /* end if copy operation */

            /* Next operation */
            current++;

        }
    }

}

void __guac_terminal_display_flush_clear(guac_terminal_display* display) {

    guac_terminal_operation* current = display->operations;
    int row, col;

    /* For each operation */
    for (row=0; row<display->height; row++) {
        for (col=0; col<display->width; col++) {

            /* If operation is a cler operation (set to space) */
            if (current->type == GUAC_CHAR_SET &&
                    !guac_terminal_has_glyph(current->character.value)) {

                /* The determined bounds of the rectangle of contiguous
                 * operations */
                int detected_right = -1;
                int detected_bottom = row;

                /* The current row or column within a rectangle */
                int rect_row, rect_col;

                /* The dimensions of the rectangle as determined */
                int rect_width, rect_height;

                /* Color of the rectangle to draw */
                int color;
                if (current->character.attributes.reverse != current->character.attributes.cursor)
                   color = current->character.attributes.foreground;
                else
                   color = current->character.attributes.background;

                const guac_terminal_color* guac_color =
                    &guac_terminal_palette[color];

                /* Current row within a subrect */
                guac_terminal_operation* rect_current_row;

                /* Determine bounds of rectangle */
                rect_current_row = current;
                for (rect_row=row; rect_row<display->height; rect_row++) {

                    guac_terminal_operation* rect_current = rect_current_row;

                    /* Find width */
                    for (rect_col=col; rect_col<display->width; rect_col++) {

                        int joining_color;
                        if (rect_current->character.attributes.reverse != rect_current->character.attributes.cursor)
                           joining_color = rect_current->character.attributes.foreground;
                        else
                           joining_color = rect_current->character.attributes.background;

                        /* If not identical operation, stop */
                        if (rect_current->type != GUAC_CHAR_SET
                                || guac_terminal_has_glyph(rect_current->character.value)
                                || joining_color != color)
                            break;

                        /* Next column */
                        rect_current++;

                    }

                    /* If too small, cannot append row */
                    if (rect_col-1 < detected_right)
                        break;

                    /* As row has been accepted, update rect_row of rect */
                    detected_bottom = rect_row;

                    /* For now, only set rect_col bound if uninitialized */
                    if (detected_right == -1)
                        detected_right = rect_col - 1;

                    /* Next row */
                    rect_current_row += display->width;

                }

                /* Calculate dimensions */
                rect_width  = detected_right  - col + 1;
                rect_height = detected_bottom - row + 1;

                /* Mark rect as NOP (as it has been handled) */
                rect_current_row = current;
                for (rect_row=0; rect_row<rect_height; rect_row++) {
                    
                    guac_terminal_operation* rect_current = rect_current_row;

                    for (rect_col=0; rect_col<rect_width; rect_col++) {

                        int joining_color;
                        if (rect_current->character.attributes.reverse != rect_current->character.attributes.cursor)
                           joining_color = rect_current->character.attributes.foreground;
                        else
                           joining_color = rect_current->character.attributes.background;

                        /* Mark clear operations as NOP */
                        if (rect_current->type == GUAC_CHAR_SET
                                && !guac_terminal_has_glyph(rect_current->character.value)
                                && joining_color == color)
                            rect_current->type = GUAC_CHAR_NOP;

                        /* Next column */
                        rect_current++;

                    }

                    /* Next row */
                    rect_current_row += display->width;

                }

                /* Send rect */
                guac_protocol_send_rect(display->client->socket,
                        GUAC_DEFAULT_LAYER,
                        col * display->char_width,
                        row * display->char_height,
                        rect_width * display->char_width,
                        rect_height * display->char_height);

                guac_protocol_send_cfill(display->client->socket,
                        GUAC_COMP_OVER,
                        GUAC_DEFAULT_LAYER,
                        guac_color->red, guac_color->green, guac_color->blue,
                        0xFF);

            } /* end if clear operation */

            /* Next operation */
            current++;

        }
    }

}

void __guac_terminal_display_flush_set(guac_terminal_display* display) {

    guac_terminal_operation* current = display->operations;
    int row, col;

    /* For each operation */
    for (row=0; row<display->height; row++) {
        for (col=0; col<display->width; col++) {

            /* Perform given operation */
            if (current->type == GUAC_CHAR_SET) {

                /* Set attributes */
                __guac_terminal_set_colors(display,
                        &(current->character.attributes));

                /* Send character */
                __guac_terminal_set(display, row, col,
                        current->character.value);

                /* Mark operation as handled */
                current->type = GUAC_CHAR_NOP;

            }

            /* Next operation */
            current++;

        }
    }

}

void guac_terminal_display_flush(guac_terminal_display* display) {

    /* Flush operations, copies first, then clears, then sets. */
    __guac_terminal_display_flush_copy(display);
    __guac_terminal_display_flush_clear(display);
    __guac_terminal_display_flush_set(display);

}

void guac_terminal_display_commit_select(guac_terminal_display* display) {
    display->selection_committed = true;
}

void guac_terminal_display_select(guac_terminal_display* display,
        int start_row, int start_col, int end_row, int end_col) {

    guac_socket* socket = display->client->socket;
    guac_layer* select_layer = display->select_layer;

    /* Text is now selected */
    display->text_selected = true;

    display->selection_start_row = start_row;
    display->selection_start_column = start_col;
    display->selection_end_row = end_row;
    display->selection_end_column = end_col;


    /* If single row, just need one rectangle */
    if (start_row == end_row) {

        /* Ensure proper ordering of columns */
        if (start_col > end_col) {
            int temp = start_col;
            start_col = end_col;
            end_col = temp;
        }

        /* Select characters between columns */
        guac_protocol_send_rect(socket, select_layer,

                start_col * display->char_width,
                start_row * display->char_height,

                (end_col - start_col + 1) * display->char_width,
                display->char_height);

    }

    /* Otherwise, need three */
    else {

        /* Ensure proper ordering of start and end coords */
        if (start_row > end_row) {

            int temp;

            temp = start_row;
            start_row = end_row;
            end_row = temp;

            temp = start_col;
            start_col = end_col;
            end_col = temp;

        }

        /* First row */
        guac_protocol_send_rect(socket, select_layer,

                start_col * display->char_width,
                start_row * display->char_height,

                display->width * display->char_width,
                display->char_height);

        /* Middle */
        guac_protocol_send_rect(socket, select_layer,

                0,
                (start_row + 1) * display->char_height,

                display->width * display->char_width,
                (end_row - start_row - 1) * display->char_height);

        /* Last row */
        guac_protocol_send_rect(socket, select_layer,

                0,
                end_row * display->char_height,

                (end_col + 1) * display->char_width,
                display->char_height);

    }

    /* Draw new selection, erasing old */
    guac_protocol_send_cfill(socket, GUAC_COMP_SRC, select_layer,
            0x00, 0x80, 0xFF, 0x60);

    guac_socket_flush(socket);

}

