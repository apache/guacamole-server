/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "color.h"
#include "common/surface.h"
#include "config.h"
#include "glyph.h"
#include "rdp.h"

#include <freerdp/freerdp.h>
#include <guacamole/client.h>
#include <winpr/wtypes.h>

#include <stdint.h>
#include <stdlib.h>

/* Define cairo_format_stride_for_width() if missing */
#ifndef HAVE_CAIRO_FORMAT_STRIDE_FOR_WIDTH
#define cairo_format_stride_for_width(format, width) (width*4)
#endif

BOOL guac_rdp_glyph_new(rdpContext* context, const rdpGlyph* glyph) {

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

    return TRUE;

}

BOOL guac_rdp_glyph_draw(rdpContext* context, const rdpGlyph* glyph,
        GLYPH_CALLBACK_INT32 x, GLYPH_CALLBACK_INT32 y,
        GLYPH_CALLBACK_INT32 w, GLYPH_CALLBACK_INT32 h,
        GLYPH_CALLBACK_INT32 sx, GLYPH_CALLBACK_INT32 sy,
        BOOL redundant) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_common_surface* current_surface = rdp_client->current_surface;
    uint32_t fgcolor = rdp_client->glyph_color;

    /* Paint with glyph as mask */
    guac_common_surface_paint(current_surface, x, y, ((guac_rdp_glyph*) glyph)->surface,
                               (fgcolor & 0xFF0000) >> 16,
                               (fgcolor & 0x00FF00) >> 8,
                                fgcolor & 0x0000FF);

    return TRUE;

}

void guac_rdp_glyph_free(rdpContext* context, rdpGlyph* glyph) {

    unsigned char* image_buffer = cairo_image_surface_get_data(
            ((guac_rdp_glyph*) glyph)->surface);

    /* Free surface */
    cairo_surface_destroy(((guac_rdp_glyph*) glyph)->surface);
    free(image_buffer);

    /* NOTE: FreeRDP-allocated memory for the rdpGlyph will NOT be
     * automatically released after this free handler is invoked, thus we must
     * do so manually here */

    free(glyph->aj);
    free(glyph);

}

BOOL guac_rdp_glyph_begindraw(rdpContext* context,
        GLYPH_CALLBACK_INT32 x, GLYPH_CALLBACK_INT32 y,
        GLYPH_CALLBACK_INT32 width, GLYPH_CALLBACK_INT32 height,
        UINT32 fgcolor, UINT32 bgcolor, BOOL redundant) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client =
        (guac_rdp_client*) client->data;

    /* Fill background with color if specified */
    if (width != 0 && height != 0 && !redundant) {

        /* Convert background color */
        bgcolor = guac_rdp_convert_color(context, bgcolor);

        guac_common_surface_set(rdp_client->current_surface,
                x, y, width, height,
                (bgcolor & 0xFF0000) >> 16,
                (bgcolor & 0x00FF00) >> 8,
                (bgcolor & 0x0000FF),
                0xFF);

    }

    /* Convert foreground color */
    rdp_client->glyph_color = guac_rdp_convert_color(context, fgcolor);

    return TRUE;

}

BOOL guac_rdp_glyph_enddraw(rdpContext* context,
        GLYPH_CALLBACK_INT32 x, GLYPH_CALLBACK_INT32 y,
        GLYPH_CALLBACK_INT32 width, GLYPH_CALLBACK_INT32 height,
        UINT32 fgcolor, UINT32 bgcolor) {
    /* IGNORE */
    return TRUE;
}

