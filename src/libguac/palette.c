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

#include "config.h"

#include "palette.h"

#include <cairo/cairo.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

guac_palette* guac_palette_alloc(cairo_surface_t* surface) {

    int x, y;

    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    int stride = cairo_image_surface_get_stride(surface);
    unsigned char* data = cairo_image_surface_get_data(surface);

    /* Allocate palette */
    guac_palette* palette = (guac_palette*) malloc(sizeof(guac_palette));
    memset(palette, 0, sizeof(guac_palette));

    for (y=0; y<height; y++) {
        for (x=0; x<width; x++) {

            /* Get pixel color */
            int color = ((uint32_t*) data)[x] & 0xFFFFFF;

            /* Calculate hash code */
            int hash = ((color & 0xFFF000) >> 12) ^ (color & 0xFFF);

            guac_palette_entry* entry;

            /* Search for open palette entry */
            for (;;) {
                
                entry = &(palette->entries[hash]);

                /* If we've found a free space, use it */
                if (entry->index == 0) {

                    png_color* c;

                    /* Stop if already at capacity */
                    if (palette->size == 256) {
                        guac_palette_free(palette);
                        return NULL;
                    }

                    /* Store in palette */
                    c = &(palette->colors[palette->size]);
                    c->blue  = (color      ) & 0xFF;
                    c->green = (color >> 8 ) & 0xFF;
                    c->red   = (color >> 16) & 0xFF;

                    /* Add color to map */
                    entry->index = ++palette->size;
                    entry->color = color;

                    break;

                }

                /* Otherwise, if already stored here, done */
                if (entry->color == color)
                    break;

                /* Otherwise, collision. Move on to another bucket */
                hash = (hash+1) & 0xFFF;

            }
        }

        /* Advance to next data row */
        data += stride;

    }

    return palette;

}

int guac_palette_find(guac_palette* palette, int color) {

    /* Calculate hash code */
    int hash = ((color & 0xFFF000) >> 12) ^ (color & 0xFFF);

    guac_palette_entry* entry;

    /* Search for palette entry */
    for (;;) {
        
        entry = &(palette->entries[hash]);

        /* If we've found a free space, color not stored. */
        if (entry->index == 0)
            return -1;

        /* Otherwise, if color indeed stored here, done */
        if (entry->color == color)
            return entry->index - 1;

        /* Otherwise, collision. Move on to another bucket */
        hash = (hash+1) & 0xFFF;

    }

}

void guac_palette_free(guac_palette* palette) {
    free(palette);
}

