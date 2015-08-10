/*
 * Copyright (C) 2015 Glyptodon LLC
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

/*
 * Smoothness detection from:
 * QEMU VNC display driver: tight encoding
 *
 * From libvncserver/libvncserver/tight.c
 * Copyright (C) 2000, 2001 Const Kaplinsky.  All Rights Reserved.
 * Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
 *
 * Copyright (C) 2010 Corentin Chary <corentin.chary@gmail.com>
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "guac_surface_smoothness.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/**
 * The threshold to determine an image to be smooth.
 */
#define GUAC_SURFACE_SMOOTHNESS_THRESHOLD 0

/**
 * Width of sub-row when detecting image smoothness.
 */
#define GUAC_SURFACE_SMOOTHNESS_DETECT_SUBROW_WIDTH 7

int guac_common_surface_rect_is_smooth(guac_common_surface* surface,
        guac_common_rect* rect)
{

    /*
     * Code to guess if the image in a given rectangle is smooth
     * (by applying "gradient" filter or JPEG coder).
     */
    int x, y, d, dx;
    unsigned int c;
    unsigned int stats[256];
    int pixels = 0;
    int pix, left[3];
    unsigned char* buffer = surface->buffer;
    int stride = surface->stride;
    int w = rect->x + rect->width;
    int h = rect->y + rect->height;

    /* If rect is out of bounds, bail out */
    if (rect->x < 0 || rect->y < 0 ||
        w > surface->width || h > surface->height) {
        return 0;
    }

    /* If rect is too small to process, bail out */
    if (rect->width < GUAC_SURFACE_SMOOTHNESS_DETECT_SUBROW_WIDTH + 1 ||
        rect->height < GUAC_SURFACE_SMOOTHNESS_DETECT_SUBROW_WIDTH + 1) {
        return 0;
    }

    /* Init stats array */
    memset(stats, 0, sizeof (stats));

    for (y = rect->y, x = rect->x; y < h && x < w;) {

        /* Scan sub-sections of the surface to determine how close the colors are
         * to the previous. */
        for (d = 0;
             d < h - y && d < w - x - GUAC_SURFACE_SMOOTHNESS_DETECT_SUBROW_WIDTH;
             d++) {

            for (c = 0; c < 3; c++) {
                unsigned int index = (y+d)*stride + (x+d)*4 + c;
                left[c] = buffer[index] & 0xFF;
            }

            for (dx = 1; dx <= GUAC_SURFACE_SMOOTHNESS_DETECT_SUBROW_WIDTH; dx++) {

                for (c = 0; c < 3; c++) {
                    unsigned int index = (y+d)*stride + (x+d+dx)*4 + c;
                    pix = buffer[index] & 0xFF;
                    stats[abs(pix - left[c])]++;
                    left[c] = pix;
                }
                ++pixels;
            }
        }

        /* Advance to next section */
        if (w > h) {
            x += h;
            y = rect->y;
        } else {
            x = rect->x;
            y += w;
        }
    }

    if (pixels == 0) {
        return 1;
    }

    /* 95% smooth or more */
    if (stats[0] * 33 / pixels >= 95) {
        return 1;
    }

    unsigned int smoothness = 0;
    for (c = 1; c < 8; c++) {
        smoothness += stats[c] * (c * c);
        if (stats[c] == 0 || stats[c] > stats[c-1] * 2) {
            return 1;
        }
    }
    for (; c < 256; c++) {
        smoothness += stats[c] * (c * c);
    }
    smoothness /= (pixels * 3 - stats[0]);

    return smoothness <= GUAC_SURFACE_SMOOTHNESS_THRESHOLD;
}
