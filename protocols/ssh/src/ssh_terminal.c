
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

ssh_guac_terminal* ssh_guac_terminal_create(guac_client* client) {

    PangoFontMap* font_map;
    PangoFont* font;
    PangoFontMetrics* metrics;
    PangoContext* context;

    ssh_guac_terminal* term = malloc(sizeof(ssh_guac_terminal));
    term->client = client;

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

    return term;

}

void ssh_guac_terminal_free(ssh_guac_terminal* term) {
    /* STUB */
}

guac_layer* __ssh_guac_terminal_get_glyph(ssh_guac_terminal* term, char c) {

    GUACIO* io = term->client->io;
    guac_layer* glyph;

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
    cairo_set_source_rgba(cairo, 1.0, 1.0, 1.0, 1.0);
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

