
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

#include "types.h"
#include "delta.h"

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
 * Returns the location of the given character in the glyph cache layer,
 * sending it first if necessary. The location returned is in characters,
 * and thus must be multiplied by the glyph width to obtain the actual
 * location within the glyph cache layer.
 */
int __guac_terminal_get_glyph(guac_terminal_delta* delta, char c) {

    guac_socket* socket = delta->client->socket;
    int location;
    
    /* Use foreground color */
    const guac_terminal_color* color =
        &guac_terminal_palette[delta->glyph_foreground];

    /* Use background color */
    const guac_terminal_color* background =
        &guac_terminal_palette[delta->glyph_background];

    cairo_surface_t* surface;
    cairo_t* cairo;
   
    PangoLayout* layout;

    /* Return glyph if exists */
    if (delta->glyphs[(int) c])
        return delta->glyphs[(int) c] - 1;

    location = delta->next_glyph++;

    /* Otherwise, draw glyph */
    surface = cairo_image_surface_create(
            CAIRO_FORMAT_ARGB32,
            delta->char_width, delta->char_height);
    cairo = cairo_create(surface);

    /* Get layout */
    layout = pango_cairo_create_layout(cairo);
    pango_layout_set_font_description(layout, delta->font_desc);
    pango_layout_set_text(layout, &c, 1);

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

    /* Send glyph and update filled flyphs */
    guac_protocol_send_png(socket, GUAC_COMP_OVER, delta->glyph_stroke, location * delta->char_width, 0, surface);

    guac_protocol_send_rect(socket, delta->filled_glyphs,
            location * delta->char_width, 0,
            delta->char_width, delta->char_height);

    guac_protocol_send_cfill(socket, GUAC_COMP_OVER, delta->filled_glyphs,
            background->red,
            background->green,
            background->blue,
            0xFF);

    guac_protocol_send_copy(socket, delta->glyph_stroke,
            location * delta->char_width, 0, delta->char_width, delta->char_height,
            GUAC_COMP_OVER, delta->filled_glyphs, location * delta->char_width, 0);

    delta->glyphs[(int) c] = location+1;

    cairo_surface_destroy(surface);

    /* Return glyph */
    return location;

}

/**
 * Sets the attributes of the glyph cache layer such that future copies from
 * this layer will display as expected.
 */
int __guac_terminal_set_colors(guac_terminal_delta* delta,
        guac_terminal_attributes* attributes) {

    guac_socket* socket = delta->client->socket;
    const guac_terminal_color* background_color;
    int background, foreground;

    /* Handle reverse video */
    if (attributes->reverse != attributes->selected) {
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
    if (foreground != delta->glyph_foreground) {

        /* Get color */
        const guac_terminal_color* color =
            &guac_terminal_palette[foreground];

        /* Colorize letter */
        guac_protocol_send_rect(socket, delta->glyph_stroke,
            0, 0,
            delta->char_width * delta->next_glyph, delta->char_height);

        guac_protocol_send_cfill(socket, GUAC_COMP_ATOP, delta->glyph_stroke,
            color->red,
            color->green,
            color->blue,
            255);

    }

    /* If any color change at all, update filled */
    if (foreground != delta->glyph_foreground
            || background != delta->glyph_background) {

        /* Set background */
        guac_protocol_send_rect(socket, delta->filled_glyphs,
            0, 0,
            delta->char_width * delta->next_glyph, delta->char_height);

        guac_protocol_send_cfill(socket, GUAC_COMP_OVER, delta->filled_glyphs,
            background_color->red,
            background_color->green,
            background_color->blue,
            255);

        /* Copy stroke */
        guac_protocol_send_copy(socket, delta->glyph_stroke,

            0, 0,
            delta->char_width * delta->next_glyph, delta->char_height,

            GUAC_COMP_OVER, delta->filled_glyphs,
            0, 0);

    }

    delta->glyph_foreground = foreground;
    delta->glyph_background = background;

    return 0;

}

/**
 * Sends the given character to the terminal at the given row and column,
 * rendering the charater immediately. This bypasses the guac_terminal_delta
 * mechanism and is intended for flushing of updates only.
 */
int __guac_terminal_set(guac_terminal_delta* delta, int row, int col, char c) {

    guac_socket* socket = delta->client->socket;
    int location = __guac_terminal_get_glyph(delta, c); 

    return guac_protocol_send_copy(socket,
        delta->filled_glyphs,
        location * delta->char_width, 0, delta->char_width, delta->char_height,
        GUAC_COMP_OVER, GUAC_DEFAULT_LAYER,
        delta->char_width * col,
        delta->char_height * row);

}


guac_terminal_delta* guac_terminal_delta_alloc(guac_client* client, int width, int height,
        int foreground, int background) {

    guac_terminal_operation* current;
    int x, y;

    PangoFontMap* font_map;
    PangoFont* font;
    PangoFontMetrics* metrics;
    PangoContext* context;

    /* Allocate delta */
    guac_terminal_delta* delta = malloc(sizeof(guac_terminal_delta));
    delta->client = client;

    memset(delta->glyphs, 0, sizeof(delta->glyphs));
    delta->glyph_stroke = guac_client_alloc_buffer(client);
    delta->filled_glyphs = guac_client_alloc_buffer(client);

    /* Get font */
    delta->font_desc = pango_font_description_new();
    pango_font_description_set_family(delta->font_desc, "monospace");
    pango_font_description_set_weight(delta->font_desc, PANGO_WEIGHT_NORMAL);
    pango_font_description_set_size(delta->font_desc, 12*PANGO_SCALE);

    font_map = pango_cairo_font_map_get_default();
    context = pango_font_map_create_context(font_map);

    font = pango_font_map_load_font(font_map, context, delta->font_desc);
    if (font == NULL) {
        guac_client_log_error(delta->client, "Unable to get font.");
        return NULL;
    }

    metrics = pango_font_get_metrics(font, NULL);
    if (metrics == NULL) {
        guac_client_log_error(delta->client, "Unable to get font metrics.");
        return NULL;
    }

    delta->glyph_foreground = foreground;
    delta->glyph_background = background;

    /* Calculate character dimensions */
    delta->char_width =
        pango_font_metrics_get_approximate_digit_width(metrics) / PANGO_SCALE;
    delta->char_height =
        (pango_font_metrics_get_descent(metrics)
            + pango_font_metrics_get_ascent(metrics)) / PANGO_SCALE;

    /* Set width and height */
    delta->width = width;
    delta->height = height;

    /* Alloc operations */
    delta->operations = malloc(width * height *
            sizeof(guac_terminal_operation));

    /* Init each operation buffer row */
    current = delta->operations;
    for (y=0; y<height; y++) {

        /* Init entire row to NOP */
        for (x=0; x<width; x++)
            (current++)->type = GUAC_CHAR_NOP;

    }

    /* Alloc scratch area */
    delta->scratch = malloc(width * height * sizeof(guac_terminal_operation));

    /* Send initial display size */
    guac_protocol_send_size(client->socket,
            GUAC_DEFAULT_LAYER,
            delta->char_width  * width,
            delta->char_height * height);

    return delta;

}

void guac_terminal_delta_free(guac_terminal_delta* delta) {

    /* Free operations buffers */
    free(delta->operations);
    free(delta->scratch);

    /* Free delta */
    free(delta);

}

void guac_terminal_delta_copy_columns(guac_terminal_delta* delta, int row,
        int start_column, int end_column, int offset) {
    /* STUB */
}

void guac_terminal_delta_copy_rows(guac_terminal_delta* delta, int src_row, int rows,
        int start_row, int end_row, int offset) {
    /* STUB */
}

void guac_terminal_delta_set_columns(guac_terminal_delta* delta, int row,
        int start_column, int end_column, guac_terminal_char* character) {
    /* STUB */
}

void guac_terminal_delta_resize(guac_terminal_delta* delta, int rows, int cols) {
    /* STUB */
}

void __guac_terminal_delta_flush_copy(guac_terminal_delta* delta) {

    guac_terminal_operation* current = delta->operations;
    int row, col;

    /* For each operation */
    for (row=0; row<delta->height; row++) {
        for (col=0; col<delta->width; col++) {

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
                for (rect_row=row; rect_row<delta->height; rect_row++) {

                    guac_terminal_operation* rect_current = rect_current_row;
                    expected_col = current->column;

                    /* Find width */
                    for (rect_col=col; rect_col<delta->width; rect_col++) {

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
                    rect_current_row += delta->width;
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
                    rect_current_row += delta->width;
                    expected_row++;

                }

                /* Send copy */
                guac_protocol_send_copy(delta->client->socket,

                        GUAC_DEFAULT_LAYER,
                        current->column * delta->char_width,
                        current->row * delta->char_height,
                        rect_width * delta->char_width,
                        rect_height * delta->char_height,

                        GUAC_COMP_OVER,
                        GUAC_DEFAULT_LAYER,
                        col * delta->char_width,
                        row * delta->char_height);

            } /* end if copy operation */

            /* Next operation */
            current++;

        }
    }

}

void __guac_terminal_delta_flush_clear(guac_terminal_delta* delta) {

    guac_terminal_operation* current = delta->operations;
    int row, col;

    /* For each operation */
    for (row=0; row<delta->height; row++) {
        for (col=0; col<delta->width; col++) {

            /* If operation is a cler operation (set to space) */
            if (current->type == GUAC_CHAR_SET &&
                    current->character.value == ' ') {

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
                if (current->character.attributes.reverse != current->character.attributes.selected)
                   color = current->character.attributes.foreground;
                else
                   color = current->character.attributes.background;

                const guac_terminal_color* guac_color =
                    &guac_terminal_palette[color];

                /* Current row within a subrect */
                guac_terminal_operation* rect_current_row;

                /* Determine bounds of rectangle */
                rect_current_row = current;
                for (rect_row=row; rect_row<delta->height; rect_row++) {

                    guac_terminal_operation* rect_current = rect_current_row;

                    /* Find width */
                    for (rect_col=col; rect_col<delta->width; rect_col++) {

                        int joining_color;
                        if (rect_current->character.attributes.reverse)
                           joining_color = current->character.attributes.foreground;
                        else
                           joining_color = current->character.attributes.background;

                        /* If not identical operation, stop */
                        if (rect_current->type != GUAC_CHAR_SET
                                || rect_current->character.value != ' '
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
                    rect_current_row += delta->width;

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
                        if (rect_current->character.attributes.reverse)
                           joining_color = current->character.attributes.foreground;
                        else
                           joining_color = current->character.attributes.background;

                        /* Mark clear operations as NOP */
                        if (rect_current->type == GUAC_CHAR_SET
                                && rect_current->character.value == ' '
                                && joining_color == color)
                            rect_current->type = GUAC_CHAR_NOP;

                        /* Next column */
                        rect_current++;

                    }

                    /* Next row */
                    rect_current_row += delta->width;

                }

                /* Send rect */
                guac_protocol_send_rect(delta->client->socket,
                        GUAC_DEFAULT_LAYER,
                        col * delta->char_width,
                        row * delta->char_height,
                        rect_width * delta->char_width,
                        rect_height * delta->char_height);

                guac_protocol_send_cfill(delta->client->socket,
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

void __guac_terminal_delta_flush_set(guac_terminal_delta* delta) {

    guac_terminal_operation* current = delta->operations;
    int row, col;

    /* For each operation */
    for (row=0; row<delta->height; row++) {
        for (col=0; col<delta->width; col++) {

            /* Perform given operation */
            if (current->type == GUAC_CHAR_SET) {

                /* Set attributes */
                __guac_terminal_set_colors(delta,
                        &(current->character.attributes));

                /* Send character */
                __guac_terminal_set(delta, row, col,
                        current->character.value);

                /* Mark operation as handled */
                current->type = GUAC_CHAR_NOP;

            }

            /* Next operation */
            current++;

        }
    }

}

void guac_terminal_delta_flush(guac_terminal_delta* delta) {

    /* Flush operations, copies first, then clears, then sets. */
    __guac_terminal_delta_flush_copy(delta);
    __guac_terminal_delta_flush_clear(delta);
    __guac_terminal_delta_flush_set(delta);

}

