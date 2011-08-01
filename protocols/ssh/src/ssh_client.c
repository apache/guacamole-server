
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

/* Client plugin arguments */
const char* GUAC_CLIENT_ARGS[] = {
    "hostname",
    "port",
    NULL
};

typedef struct ssh_guac_client_data {

    PangoFontDescription* font_desc;

    guac_layer* glyphs[256];

    int char_width;
    int char_height;

    int term_width;
    int term_height;

    int cursor_row;
    int cursor_col;

} ssh_guac_client_data;

int ssh_guac_client_send_glyph(guac_client* client, int row, int col, char c);
int ssh_guac_client_print(guac_client* client, const char* c);

int guac_client_init(guac_client* client, int argc, char** argv) {

    GUACIO* io = client->io;

    PangoFontMap* font_map;
    PangoFont* font;
    PangoFontMetrics* metrics;
    PangoContext* context;

    ssh_guac_client_data* client_data = malloc(sizeof(ssh_guac_client_data));

    client_data->cursor_row = 0;
    client_data->cursor_col = 0;

    client_data->term_width = 80;
    client_data->term_height = 25;

    /* Get font */
    client_data->font_desc = pango_font_description_new();
    pango_font_description_set_family(client_data->font_desc, "monospace");
    pango_font_description_set_weight(client_data->font_desc, PANGO_WEIGHT_NORMAL);
    pango_font_description_set_size(client_data->font_desc, 16*PANGO_SCALE);

    font_map = pango_cairo_font_map_get_default();
    context = pango_font_map_create_context(font_map);

    font = pango_font_map_load_font(font_map, context, client_data->font_desc);
    if (font == NULL) {
        guac_log_error("Unable to get font.");
        return 1;
    }

    metrics = pango_font_get_metrics(font, NULL);
    if (metrics == NULL) {
        guac_log_error("Unable to get font metrics.");
        return 1;
    }

    /* Calculate character dimensions */
    client_data->char_width =
        pango_font_metrics_get_approximate_digit_width(metrics) / PANGO_SCALE;
    client_data->char_height =
        (pango_font_metrics_get_descent(metrics)
            + pango_font_metrics_get_ascent(metrics)) / PANGO_SCALE;

    client->data = client_data;

    /* Send name and dimensions */
    guac_send_name(io, "SSH TEST");
    guac_send_size(io,
            client_data->char_width  * client_data->term_width,
            client_data->char_height * client_data->term_height);

    guac_flush(io);

    ssh_guac_client_print(client, "Hello World!\r\nThis is a test of the new Guacamole SSH client plugin!!!\r\n");
    guac_flush(io);

    ssh_guac_client_print(client, "ROW 1\r\n"); guac_flush(io);
    ssh_guac_client_print(client, "ROW 2\r\n"); guac_flush(io);
    ssh_guac_client_print(client, "ROW 3\r\n"); guac_flush(io);
    ssh_guac_client_print(client, "ROW 4\r\n"); guac_flush(io);
    ssh_guac_client_print(client, "ROW 5\r\n"); guac_flush(io);
    ssh_guac_client_print(client, "ROW 6\r\n"); guac_flush(io);
    ssh_guac_client_print(client, "ROW 8\r\n"); guac_flush(io);
    ssh_guac_client_print(client, "ROW 9\r\n"); guac_flush(io);
    ssh_guac_client_print(client, "ROW 10\r\n"); guac_flush(io);
    ssh_guac_client_print(client, "ROW 11\r\n"); guac_flush(io);
    ssh_guac_client_print(client, "ROW 12\r\n"); guac_flush(io);
    ssh_guac_client_print(client, "ROW 13\r\n"); guac_flush(io);
    ssh_guac_client_print(client, "ROW 14\r\n"); guac_flush(io);
    ssh_guac_client_print(client, "ROW 15\r\n"); guac_flush(io);
    ssh_guac_client_print(client, "ROW 16\r\n"); guac_flush(io);
    ssh_guac_client_print(client, "ROW 17\r\n"); guac_flush(io);
    ssh_guac_client_print(client, "ROW 18\r\n"); guac_flush(io);
    ssh_guac_client_print(client, "ROW 19\r\n"); guac_flush(io);
    ssh_guac_client_print(client, "ROW 20\r\n"); guac_flush(io);
    ssh_guac_client_print(client, "ROW 21\r\n"); guac_flush(io);
    ssh_guac_client_print(client, "ROW 22\r\n"); guac_flush(io);
    ssh_guac_client_print(client, "ROW 23\r\n"); guac_flush(io);
    ssh_guac_client_print(client, "ROW 24\r\n"); guac_flush(io);

    /* Success */
    return 0;

}

guac_layer* ssh_guac_client_get_glyph(guac_client* client, char c) {

    GUACIO* io = client->io;
    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    guac_layer* glyph;

    cairo_surface_t* surface;
    cairo_t* cairo;
   
    PangoLayout* layout;

    /* Return glyph if exists */
    if (client_data->glyphs[(int) c])
        return client_data->glyphs[(int) c];

    /* Otherwise, draw glyph */
    surface = cairo_image_surface_create(
            CAIRO_FORMAT_ARGB32,
            client_data->char_width, client_data->char_height);
    cairo = cairo_create(surface);

    /* Get layout */
    layout = pango_cairo_create_layout(cairo);
    pango_layout_set_font_description(layout, client_data->font_desc);
    pango_layout_set_text(layout, &c, 1);

    /* Draw */
    cairo_set_source_rgba(cairo, 1.0, 1.0, 1.0, 1.0);
    cairo_move_to(cairo, 0.0, 0.0);
    pango_cairo_show_layout(cairo, layout);

    /* Free all */
    g_object_unref(layout);
    cairo_destroy(cairo);

    /* Send glyph and save */
    glyph = guac_client_alloc_buffer(client);
    guac_send_png(io, GUAC_COMP_OVER, glyph, 0, 0, surface);
    client_data->glyphs[(int) c] = glyph;

    guac_flush(io);
    cairo_surface_destroy(surface);

    /* Return glyph */
    return glyph;

}

int ssh_guac_client_send_glyph(guac_client* client, int row, int col, char c) {

    GUACIO* io = client->io;
    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;
    guac_layer* glyph = ssh_guac_client_get_glyph(client, c);

    return guac_send_copy(io,
            glyph, 0, 0, client_data->char_width, client_data->char_height,
            GUAC_COMP_OVER, GUAC_DEFAULT_LAYER,
            client_data->char_width * col,
            client_data->char_height * row);

}

int ssh_guac_client_print(guac_client* client, const char* c) {

    GUACIO* io = client->io;
    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;

    while (*c != 0) {

        switch (*c) {

            /* Carriage return */
            case '\r':
                client_data->cursor_col = 0;
                break;

            /* Line feed */
            case '\n':
                client_data->cursor_row++;
                break;

            /* Displayable chars */
            default:
                ssh_guac_client_send_glyph(client,
                        client_data->cursor_row,
                        client_data->cursor_col,
                        *c);

                /* Advance cursor, wrap if necessary */
                client_data->cursor_col++;
                if (client_data->cursor_col >= client_data->term_width) {
                    client_data->cursor_col = 0;
                    client_data->cursor_row++;
                }
        }

        /* Scroll up if necessary */
        if (client_data->cursor_row >= client_data->term_height) {
            client_data->cursor_row = client_data->term_height - 1;
            
            /* Copy screen up by one row */
            guac_send_copy(io,
                    GUAC_DEFAULT_LAYER, 0, client_data->char_height,
                    client_data->char_width * client_data->term_width,
                    client_data->char_height * (client_data->term_height - 1),
                    GUAC_COMP_SRC, GUAC_DEFAULT_LAYER, 0, 0);

            /* Fill bottom row with background */
            guac_send_rect(io,
                    GUAC_COMP_SRC, GUAC_DEFAULT_LAYER,
                    0, client_data->char_height * (client_data->term_height - 1),
                    client_data->char_width * client_data->term_width,
                    client_data->char_height * client_data->term_height,
                    0, 0, 0, 255);

        }

        c++;
    }

    return 0;

}

