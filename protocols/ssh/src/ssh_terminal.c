
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

#include <cairo/cairo.h>
#include <pango/pangocairo.h>

#include <guacamole/log.h>
#include <guacamole/guacio.h>
#include <guacamole/protocol.h>
#include <guacamole/client.h>

#include "ssh_terminal.h"
#include "ssh_terminal_handlers.h"

const ssh_guac_terminal_color ssh_guac_terminal_palette[16] = {

    /* Normal colors */
    {0x00, 0x00, 0x00}, /* Black   */
    {0x80, 0x00, 0x00}, /* Red     */
    {0x00, 0x80, 0x00}, /* Green   */
    {0x80, 0x80, 0x00}, /* Brown   */
    {0x00, 0x00, 0x80}, /* Blue    */
    {0x80, 0x00, 0x80}, /* Magenta */
    {0x00, 0x80, 0x80}, /* Cyan    */
    {0x80, 0x80, 0x80}, /* White   */

    /* Intense colors */
    {0x40, 0x40, 0x40}, /* Black   */
    {0xFF, 0x00, 0x00}, /* Red     */
    {0x00, 0xFF, 0x00}, /* Green   */
    {0xFF, 0xFF, 0x00}, /* Brown   */
    {0x00, 0x00, 0xFF}, /* Blue    */
    {0xFF, 0x00, 0xFF}, /* Magenta */
    {0x00, 0xFF, 0xFF}, /* Cyan    */
    {0xFF, 0xFF, 0xFF}, /* White   */

};

ssh_guac_terminal* ssh_guac_terminal_create(guac_client* client) {

    PangoFontMap* font_map;
    PangoFont* font;
    PangoFontMetrics* metrics;
    PangoContext* context;

    ssh_guac_terminal* term = malloc(sizeof(ssh_guac_terminal));
    term->client = client;

    term->foreground = term->default_foreground = 7; /* White */
    term->background = term->default_background = 0; /* Black */

    term->cursor_row = 0;
    term->cursor_col = 0;

    term->term_width = 160;
    term->term_height = 50;
    term->char_handler = ssh_guac_terminal_echo; 

    /* Get font */
    term->font_desc = pango_font_description_new();
    pango_font_description_set_family(term->font_desc, "monospace");
    pango_font_description_set_weight(term->font_desc, PANGO_WEIGHT_NORMAL);
    pango_font_description_set_size(term->font_desc, 8*PANGO_SCALE);

    font_map = pango_cairo_font_map_get_default();
    context = pango_font_map_create_context(font_map);

    font = pango_font_map_load_font(font_map, context, term->font_desc);
    if (font == NULL) {
        guac_log_error("Unable to get font.");
        return NULL;
    }

    metrics = pango_font_get_metrics(font, NULL);
    if (metrics == NULL) {
        guac_log_error("Unable to get font metrics.");
        return NULL;
    }

    /* Calculate character dimensions */
    term->char_width =
        pango_font_metrics_get_approximate_digit_width(metrics) / PANGO_SCALE;
    term->char_height =
        (pango_font_metrics_get_descent(metrics)
            + pango_font_metrics_get_ascent(metrics)) / PANGO_SCALE;

    /* Clear with background color */
    ssh_guac_terminal_clear(term,
            0, 0, term->term_width, term->term_height,
            term->background);

    return term;

}

void ssh_guac_terminal_free(ssh_guac_terminal* term) {
    /* STUB */
}

guac_layer* __ssh_guac_terminal_get_glyph(ssh_guac_terminal* term, char c) {

    GUACIO* io = term->client->io;
    guac_layer* glyph;
    
    /* Use default foreground color */
    const ssh_guac_terminal_color* color =
        &ssh_guac_terminal_palette[term->default_foreground];

    cairo_surface_t* surface;
    cairo_t* cairo;
   
    PangoLayout* layout;

    /* Return glyph if exists */
    if (term->glyphs[(int) c])
        return term->glyphs[(int) c];

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

    /* Send glyph and save */
    glyph = guac_client_alloc_buffer(term->client);
    guac_send_png(io, GUAC_COMP_OVER, glyph, 0, 0, surface);
    term->glyphs[(int) c] = glyph;

    guac_flush(io);
    cairo_surface_destroy(surface);

    /* Return glyph */
    return glyph;

}

int ssh_guac_terminal_send_glyph(ssh_guac_terminal* term, int row, int col, char c) {

    GUACIO* io = term->client->io;
    guac_layer* glyph = __ssh_guac_terminal_get_glyph(term, c);

    return guac_send_copy(io,
            glyph, 0, 0, term->char_width, term->char_height,
            GUAC_COMP_SRC, GUAC_DEFAULT_LAYER,
            term->char_width * col,
            term->char_height * row);

}

int ssh_guac_terminal_write(ssh_guac_terminal* term, const char* c, int size) {

    while (size > 0) {
        term->char_handler(term, *(c++));
        size--;
    }

    return 0;

}

int ssh_guac_terminal_copy(ssh_guac_terminal* term,
        int src_row, int src_col, int rows, int cols,
        int dst_row, int dst_col) {

    GUACIO* io = term->client->io;

    /* Send copy instruction */
    return guac_send_copy(io,

            GUAC_DEFAULT_LAYER,
            src_col * term->char_width, src_row * term->char_height,
            cols    * term->char_width, rows    * term->char_height,

            GUAC_COMP_SRC, GUAC_DEFAULT_LAYER,
            dst_col * term->char_width, dst_row * term->char_height);

}


int ssh_guac_terminal_clear(ssh_guac_terminal* term,
        int row, int col, int rows, int cols, int background_color) {

    GUACIO* io = term->client->io;
    const ssh_guac_terminal_color* color =
        &ssh_guac_terminal_palette[background_color];

    /* Fill with color */
    return guac_send_rect(io,
            GUAC_COMP_SRC, GUAC_DEFAULT_LAYER,

            col  * term->char_width, row  * term->char_height,
            cols * term->char_width, rows * term->char_height,

            color->red, color->green, color->blue, 255);

}

int ssh_guac_terminal_scroll_up(ssh_guac_terminal* term,
        int start_row, int end_row, int amount) {

    /* Calculate height of scroll region */
    int height = end_row - start_row + 1;
    
    return 

        /* Move rows within scroll region up by the given amount */
        ssh_guac_terminal_copy(term,
                start_row + amount, 0,
                height - amount, term->term_width,
                start_row, 0)

        /* Fill new rows with background */
        || ssh_guac_terminal_clear(term,
                end_row - amount + 1, 0, amount, term->term_width,
                term->background);

}


int ssh_guac_terminal_clear_range(ssh_guac_terminal* term,
        int start_row, int start_col,
        int end_row, int end_col, int background_color) {

    /* If not at far left, must clear sub-region to far right */
    if (start_col > 0) {

        /* Clear from start_col to far right */
        if (ssh_guac_terminal_clear(term,
                start_row, start_col, 1, term->term_width - start_col,
                background_color))
            return 1;

        /* One less row to clear */
        start_row++;
    }

    /* If not at far right, must clear sub-region to far left */
    if (end_col < term->term_width - 1) {

        /* Clear from far left to end_col */
        if (ssh_guac_terminal_clear(term,
                end_row, 0, 1, end_col + 1,
                background_color))
            return 1;

        /* One less row to clear */
        end_row--;

    }

    /* Remaining region now guaranteed rectangular. Clear, if possible */
    if (start_row <= end_row) {

        if (ssh_guac_terminal_clear(term,
                start_row, 0, end_row - start_row + 1, term->term_width,
                background_color))
            return 1;

    }

    return 0;

}

