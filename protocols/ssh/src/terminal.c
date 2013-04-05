
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
 * James Muehlner <dagger10k@users.sourceforge.net>
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

#include <cairo/cairo.h>
#include <pango/pangocairo.h>

#include <guacamole/socket.h>
#include <guacamole/protocol.h>
#include <guacamole/client.h>

#include "terminal.h"
#include "terminal_handlers.h"

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

guac_terminal* guac_terminal_create(guac_client* client,
        int width, int height) {

    guac_terminal_attributes default_attributes = {
        .foreground = 7,
        .background = 0,
        .bold       = false,
        .reverse    = false,
        .underscore = false
    };

    PangoFontMap* font_map;
    PangoFont* font;
    PangoFontMetrics* metrics;
    PangoContext* context;

    guac_terminal* term = malloc(sizeof(guac_terminal));
    term->client = client;

    term->current_attributes = 
    term->default_attributes = default_attributes;
    term->glyph_foreground = default_attributes.foreground;
    term->glyph_background = default_attributes.background;

    memset(term->glyphs, 0, sizeof(term->glyphs));
    term->glyph_stroke = guac_client_alloc_buffer(client);
    term->filled_glyphs = guac_client_alloc_buffer(client);

    /* Get font */
    term->font_desc = pango_font_description_new();
    pango_font_description_set_family(term->font_desc, "monospace");
    pango_font_description_set_weight(term->font_desc, PANGO_WEIGHT_NORMAL);
    pango_font_description_set_size(term->font_desc, 12*PANGO_SCALE);

    font_map = pango_cairo_font_map_get_default();
    context = pango_font_map_create_context(font_map);

    font = pango_font_map_load_font(font_map, context, term->font_desc);
    if (font == NULL) {
        guac_client_log_error(term->client, "Unable to get font.");
        return NULL;
    }

    metrics = pango_font_get_metrics(font, NULL);
    if (metrics == NULL) {
        guac_client_log_error(term->client, "Unable to get font metrics.");
        return NULL;
    }

    /* Calculate character dimensions */
    term->char_width =
        pango_font_metrics_get_approximate_digit_width(metrics) / PANGO_SCALE;
    term->char_height =
        (pango_font_metrics_get_descent(metrics)
            + pango_font_metrics_get_ascent(metrics)) / PANGO_SCALE;


    term->cursor_row = 0;
    term->cursor_col = 0;

    term->term_width = width / term->char_width;
    term->term_height = height / term->char_height;
    term->char_handler = guac_terminal_echo; 

    term->scroll_start = 0;
    term->scroll_end = term->term_height - 1;

    /* Init scrollback buffer */
    term->scrollback = guac_terminal_scrollback_buffer_alloc(1000);
    term->scroll_offset = 0;

    /* Init delta */
    term->delta = guac_terminal_delta_alloc(term->term_width,
            term->term_height);

    /* Init buffer */
    term->buffer = guac_terminal_buffer_alloc(term->term_width,
            term->term_height);

    /* Clear with background color */
    guac_terminal_clear(term,
            0, 0, term->term_height, term->term_width);

    return term;

}

void guac_terminal_free(guac_terminal* term) {
    
    /* Free scrollback buffer */
    guac_terminal_scrollback_buffer_free(term->scrollback);

    /* Free delta */
    guac_terminal_delta_free(term->delta);

    /* Free buffer */
    guac_terminal_buffer_free(term->buffer);

}

/**
 * Returns the location of the given character in the glyph cache layer,
 * sending it first if necessary. The location returned is in characters,
 * and thus must be multiplied by the glyph width to obtain the actual
 * location within the glyph cache layer.
 */
int __guac_terminal_get_glyph(guac_terminal* term, char c) {

    guac_socket* socket = term->client->socket;
    int location;
    
    /* Use foreground color */
    const guac_terminal_color* color =
        &guac_terminal_palette[term->glyph_foreground];

    /* Use background color */
    const guac_terminal_color* background =
        &guac_terminal_palette[term->glyph_background];

    cairo_surface_t* surface;
    cairo_t* cairo;
   
    PangoLayout* layout;

    /* Return glyph if exists */
    if (term->glyphs[(int) c])
        return term->glyphs[(int) c] - 1;

    location = term->next_glyph++;

    /* Otherwise, draw glyph */
    surface = cairo_image_surface_create(
            CAIRO_FORMAT_ARGB32,
            term->char_width, term->char_height);
    cairo = cairo_create(surface);

    /* Get layout */
    layout = pango_cairo_create_layout(cairo);
    pango_layout_set_font_description(layout, term->font_desc);
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
    guac_protocol_send_png(socket, GUAC_COMP_OVER, term->glyph_stroke, location * term->char_width, 0, surface);

    guac_protocol_send_rect(socket, term->filled_glyphs,
            location * term->char_width, 0,
            term->char_width, term->char_height);

    guac_protocol_send_cfill(socket, GUAC_COMP_OVER, term->filled_glyphs,
            background->red,
            background->green,
            background->blue,
            0xFF);

    guac_protocol_send_copy(socket, term->glyph_stroke,
            location * term->char_width, 0, term->char_width, term->char_height,
            GUAC_COMP_OVER, term->filled_glyphs, location * term->char_width, 0);

    term->glyphs[(int) c] = location+1;

    cairo_surface_destroy(surface);

    /* Return glyph */
    return location;

}

/**
 * Sets the attributes of the glyph cache layer such that future copies from
 * this layer will display as expected.
 */
int __guac_terminal_set_colors(guac_terminal* term,
        guac_terminal_attributes* attributes) {

    guac_socket* socket = term->client->socket;
    const guac_terminal_color* background_color;
    int background, foreground;

    /* Handle reverse video */
    if (attributes->reverse) {
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
    if (foreground != term->glyph_foreground) {

        /* Get color */
        const guac_terminal_color* color =
            &guac_terminal_palette[foreground];

        /* Colorize letter */
        guac_protocol_send_rect(socket, term->glyph_stroke,
            0, 0,
            term->char_width * term->next_glyph, term->char_height);

        guac_protocol_send_cfill(socket, GUAC_COMP_ATOP, term->glyph_stroke,
            color->red,
            color->green,
            color->blue,
            255);

    }

    /* If any color change at all, update filled */
    if (foreground != term->glyph_foreground
            || background != term->glyph_background) {

        /* Set background */
        guac_protocol_send_rect(socket, term->filled_glyphs,
            0, 0,
            term->char_width * term->next_glyph, term->char_height);

        guac_protocol_send_cfill(socket, GUAC_COMP_OVER, term->filled_glyphs,
            background_color->red,
            background_color->green,
            background_color->blue,
            255);

        /* Copy stroke */
        guac_protocol_send_copy(socket, term->glyph_stroke,

            0, 0,
            term->char_width * term->next_glyph, term->char_height,

            GUAC_COMP_OVER, term->filled_glyphs,
            0, 0);

    }

    term->glyph_foreground = foreground;
    term->glyph_background = background;

    return 0;

}

/**
 * Sends the given character to the terminal at the given row and column,
 * rendering the charater immediately. This bypasses the guac_terminal_delta
 * mechanism and is intended for flushing of updates only.
 */
int __guac_terminal_set(guac_terminal* term, int row, int col, char c) {

    guac_socket* socket = term->client->socket;
    int location = __guac_terminal_get_glyph(term, c); 

    return guac_protocol_send_copy(socket,
        term->filled_glyphs,
        location * term->char_width, 0, term->char_width, term->char_height,
        GUAC_COMP_OVER, GUAC_DEFAULT_LAYER,
        term->char_width * col,
        term->char_height * row);

}

int guac_terminal_set(guac_terminal* term, int row, int col, char c) {

    int scrolled_row = row + term->scroll_offset;

    /* Build character with current attributes */
    guac_terminal_char guac_char;
    guac_char.value = c;
    guac_char.attributes = term->current_attributes;

    /* Set delta */
    if (scrolled_row < term->delta->height)
        guac_terminal_delta_set(term->delta, scrolled_row, col, &guac_char);

    /* Set buffer */
    guac_terminal_buffer_set(term->buffer, row, col, &guac_char);

    return 0;

}

int guac_terminal_toggle_reverse(guac_terminal* term, int row, int col) {

    int scrolled_row = row + term->scroll_offset;

    /* Get character from buffer */
    guac_terminal_char* guac_char =
        &(term->buffer->characters[row*term->buffer->width + col]);

    /* Toggle reverse */
    guac_char->attributes.reverse = !(guac_char->attributes.reverse);

    /* Set delta */
    if (scrolled_row < term->delta->height)
        guac_terminal_delta_set(term->delta, scrolled_row, col, guac_char);

    return 0;

}

int guac_terminal_write(guac_terminal* term, const char* c, int size) {

    while (size > 0) {
        term->char_handler(term, *(c++));
        size--;
    }

    return 0;

}

int guac_terminal_copy(guac_terminal* term,
        int src_row, int src_col, int rows, int cols,
        int dst_row, int dst_col) {

    int scrolled_src_row = src_row + term->scroll_offset;
    int scrolled_dst_row = dst_row + term->scroll_offset;

    int scrolled_rows = rows;

    /* Adjust delta rect height if scrolled out of view */
    if (scrolled_src_row + scrolled_rows > term->delta->height)
        scrolled_rows = term->delta->height - scrolled_src_row;

    if (scrolled_dst_row + scrolled_rows > term->delta->height)
        scrolled_rows = term->delta->height - scrolled_dst_row;

    /* FIXME: If source (but not dest) is partially scrolled out of view, then
     *        the delta will not be updated properly. We need to pull the data
     *        from the buffer in such a case.
     */

    /* Update delta */
    guac_terminal_delta_copy(term->delta,
        scrolled_dst_row, dst_col,
        scrolled_src_row, src_col,
        cols, rows);

    /* Update buffer */
    guac_terminal_buffer_copy(term->buffer,
        dst_row, dst_col,
        src_row, src_col,
        cols, rows);

    return 0;

}


int guac_terminal_clear(guac_terminal* term,
        int row, int col, int rows, int cols) {

    int scrolled_row = row + term->scroll_offset;
    int scrolled_rows = rows;

    /* Adjust delta rect height if scrolled out of view */
    if (scrolled_row + scrolled_rows > term->delta->height)
        scrolled_rows = term->delta->height - scrolled_row;

    /* Build space */
    guac_terminal_char character;
    character.value = ' ';
    character.attributes = term->current_attributes;

    /* Fill with color */
    guac_terminal_delta_set_rect(term->delta,
        scrolled_row, col, cols, scrolled_rows, &character);

    guac_terminal_buffer_set_rect(term->buffer,
        row, col, cols, rows, &character);

    return 0;

}

int guac_terminal_scroll_up(guac_terminal* term,
        int start_row, int end_row, int amount) {

    /* Calculate height of scroll region */
    int height = end_row - start_row + 1;
    
    /* If scroll region is entire screen, push rows into scrollback */
    if (start_row == 0 && end_row == term->term_height-1)
        guac_terminal_scrollback_buffer_append(term->scrollback, term, amount);

    return 

        /* Move rows within scroll region up by the given amount */
        guac_terminal_copy(term,
                start_row + amount, 0,
                height - amount, term->term_width,
                start_row, 0)

        /* Fill new rows with background */
        || guac_terminal_clear(term,
                end_row - amount + 1, 0, amount, term->term_width);

}

int guac_terminal_scroll_down(guac_terminal* term,
        int start_row, int end_row, int amount) {

    /* Calculate height of scroll region */
    int height = end_row - start_row + 1;
    
    return 

        /* Move rows within scroll region down by the given amount */
        guac_terminal_copy(term,
                start_row, 0,
                height - amount, term->term_width,
                start_row + amount, 0)

        /* Fill new rows with background */
        || guac_terminal_clear(term,
                start_row, 0, amount, term->term_width);

}

int guac_terminal_clear_range(guac_terminal* term,
        int start_row, int start_col,
        int end_row, int end_col) {

    /* If not at far left, must clear sub-region to far right */
    if (start_col > 0) {

        /* Clear from start_col to far right */
        if (guac_terminal_clear(term,
                start_row, start_col, 1, term->term_width - start_col))
            return 1;

        /* One less row to clear */
        start_row++;
    }

    /* If not at far right, must clear sub-region to far left */
    if (end_col < term->term_width - 1) {

        /* Clear from far left to end_col */
        if (guac_terminal_clear(term,
                end_row, 0, 1, end_col + 1))
            return 1;

        /* One less row to clear */
        end_row--;

    }

    /* Remaining region now guaranteed rectangular. Clear, if possible */
    if (start_row <= end_row) {

        if (guac_terminal_clear(term,
                start_row, 0, end_row - start_row + 1, term->term_width))
            return 1;

    }

    return 0;

}

guac_terminal_delta* guac_terminal_delta_alloc(int width, int height) {

    guac_terminal_operation* current;
    int x, y;

    /* Allocate delta */
    guac_terminal_delta* delta = malloc(sizeof(guac_terminal_delta));

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

    return delta;

}

void guac_terminal_delta_free(guac_terminal_delta* delta) {

    /* Free operations buffers */
    free(delta->operations);
    free(delta->scratch);

    /* Free delta */
    free(delta);

}

void guac_terminal_delta_resize(guac_terminal_delta* delta,
    int width, int height) {
    /* STUB */
}

void guac_terminal_delta_set(guac_terminal_delta* delta, int r, int c,
        guac_terminal_char* character) {

    /* Get operation at coordinate */
    guac_terminal_operation* op = &(delta->operations[r*delta->width + c]);

    /* Store operation */
    op->type = GUAC_CHAR_SET;
    op->character = *character;

}

void guac_terminal_delta_copy(guac_terminal_delta* delta,
        int dst_row, int dst_column,
        int src_row, int src_column,
        int w, int h) {

    int row, column;

    /* FIXME: Handle intersections between src and dst rects */

    memcpy(delta->scratch, delta->operations, 
            sizeof(guac_terminal_operation) * delta->width * delta->height);

    guac_terminal_operation* current_row =
        &(delta->operations[dst_row*delta->width + dst_column]);

    guac_terminal_operation* src_current_row =
        &(delta->scratch[src_row*delta->width + src_column]);

    /* Set rectangle to copy operations */
    for (row=0; row<h; row++) {

        guac_terminal_operation* current = current_row;
        guac_terminal_operation* src_current = src_current_row;

        for (column=0; column<w; column++) {

            /* If copying existing delta operation, just copy that rather
             * than create a new copy op */
            if (src_current->type != GUAC_CHAR_NOP)
                *current = *src_current;

            /* Store operation */
            else {
                current->type = GUAC_CHAR_COPY;
                current->row = src_row + row;
                current->column = src_column + column;
            }

            /* Next column */
            current++;
            src_current++;

        }

        /* Next row */
        current_row += delta->width;
        src_current_row += delta->width;

    }



}

void guac_terminal_delta_set_rect(guac_terminal_delta* delta,
        int row, int column, int w, int h,
        guac_terminal_char* character) {

    guac_terminal_operation* current_row =
        &(delta->operations[row*delta->width + column]);

    /* Set rectangle contents to given character */
    for (row=0; row<h; row++) {

        guac_terminal_operation* current = current_row;

        for (column=0; column<w; column++) {

            /* Store operation */
            current->type = GUAC_CHAR_SET;
            current->character = *character;

            /* Next column */
            current++;

        }

        /* Next row */
        current_row += delta->width;

    }

}

void __guac_terminal_delta_flush_copy(guac_terminal_delta* delta,
        guac_terminal* terminal) {

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
                guac_protocol_send_copy(terminal->client->socket,

                        GUAC_DEFAULT_LAYER,
                        current->column * terminal->char_width,
                        current->row * terminal->char_height,
                        rect_width * terminal->char_width,
                        rect_height * terminal->char_height,

                        GUAC_COMP_OVER,
                        GUAC_DEFAULT_LAYER,
                        col * terminal->char_width,
                        row * terminal->char_height);


            } /* end if copy operation */

            /* Next operation */
            current++;

        }
    }

}

void __guac_terminal_delta_flush_clear(guac_terminal_delta* delta,
        guac_terminal* terminal) {

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
                if (current->character.attributes.reverse)
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
                guac_protocol_send_rect(terminal->client->socket,
                        GUAC_DEFAULT_LAYER,
                        col * terminal->char_width,
                        row * terminal->char_height,
                        rect_width * terminal->char_width,
                        rect_height * terminal->char_height);

                guac_protocol_send_cfill(terminal->client->socket,
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

void __guac_terminal_delta_flush_set(guac_terminal_delta* delta,
        guac_terminal* terminal) {

    guac_terminal_operation* current = delta->operations;
    int row, col;

    /* For each operation */
    for (row=0; row<delta->height; row++) {
        for (col=0; col<delta->width; col++) {

            /* Perform given operation */
            if (current->type == GUAC_CHAR_SET) {

                /* Set attributes */
                __guac_terminal_set_colors(terminal,
                        &(current->character.attributes));

                /* Send character */
                __guac_terminal_set(terminal, row, col,
                        current->character.value);

                /* Mark operation as handled */
                current->type = GUAC_CHAR_NOP;

            }

            /* Next operation */
            current++;

        }
    }

}

void guac_terminal_delta_flush(guac_terminal_delta* delta,
        guac_terminal* terminal) {

    /* Flush copy operations first */
    __guac_terminal_delta_flush_copy(delta, terminal);

    /* Flush clear operations (as they're just rects) */
    __guac_terminal_delta_flush_clear(delta, terminal);

    /* Flush set operations (the only operations remaining) */
    __guac_terminal_delta_flush_set(delta, terminal);

}

guac_terminal_buffer* guac_terminal_buffer_alloc(int width, int height) {

    /* Allocate buffer */
    guac_terminal_buffer* buffer = malloc(sizeof(guac_terminal_buffer));

    /* Set width and height */
    buffer->width = width;
    buffer->height = height;

    /* Alloc characters */
    buffer->characters = malloc(width * height *
            sizeof(guac_terminal_char));

    return buffer;

}

void guac_terminal_buffer_resize(guac_terminal_buffer* buffer, 
        int width, int height) {
    /* STUB */
}

void guac_terminal_buffer_free(guac_terminal_buffer* buffer) {

    /* Free characters */
    free(buffer->characters);

    /* Free buffer*/
    free(buffer);

}

void guac_terminal_buffer_set(guac_terminal_buffer* buffer, int r, int c,
        guac_terminal_char* character) {

    /* Store character */
    buffer->characters[r * buffer->width + c] = *character;

}

void guac_terminal_buffer_copy(guac_terminal_buffer* buffer,
        int dst_row, int dst_column,
        int src_row, int src_column,
        int w, int h) {

    int row, column;

    /* FIXME: Handle intersections between src and dst rects */

    guac_terminal_char* current_row =
        &(buffer->characters[dst_row*buffer->width + dst_column]);

    guac_terminal_char* src_current_row =
        &(buffer->characters[src_row*buffer->width + src_column]);

    /* Set rectangle to copy operations */
    for (row=0; row<h; row++) {

        guac_terminal_char* current = current_row;
        guac_terminal_char* src_current = src_current_row;

        for (column=0; column<w; column++) {

            *current = *src_current;

            /* Next column */
            current++;
            src_current++;

        }

        /* Next row */
        current_row += buffer->width;
        src_current_row += buffer->width;

    }

}

void guac_terminal_buffer_set_rect(guac_terminal_buffer* buffer,
        int row, int column, int w, int h,
        guac_terminal_char* character) {

    guac_terminal_char* current_row =
        &(buffer->characters[row*buffer->width + column]);

    /* Set rectangle contents to given character */
    for (row=0; row<h; row++) {

        /* Copy character throughout row */
        guac_terminal_char* current = current_row;
        for (column=0; column<w; column++)
            *(current++) = *character;

        /* Next row */
        current_row += buffer->width;

    }

}

guac_terminal_scrollback_buffer*
    guac_terminal_scrollback_buffer_alloc(int rows) {

    /* Allocate scrollback */
    guac_terminal_scrollback_buffer* buffer =
        malloc(sizeof(guac_terminal_scrollback_buffer));

    int i;
    guac_terminal_scrollback_row* row;

    /* Init scrollback data */
    buffer->rows = rows;
    buffer->top = 0;
    buffer->length = 0;
    buffer->scrollback = malloc(sizeof(guac_terminal_scrollback_row) *
            buffer->rows);

    /* Init scrollback rows */
    row = buffer->scrollback;
    for (i=0; i<rows; i++) {

        /* Allocate row  */
        row->available = 256;
        row->length = 0;
        row->characters = malloc(sizeof(guac_terminal_char) * row->available);

        /* Next row */
        row++;

    }

    return buffer;

}

void guac_terminal_scrollback_buffer_free(
    guac_terminal_scrollback_buffer* buffer) {

    int i;
    guac_terminal_scrollback_row* row = buffer->scrollback;

    /* Free all rows */
    for (i=0; i<buffer->rows; i++) {
        free(row->characters);
        row++;
    }

    /* Free actual buffer */
    free(buffer->scrollback);
    free(buffer);

}

void guac_terminal_scrollback_buffer_append(
    guac_terminal_scrollback_buffer* buffer,
    guac_terminal* terminal, int rows) {

    int row, column;

    /* Copy data into scrollback */
    guac_terminal_scrollback_row* scrollback_row =
        &(buffer->scrollback[buffer->top]);
    guac_terminal_char* current = terminal->buffer->characters;

    for (row=0; row<rows; row++) {

        /* FIXME: Assumes scrollback row large enough */

        /* Copy character data for row */
        guac_terminal_char* dest = scrollback_row->characters;
        for (column=0; column < terminal->buffer->width; column++)
            *(dest++) = *(current++);

        scrollback_row->length = terminal->buffer->width;

        /* Next scrollback row */
        scrollback_row++;
        buffer->top++;

        /* Wrap around when bottom reached */
        if (buffer->top == buffer->rows) {
            buffer->top = 0;
            scrollback_row = buffer->scrollback;
        }

    } /* end for each row */

    /* Increment row count */
    buffer->length += rows;
    if (buffer->length > buffer->rows)
        buffer->length = buffer->rows;

    /* Log string version of row that WOULD have been scrolled into the
     * scrollback */
    guac_client_log_info(terminal->client,
            "scrollback->top=%i (length=%i/%i)", buffer->top, buffer->length, buffer->rows);

}

void guac_terminal_scroll_display_down(guac_terminal* terminal) {

    int scroll_amount = 3;

    int start_row, end_row;
    int dest_row;
    int row, column;

    /* Limit scroll amount by size of scrollback buffer */
    if (scroll_amount > terminal->scroll_offset)
        scroll_amount = terminal->scroll_offset;

    /* If not scrolling at all, don't bother trying */
    if (scroll_amount == 0)
        return;

    /* Shift screen up */
    if (terminal->term_height > scroll_amount)
        guac_terminal_delta_copy(terminal->delta,
                0,             0, /* Destination row, col */
                scroll_amount, 0, /* source row,col */
                terminal->term_width, terminal->term_height - scroll_amount);

    /* Advance by scroll amount */
    terminal->scroll_offset -= scroll_amount;

    /* Get row range */
    end_row   = terminal->term_height - terminal->scroll_offset - 1;
    start_row = end_row - scroll_amount + 1;
    dest_row  = terminal->term_height - scroll_amount;

    guac_client_log_info(terminal->client,
            "Scrolling rows %i through %i into view (scroll down)",
            start_row, end_row);

    /* Draw new rows from scrollback */
    for (row=start_row; row<=end_row; row++) {

        /* If row in past, pull from scrollback */
        if (row < 0) {

            /* Get row from scrollback */
            guac_terminal_scrollback_row* scrollback_row = 
                guac_terminal_scrollback_buffer_get_row(terminal->scrollback,
                        row);

            /* Draw row */
            /* FIXME: Clear row first */
            guac_terminal_char* current = scrollback_row->characters;
            for (column=0; column<scrollback_row->length; column++)
                guac_terminal_delta_set(terminal->delta, dest_row, column,
                        current++);

        }

        /* Otherwise, pull from buffer */
        else {

            guac_terminal_char* current = &(terminal->buffer->characters[
                terminal->buffer->width * row]);

            for (column=0; column<terminal->buffer->width; column++)
                guac_terminal_delta_set(terminal->delta, dest_row, column,
                        current++);

        }

        /* Next row */
        dest_row++;

    }

    /* FIXME: Should flush somewhere more sensible */
    guac_terminal_delta_flush(terminal->delta, terminal);
    guac_socket_flush(terminal->client->socket);

}

void guac_terminal_scroll_display_up(guac_terminal* terminal) {

    int scroll_amount = 3;

    int start_row, end_row;
    int dest_row;
    int row, column;


    /* Limit scroll amount by size of scrollback buffer */
    if (terminal->scroll_offset + scroll_amount > terminal->scrollback->length)
        scroll_amount = terminal->scrollback->length - terminal->scroll_offset;

    /* If not scrolling at all, don't bother trying */
    if (scroll_amount == 0)
        return;

    /* Shift screen down */
    if (terminal->term_height > scroll_amount)
        guac_terminal_delta_copy(terminal->delta,
                scroll_amount, 0, /* Destination row,col */
                0,             0, /* Source row, col */
                terminal->term_width, terminal->term_height - scroll_amount);

    /* Advance by scroll amount */
    terminal->scroll_offset += scroll_amount;

    /* Get row range */
    start_row = -terminal->scroll_offset;
    end_row   = start_row + scroll_amount - 1;
    dest_row  = 0;

    guac_client_log_info(terminal->client,
            "Scrolling rows %i through %i into view (scroll up)",
            start_row, end_row);

    /* Draw new rows from scrollback */
    for (row=start_row; row<=end_row; row++) {

        /* Get row from scrollback */
        guac_terminal_scrollback_row* scrollback_row = 
            guac_terminal_scrollback_buffer_get_row(terminal->scrollback, row);

        /* Draw row */
        /* FIXME: Clear row first */
        guac_terminal_char* current = scrollback_row->characters;
        for (column=0; column<scrollback_row->length; column++)
            guac_terminal_delta_set(terminal->delta, dest_row, column,
                    current++);

        /* Next row */
        dest_row++;

    }

    /* FIXME: Should flush somewhere more sensible */
    guac_terminal_delta_flush(terminal->delta, terminal);
    guac_socket_flush(terminal->client->socket);

}

guac_terminal_scrollback_row* guac_terminal_scrollback_buffer_get_row(
    guac_terminal_scrollback_buffer* buffer, int row) {

    /* Calculate scrollback row index */
    int index = buffer->top + row;
    if (index < 0) index += buffer->rows;

    /* Return found row */
    return &(buffer->scrollback[index]);

}

