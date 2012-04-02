
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
 * The Original Code is libguac.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2010
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
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

#include <cairo/cairo.h>

#include <sys/types.h>

#include "palette.h"

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
                    c->blue  = (color & 0x0000FF);
                    c->green = (color & 0x00FF00) >> 8;
                    c->red   = (color & 0xFF0000) >> 16;

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
            return entry->index;

        /* Otherwise, collision. Move on to another bucket */
        hash = (hash+1) & 0xFFF;

    }

}

void guac_palette_free(guac_palette* palette) {
    free(palette);
}

