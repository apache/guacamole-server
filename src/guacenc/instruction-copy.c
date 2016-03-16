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
#include "display.h"
#include "log.h"

#include <guacamole/client.h>

#include <stdlib.h>

int guacenc_handle_copy(guacenc_display* display, int argc, char** argv) {

    /* Verify argument count */
    if (argc < 9) {
        guacenc_log(GUAC_LOG_WARNING, "\"copy\" instruction incomplete");
        return 1;
    }

    /* Parse arguments */
    int sindex = atoi(argv[0]);
    int sx = atoi(argv[1]);
    int sy = atoi(argv[2]);
    int width = atoi(argv[3]);
    int height = atoi(argv[4]);
    int mask = atoi(argv[5]);
    int dindex = atoi(argv[6]);
    int dx = atoi(argv[7]);
    int dy = atoi(argv[8]);

    /* Pull buffer of source layer/buffer */
    guacenc_buffer* src = guacenc_display_get_related_buffer(display, sindex);
    if (src == NULL)
        return 1;

    /* Pull buffer of destination layer/buffer */
    guacenc_buffer* dst = guacenc_display_get_related_buffer(display, dindex);
    if (dst == NULL)
        return 1;

    /* Expand the destination buffer as necessary to fit the draw operation */
    if (dst->autosize)
        guacenc_buffer_fit(dst, dx + width, dy + height);

    /* Copy rectangle from source to destination */
    if (src->surface != NULL && dst->cairo != NULL) {

        /* If surfaces are different, no need to copy */
        cairo_surface_t* surface;
        if (src != dst)
            surface = src->surface;

        /* Otherwise, copy to a temporary surface */
        else {

            /* Create new surface to hold the source rect */
            surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32,
                    width, height);

            /* Copy relevant rectangle from source surface */
            cairo_t* cairo = cairo_create(surface);
            cairo_set_operator(cairo, CAIRO_OPERATOR_SOURCE);
            cairo_set_source_surface(cairo, src->surface, -sx, -sy);
            cairo_paint(cairo);
            cairo_destroy(cairo);

            /* Source coordinates are now (0, 0) */
            sx = sy = 0;

        }

        /* Perform copy */
        cairo_set_operator(dst->cairo, guacenc_display_cairo_operator(mask));
        cairo_set_source_surface(dst->cairo, surface, dx - sx, dy - sy);
        cairo_rectangle(dst->cairo, dx, dy, width, height);
        cairo_fill(dst->cairo);

        /* Destroy temporary surface if it was created */
        if (surface != src->surface)
            cairo_surface_destroy(surface);

    }

    return 0;

}

