/*
 * Copyright (C) 2016 Glyptodon, Inc.
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

#include "config.h"
#include "log.h"
#include "webp.h"

#include <cairo/cairo.h>
#include <guacamole/client.h>
#include <webp/decode.h>

#include <stdint.h>
#include <stdlib.h>

cairo_surface_t* guacenc_webp_decoder(unsigned char* data, int length) {

    int width, height;

    /* Validate WebP and pull dimensions */
    if (!WebPGetInfo((uint8_t*) data, length, &width, &height)) {
        guacenc_log(GUAC_LOG_WARNING, "Invalid WebP data");
        return NULL;
    }

    /* Create blank Cairo surface */
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
            width, height);

    /* Fill surface with opaque black */
    cairo_t* cairo = cairo_create(surface);
    cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_rgba(cairo, 0.0, 0.0, 0.0, 1.0);
    cairo_paint(cairo);
    cairo_destroy(cairo);

    /* Finish any pending draws */
    cairo_surface_flush(surface);

    /* Pull underlying buffer and its stride */
    int stride = cairo_image_surface_get_stride(surface);
    unsigned char* image = cairo_image_surface_get_data(surface);

    /* Read WebP into surface */
    uint8_t* result = WebPDecodeBGRAInto((uint8_t*) data, length,
            (uint8_t*) image, stride * height, stride);

    /* Verify WebP was successfully decoded */
    if (result == NULL) {
        guacenc_log(GUAC_LOG_WARNING, "Invalid WebP data");
        cairo_surface_destroy(surface);
        return NULL;
    }

    /* WebP was read successfully */
    return surface;

}

