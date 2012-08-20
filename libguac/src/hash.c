
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


#include <stdint.h>
#include <cairo/cairo.h>

/*
 * Arbitrary hash function whhich maps ALL 32-bit numbers onto 24-bit numbers
 * evenly, while guaranteeing that all 24-bit numbers are mapped onto
 * themselves.
 */
int _guac_hash_32to24(int value) {

    /* Grab highest-order byte */
    int upper = value & 0xFF000000;

    /* XOR upper with lower three bytes, truncate to 24-bit */
    return
          (value & 0xFFFFFF)
        ^ (upper >> 8)
        ^ (upper >> 16)
        ^ (upper >> 24);

}

/**
 * Rotates a given 32-bit integer by N bits.
 *
 * NOTE: We probably should check for available bitops.h macros first.
 */
unsigned int _guac_rotate(unsigned int value, int amount) {

    /* amount = amount % 32 */
    amount &= 0x1F; 

    /* Return rotated amount */
    return (value >> amount) | (value << (32 - amount));

}

int guac_hash_surface(cairo_surface_t* surface) {

    /* Init to zero */
    int hash_value = 0;

    int x, y;

    /* Get image data and metrics */
    unsigned char* data = cairo_image_surface_get_data(surface);
    int width = cairo_image_surface_get_width(surface);
    int height = cairo_image_surface_get_height(surface);
    int stride = cairo_image_surface_get_stride(surface);

    for (y=0; y<height; y++) {

        /* Get current row */
        uint32_t* row = (uint32_t*) data;
        data += stride;

        for (x=0; x<width; x++) {

            /* Get color at current pixel */
            int color = *row;
            row++;

            /* Compute next hash */
            hash_value =
                _guac_rotate(hash_value, 1) ^ _guac_hash_32to24(color);

        }

    } /* end for each row */

    /* Done */
    return hash_value;

}

