
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

    int row, col;

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
    pango_font_description_set_size(term->font_desc, 10*PANGO_SCALE);

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
    term->cursor_layer = guac_client_alloc_layer(client);

    term->term_width = width / term->char_width;
    term->term_height = height / term->char_height;
    term->char_handler = guac_terminal_echo; 

    term->scroll_start = 0;
    term->scroll_end = term->term_height - 1;

    /* Create scrollback buffer */
    term->scrollback = malloc(term->term_height * sizeof(guac_terminal_char*));

    /* Init buffer */
    for (row = 0; row < term->term_height; row++) {

        /* Create row */
        guac_terminal_char* current_row =
            term->scrollback[row] = malloc(term->term_width * sizeof(guac_terminal_char));

        /* Init row */
        for (col = 0; col < term->term_width; col++) {

            /* Empty character, default colors */
            current_row[col].value = '\0';
            current_row[col].attributes = term->default_attributes;

        }

    }

    /* Init delta */
    term->delta = guac_terminal_delta_alloc(width, height);

    /* Clear with background color */
    guac_terminal_clear(term,
            0, 0, term->term_height, term->term_width,
            term->current_attributes.background);

    return term;

}

void guac_terminal_free(guac_terminal* term) {
    
    /* Free scrollback buffer */
    for (int row = 0; row < term->term_height; row++) 
        free(term->scrollback[row]);

    free(term->scrollback);

    /* Free delta */
    guac_terminal_delta_free(term->delta);

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

    /* Build character with current attributes */
    guac_terminal_char guac_char;
    guac_char.value = c;
    guac_char.attributes = term->current_attributes;

    /* Set delta */
    guac_terminal_delta_set(term->delta, row, col, &guac_char);
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

    /* Update delta */
    guac_terminal_delta_copy(term->delta,
        dst_row, dst_col,
        src_row, src_col,
        cols, rows);

    return 0;

}


int guac_terminal_clear(guac_terminal* term,
        int row, int col, int rows, int cols, int background_color) {

    /* Build space */
    guac_terminal_char character;
    character.value = ' ';
    character.attributes.reverse = false;
    character.attributes.background = background_color;

    /* Fill with color */
    guac_terminal_delta_set_rect(term->delta,
        row, col, cols, rows, &character);

    return 0;

}

int guac_terminal_scroll_up(guac_terminal* term,
        int start_row, int end_row, int amount) {

    /* Calculate height of scroll region */
    int height = end_row - start_row + 1;
    
    return 

        /* Move rows within scroll region up by the given amount */
        guac_terminal_copy(term,
                start_row + amount, 0,
                height - amount, term->term_width,
                start_row, 0)

        /* Fill new rows with background */
        || guac_terminal_clear(term,
                end_row - amount + 1, 0, amount, term->term_width,
                term->current_attributes.background);

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
                start_row, 0, amount, term->term_width,
                term->current_attributes.background);

}

int guac_terminal_clear_range(guac_terminal* term,
        int start_row, int start_col,
        int end_row, int end_col, int background_color) {

    /* If not at far left, must clear sub-region to far right */
    if (start_col > 0) {

        /* Clear from start_col to far right */
        if (guac_terminal_clear(term,
                start_row, start_col, 1, term->term_width - start_col,
                background_color))
            return 1;

        /* One less row to clear */
        start_row++;
    }

    /* If not at far right, must clear sub-region to far left */
    if (end_col < term->term_width - 1) {

        /* Clear from far left to end_col */
        if (guac_terminal_clear(term,
                end_row, 0, 1, end_col + 1,
                background_color))
            return 1;

        /* One less row to clear */
        end_row--;

    }

    /* Remaining region now guaranteed rectangular. Clear, if possible */
    if (start_row <= end_row) {

        if (guac_terminal_clear(term,
                start_row, 0, end_row - start_row + 1, term->term_width,
                background_color))
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

    return delta;

}

void guac_terminal_delta_free(guac_terminal_delta* delta) {

    /* Free operations buffer */
    free(delta->operations);

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
    /* STUB */
}

void guac_terminal_delta_set_rect(guac_terminal_delta* delta,
        int row, int column, int w, int h,
        guac_terminal_char* character) {
    /* STUB */
}

void guac_terminal_delta_flush(guac_terminal_delta* delta,
        guac_terminal* terminal) {

    guac_terminal_operation* current = delta->operations;
    int row, col;

    /* For each operation */
    for (row=0; row<delta->height; row++) {
        for (col=0; col<delta->width; col++) {

            /* Perform given operation */
            if (current->type == GUAC_CHAR_SET) {

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

