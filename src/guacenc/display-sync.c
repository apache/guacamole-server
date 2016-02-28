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
#include "layer.h"
#include "log.h"

#include <cairo/cairo.h>
#include <guacamole/client.h>
#include <guacamole/timestamp.h>

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

int guacenc_display_sync(guacenc_display* display, guac_timestamp timestamp) {

    /* Verify timestamp is not decreasing */
    if (timestamp < display->last_sync) {
        guacenc_log(GUAC_LOG_WARNING, "Decreasing sync timestamp");
        return 1;
    }

    /* Update timestamp of display */
    display->last_sync = timestamp;

    /* Flatten display to default layer */
    if (guacenc_display_flatten(display))
        return 1;

    /* Retrieve default layer (guaranteed to not be NULL) */
    guacenc_layer* def_layer = guacenc_display_get_layer(display, 0);
    assert(def_layer != NULL);

    /* STUB: Write frame as PNG */
    char filename[256];
    sprintf(filename, "frame-%" PRId64 ".png", timestamp);
    cairo_surface_t* surface = def_layer->frame->surface;
    if (surface != NULL)
        cairo_surface_write_to_png(surface, filename);

    return 0;

}

