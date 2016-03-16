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
#include "image-stream.h"
#include "log.h"

#include <guacamole/client.h>

#include <stdlib.h>

int guacenc_display_create_image_stream(guacenc_display* display, int index,
        int mask, int layer_index, const char* mimetype, int x, int y) {

    /* Do not lookup / allocate if index is invalid */
    if (index < 0 || index > GUACENC_DISPLAY_MAX_STREAMS) {
        guacenc_log(GUAC_LOG_WARNING, "Stream index out of bounds: %i", index);
        return 1;
    }

    /* Free existing stream (if any) */
    guacenc_image_stream_free(display->image_streams[index]);

    /* Associate new stream */
    guacenc_image_stream* stream = display->image_streams[index] =
        guacenc_image_stream_alloc(mask, layer_index, mimetype, x, y);

    /* Return zero only if stream is not NULL */
    return stream == NULL;

}

guacenc_image_stream* guacenc_display_get_image_stream(
        guacenc_display* display, int index) {

    /* Do not lookup / allocate if index is invalid */
    if (index < 0 || index > GUACENC_DISPLAY_MAX_STREAMS) {
        guacenc_log(GUAC_LOG_WARNING, "Stream index out of bounds: %i", index);
        return NULL;
    }

    /* Return existing stream (if any) */
    return display->image_streams[index];

}

int guacenc_display_free_image_stream(guacenc_display* display, int index) {

    /* Do not lookup / allocate if index is invalid */
    if (index < 0 || index > GUACENC_DISPLAY_MAX_STREAMS) {
        guacenc_log(GUAC_LOG_WARNING, "Stream index out of bounds: %i", index);
        return 1;
    }

    /* Free stream (if allocated) */
    guacenc_image_stream_free(display->image_streams[index]);

    /* Mark stream as freed */
    display->image_streams[index] = NULL;

    return 0;

}

