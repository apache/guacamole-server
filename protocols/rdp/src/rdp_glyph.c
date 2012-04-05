
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
 * The Original Code is libguac-client-rdp.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Matt Hortman
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

#include <freerdp/freerdp.h>

#include <guacamole/client.h>

#include "client.h"
#include "rdp_glyph.h"

void guac_rdp_glyph_new(rdpContext* context, rdpGlyph* glyph) {

    int x, y, i;
    int stride;
    unsigned char* image_buffer;
    unsigned char* image_buffer_row;

    unsigned char* data = glyph->aj;
    int width  = glyph->cx;
    int height = glyph->cy;

    /* Init Cairo buffer */
    stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
    image_buffer = malloc(height*stride);
    image_buffer_row = image_buffer;

    /* Copy image data from image data to buffer */
    for (y = 0; y<height; y++) {

        unsigned int*  image_buffer_current;
        
        /* Get current buffer row, advance to next */
        image_buffer_current  = (unsigned int*) image_buffer_row;
        image_buffer_row     += stride;

        for (x = 0; x<width;) {

            /* Get byte from image data */
            unsigned int v = *(data++);

            /* Read bits, write pixels */
            for (i = 0; i<8 && x<width; i++, x++) {

                /* Output RGB */
                if (v & 0x80)
                    *(image_buffer_current++) = 0xFF000000;
                else
                    *(image_buffer_current++) = 0x00000000;

                /* Next bit */
                v <<= 1;

            }

        }
    }

    /* Store glyph surface */
    ((guac_rdp_glyph*) glyph)->surface = cairo_image_surface_create_for_data(
            image_buffer, CAIRO_FORMAT_ARGB32, width, height, stride);

}

void guac_rdp_glyph_draw(rdpContext* context, rdpGlyph* glyph, int x, int y) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;
 
    /* Use glyph as mask */
    cairo_mask_surface(
            guac_client_data->glyph_cairo,
            ((guac_rdp_glyph*) glyph)->surface, x, y);

    /* Fill rectangle with foreground */
    cairo_rectangle(guac_client_data->glyph_cairo, x, y, glyph->cx, glyph->cy);
    cairo_fill(guac_client_data->glyph_cairo);

}

void guac_rdp_glyph_free(rdpContext* context, rdpGlyph* glyph) {

    unsigned char* image_buffer = cairo_image_surface_get_data(
            ((guac_rdp_glyph*) glyph)->surface);

    /* Free surface */
    cairo_surface_destroy(((guac_rdp_glyph*) glyph)->surface);
    free(image_buffer);

}

void guac_rdp_glyph_begindraw(rdpContext* context,
        int x, int y, int width, int height, uint32 fgcolor, uint32 bgcolor) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;

    bgcolor = freerdp_color_convert_var(bgcolor,
            context->instance->settings->color_depth, 32,
            ((rdp_freerdp_context*) context)->clrconv);

    fgcolor = freerdp_color_convert_var(fgcolor,
            context->instance->settings->color_depth, 32,
            ((rdp_freerdp_context*) context)->clrconv);

    guac_client_data->foreground.blue  =  fgcolor & 0x0000FF;
    guac_client_data->foreground.green = (fgcolor & 0x00FF00) >> 8;
    guac_client_data->foreground.red   = (fgcolor & 0xFF0000) >> 16;

    guac_client_data->background.blue   =  bgcolor & 0x0000FF;
    guac_client_data->background.green  = (bgcolor & 0x00FF00) >> 8;
    guac_client_data->background.red    = (bgcolor & 0xFF0000) >> 16;

    /* Create glyph surface and cairo instance */
    guac_client_data->glyph_surface = cairo_image_surface_create(
            CAIRO_FORMAT_RGB24, width, height);

    guac_client_data->glyph_cairo = cairo_create(
        guac_client_data->glyph_surface);

    /* Fill with color */
    cairo_set_source_rgb(guac_client_data->glyph_cairo,
            guac_client_data->background.red   / 255.0,
            guac_client_data->background.green / 255.0,
            guac_client_data->background.blue  / 255.0);

    cairo_rectangle(guac_client_data->glyph_cairo,
            0, 0, width, height);

    cairo_fill(guac_client_data->glyph_cairo);

    /* Prepare for glyph drawing */
    cairo_set_source_rgb(guac_client_data->glyph_cairo,
            guac_client_data->foreground.red   / 255.0,
            guac_client_data->foreground.green / 255.0,
            guac_client_data->foreground.blue  / 255.0);

}

void guac_rdp_glyph_enddraw(rdpContext* context,
        int x, int y, int width, int height, uint32 fgcolor, uint32 bgcolor) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;
    const guac_layer* current_layer = ((rdp_guac_client_data*) client->data)->current_surface;

    /* Send surface with all glyphs to layer */
    guac_protocol_send_png(client->socket,
            GUAC_COMP_OVER, current_layer, x, y,
            guac_client_data->glyph_surface);

    /* Clean up cairo and glyph surface */
    cairo_destroy(guac_client_data->glyph_cairo);
    cairo_surface_destroy(guac_client_data->glyph_surface);

}

