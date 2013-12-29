/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#include <pthread.h>
#include <freerdp/freerdp.h>

#ifdef ENABLE_WINPR
#include <winpr/wtypes.h>
#else
#include "compat/winpr-wtypes.h"
#endif

#include <guacamole/client.h>
#include <guacamole/error.h>

#include "client.h"
#include "rdp_glyph.h"

/* Define cairo_format_stride_for_width() if missing */
#ifndef HAVE_CAIRO_FORMAT_STRIDE_FOR_WIDTH
#define cairo_format_stride_for_width(format, width) (width*4)
#endif

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
        int x, int y, int width, int height, UINT32 fgcolor, UINT32 bgcolor) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    rdp_guac_client_data* guac_client_data =
        (rdp_guac_client_data*) client->data;

    /* Convert foreground color */
    fgcolor = freerdp_color_convert_var(fgcolor,
            guac_client_data->settings.color_depth, 32,
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
                guac_client_data->settings.color_depth, 32,
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
        int x, int y, int width, int height, UINT32 fgcolor, UINT32 bgcolor) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;
    const guac_layer* current_layer = ((rdp_guac_client_data*) client->data)->current_surface;

    /* Use glyph surface to provide image data for glyph rectangle */
    cairo_surface_t* glyph_surface = guac_client_data->glyph_surface;
    int stride = cairo_image_surface_get_stride(glyph_surface);

    /* Calculate bounds */
    int max_width = cairo_image_surface_get_width(glyph_surface) - x;
    int max_height = cairo_image_surface_get_height(glyph_surface) - y;

    /* Ensure dimensions of glyph do not exceed bounds */
    if (width > max_width) width = max_width;
    if (height > max_height) height = max_height;

    /* Clip operation to clipping region, if any */
    if (!guac_rdp_clip_rect(guac_client_data, &x, &y, &width, &height)) {

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

    }

    /* Destroy cairo instance */
    cairo_destroy(guac_client_data->glyph_cairo);

}

