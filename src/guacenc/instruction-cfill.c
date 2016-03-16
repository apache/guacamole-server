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

int guacenc_handle_cfill(guacenc_display* display, int argc, char** argv) {
 
    /* Verify argument count */
    if (argc < 6) {
        guacenc_log(GUAC_LOG_WARNING, "\"cfill\" instruction incomplete");
        return 1;
    }

    /* Parse arguments */
    int mask = atoi(argv[0]);
    int index = atoi(argv[1]);
    double r = atoi(argv[2]) / 255.0;
    double g = atoi(argv[3]) / 255.0;
    double b = atoi(argv[4]) / 255.0;
    double a = atoi(argv[5]) / 255.0;

    /* Pull buffer of requested layer/buffer */
    guacenc_buffer* buffer = guacenc_display_get_related_buffer(display, index);
    if (buffer == NULL)
        return 1;

    /* Fill with RGBA color */
    if (buffer->cairo != NULL) {
        cairo_set_operator(buffer->cairo, guacenc_display_cairo_operator(mask));
        cairo_set_source_rgba(buffer->cairo, r, g, b, a);
        cairo_fill(buffer->cairo);
    }

    return 0;

}

