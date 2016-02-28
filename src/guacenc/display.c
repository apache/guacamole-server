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

#include <cairo/cairo.h>

#include <stdlib.h>

cairo_operator_t guacenc_display_cairo_operator(guac_composite_mode mask) {

    /* Translate Guacamole channel mask into Cairo operator */
    switch (mask) {

        /* Source */
        case GUAC_COMP_SRC:
            return CAIRO_OPERATOR_SOURCE;

        /* Over */
        case GUAC_COMP_OVER:
            return CAIRO_OPERATOR_OVER;

        /* In */
        case GUAC_COMP_IN:
            return CAIRO_OPERATOR_IN;

        /* Out */
        case GUAC_COMP_OUT:
            return CAIRO_OPERATOR_OUT;

        /* Atop */
        case GUAC_COMP_ATOP:
            return CAIRO_OPERATOR_ATOP;

        /* Over (source/destination reversed) */
        case GUAC_COMP_ROVER:
            return CAIRO_OPERATOR_DEST_OVER;

        /* In (source/destination reversed) */
        case GUAC_COMP_RIN:
            return CAIRO_OPERATOR_DEST_IN;

        /* Out (source/destination reversed) */
        case GUAC_COMP_ROUT:
            return CAIRO_OPERATOR_DEST_OUT;

        /* Atop (source/destination reversed) */
        case GUAC_COMP_RATOP:
            return CAIRO_OPERATOR_DEST_ATOP;

        /* XOR */
        case GUAC_COMP_XOR:
            return CAIRO_OPERATOR_XOR;

        /* Additive */
        case GUAC_COMP_PLUS:
            return CAIRO_OPERATOR_ADD;

        /* If unrecognized, just default to CAIRO_OPERATOR_OVER */
        default:
            return CAIRO_OPERATOR_OVER;

    }

}

guacenc_display* guacenc_display_alloc() {
    return (guacenc_display*) calloc(1, sizeof(guacenc_display));
}

int guacenc_display_free(guacenc_display* display) {

    int i;

    /* Ignore NULL display */
    if (display == NULL)
        return 0;

    /* Free all buffers */
    for (i = 0; i < GUACENC_DISPLAY_MAX_BUFFERS; i++)
        guacenc_buffer_free(display->buffers[i]);

    /* Free all layers */
    for (i = 0; i < GUACENC_DISPLAY_MAX_LAYERS; i++)
        guacenc_layer_free(display->layers[i]);

    /* Free all streams */
    for (i = 0; i < GUACENC_DISPLAY_MAX_STREAMS; i++)
        guacenc_image_stream_free(display->image_streams[i]);

    free(display);
    return 0;

}

