
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
#include <guacamole/error.h>

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

    /* Do not attempt to draw glyphs if glyph drawing is not begun */
    if (guac_client_data->glyph_cairo == NULL)
        return;

    /* Use glyph as mask */
    cairo_mask_surface(
            guac_client_data->glyph_cairo,
            ((guac_rdp_glyph*) glyph)->surface, x, y);

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
    rdp_guac_client_data* guac_client_data =
        (rdp_guac_client_data*) client->data;

    /* Convert foreground color */
    fgcolor = freerdp_color_convert_var(fgcolor,
            context->instance->settings->color_depth, 32,
            ((rdp_freerdp_context*) context)->clrconv);

    /* Fill background with color if specified */
    if (width != 0 && height != 0) {

        /* Prepare for opaque glyphs */
        guac_client_data->glyph_surface = 
            guac_client_data->opaque_glyph_surface;

        /* Create cairo instance */
        guac_client_data->glyph_cairo = cairo_create(
            guac_client_data->glyph_surface);

        /* Convert background color */
        bgcolor = freerdp_color_convert_var(bgcolor,
                context->instance->settings->color_depth, 32,
                ((rdp_freerdp_context*) context)->clrconv);

        /* Fill background */
        cairo_rectangle(guac_client_data->glyph_cairo,
                x, y, width, height);

        cairo_set_source_rgb(guac_client_data->glyph_cairo,
                ((bgcolor & 0xFF0000) >> 16) / 255.0,
                ((bgcolor & 0x00FF00) >> 8 ) / 255.0,
                ( bgcolor & 0x0000FF       ) / 255.0);

        cairo_fill(guac_client_data->glyph_cairo);

    }

    /* Otherwise, prepare for transparent glyphs  */
    else {

        /* Select transparent glyph surface */
        guac_client_data->glyph_surface = 
            guac_client_data->trans_glyph_surface;

        guac_client_data->glyph_cairo = cairo_create(
            guac_client_data->glyph_surface);

        /* Clear surface */
        cairo_set_operator(guac_client_data->glyph_cairo,
            CAIRO_OPERATOR_SOURCE);

        cairo_set_source_rgba(guac_client_data->glyph_cairo, 0, 0, 0, 0);
        cairo_paint(guac_client_data->glyph_cairo);

        /* Restore operator */
        cairo_set_operator(guac_client_data->glyph_cairo,
            CAIRO_OPERATOR_OVER);

    }

    /* Prepare for glyph drawing */
    cairo_set_source_rgb(guac_client_data->glyph_cairo,
            ((fgcolor & 0xFF0000) >> 16) / 255.0,
            ((fgcolor & 0x00FF00) >> 8 ) / 255.0,
            ( fgcolor & 0x0000FF       ) / 255.0);

}

void guac_rdp_glyph_enddraw(rdpContext* context,
        int x, int y, int width, int height, uint32 fgcolor, uint32 bgcolor) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;
    const guac_layer* current_layer = ((rdp_guac_client_data*) client->data)->current_surface;

    /* Use glyph surface to provide image data for glyph rectangle */
    cairo_surface_t* glyph_surface = guac_client_data->glyph_surface;
    int stride = cairo_image_surface_get_stride(glyph_surface);

    /* Ensure data is ready */
    cairo_surface_flush(glyph_surface);

    /* Create surface for subsection with text */
    cairo_surface_t* surface = cairo_image_surface_create_for_data(
            cairo_image_surface_get_data(glyph_surface) + 4*x + y*stride,
            cairo_image_surface_get_format(glyph_surface),
            width, height, stride);

    /* Send surface with all glyphs to layer */
    guac_protocol_send_png(client->socket,
            GUAC_COMP_OVER, current_layer, x, y,
            surface);

    /* Destroy surface */
    cairo_surface_destroy(surface);

    /* Destroy cairo instance */
    cairo_destroy(guac_client_data->glyph_cairo);

}

