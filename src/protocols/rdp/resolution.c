/*
 * Copyright (C) 2014 Glyptodon LLC
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

#include "client.h"
#include "resolution.h"

#include <guacamole/user.h>

int guac_rdp_resolution_reasonable(guac_user* user, int resolution) {

    int width  = user->info.optimal_width;
    int height = user->info.optimal_height;

    /* Convert user pixels to remote pixels */
    width  = width  * resolution / user->info.optimal_resolution;
    height = height * resolution / user->info.optimal_resolution;

    /*
     * Resolution is reasonable if the same as the user optimal resolution
     * OR if the resulting display area is reasonable
     */
    return user->info.optimal_resolution == resolution
        || width*height >= GUAC_RDP_REASONABLE_AREA;

}

int guac_rdp_suggest_resolution(guac_user* user) {

    /* Prefer RDP's native resolution */
    if (guac_rdp_resolution_reasonable(user, GUAC_RDP_NATIVE_RESOLUTION))
        return GUAC_RDP_NATIVE_RESOLUTION;

    /* If native resolution is too tiny, try higher resolution */
    if (guac_rdp_resolution_reasonable(user, GUAC_RDP_HIGH_RESOLUTION))
        return GUAC_RDP_HIGH_RESOLUTION;

    /* Fallback to user-suggested resolution */
    return user->info.optimal_resolution;

}

