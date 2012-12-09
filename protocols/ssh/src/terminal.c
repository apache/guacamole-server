
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

    int row, col;

    PangoFontMap* font_map;
    PangoFont* font;
    PangoFontMetrics* metrics;
    PangoContext* context;

    guac_terminal* term = malloc(sizeof(guac_terminal));
    term->client = client;

    term->glyph_foreground = term->foreground = term->default_foreground = 7; /* White */
    term->glyph_background = term->background = term->default_background = 0; /* Black */
    term->reverse = 0;    /* Normal video */
    term->bold = 0;       /* Normal intensity */
    term->underscore = 0; /* No underline */

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
            current_row[col].foreground = term->default_foreground;
            current_row[col].background = term->default_background;

        }

    }

    /* Clear with background color */
    guac_terminal_clear(term,
            0, 0, term->term_height, term->term_width,
            term->background);

    return term;

}

void guac_terminal_free(guac_terminal* term) {
    
    /* Free scrollback buffer */
    for (int row = 0; row < term->term_height; row++) 
        free(term->scrollback[row]);

    free(term->scrollback);
}

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

int guac_terminal_redraw_cursor(guac_terminal* term) {

    guac_socket* socket = term->client->socket;

    /* Erase old cursor */
    return
        guac_protocol_send_move(socket,
            term->cursor_layer,

            GUAC_DEFAULT_LAYER,
            term->char_width * term->cursor_col,
            term->char_height * term->cursor_row,
            1);

}

int guac_terminal_set_colors(guac_terminal* term,
        int foreground, int background) {

    guac_socket* socket = term->client->socket;
    const guac_terminal_color* background_color;

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
    if (foreground != term->glyph_foreground || background != term->glyph_background) {

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

int guac_terminal_set(guac_terminal* term, int row, int col, char c) {

    guac_socket* socket = term->client->socket;
    int location = __guac_terminal_get_glyph(term, c); 

    return guac_protocol_send_copy(socket,
        term->filled_glyphs,
        location * term->char_width, 0, term->char_width, term->char_height,
        GUAC_COMP_OVER, GUAC_DEFAULT_LAYER,
        term->char_width * col,
        term->char_height * row);

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

    guac_socket* socket = term->client->socket;

    /* Send copy instruction */
    return guac_protocol_send_copy(socket,

            GUAC_DEFAULT_LAYER,
            src_col * term->char_width, src_row * term->char_height,
            cols    * term->char_width, rows    * term->char_height,

            GUAC_COMP_OVER, GUAC_DEFAULT_LAYER,
            dst_col * term->char_width, dst_row * term->char_height);

}


int guac_terminal_clear(guac_terminal* term,
        int row, int col, int rows, int cols, int background_color) {

    guac_socket* socket = term->client->socket;
    const guac_terminal_color* color =
        &guac_terminal_palette[background_color];

    /* Fill with color */
    return
        guac_protocol_send_rect(socket, GUAC_DEFAULT_LAYER,
            col  * term->char_width, row  * term->char_height,
            cols * term->char_width, rows * term->char_height)

     || guac_protocol_send_cfill(socket, GUAC_COMP_OVER, GUAC_DEFAULT_LAYER,
            color->red, color->green, color->blue, 255);

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
                term->background);

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
                term->background);

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

