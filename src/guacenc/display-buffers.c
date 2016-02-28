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
#include "buffer.h"
#include "layer.h"
#include "log.h"

#include <guacamole/client.h>

#include <stdlib.h>

guacenc_buffer* guacenc_display_get_buffer(guacenc_display* display,
        int index) {

    /* Transform index to buffer space */
    int internal_index = -index - 1;

    /* Do not lookup / allocate if index is invalid */
    if (internal_index < 0 || internal_index > GUACENC_DISPLAY_MAX_BUFFERS) {
        guacenc_log(GUAC_LOG_WARNING, "Buffer index out of bounds: %i", index);
        return NULL;
    }

    /* Lookup buffer, allocating a new buffer if necessary */
    guacenc_buffer* buffer = display->buffers[internal_index];
    if (buffer == NULL) {

        /* Attempt to allocate buffer */
        buffer = guacenc_buffer_alloc();
        if (buffer == NULL) {
            guacenc_log(GUAC_LOG_WARNING, "Buffer allocation failed");
            return NULL;
        }

        /* All non-layer buffers must autosize */
        buffer->autosize = true;

        /* Store buffer within display for future retrieval / management */
        display->buffers[internal_index] = buffer;

    }

    return buffer;

}

int guacenc_display_free_buffer(guacenc_display* display,
        int index) {

    /* Transform index to buffer space */
    int internal_index = -index - 1;

    /* Do not lookup / allocate if index is invalid */
    if (internal_index < 0 || internal_index > GUACENC_DISPLAY_MAX_BUFFERS) {
        guacenc_log(GUAC_LOG_WARNING, "Buffer index out of bounds: %i", index);
        return 1;
    }

    /* Free buffer (if allocated) */
    guacenc_buffer_free(display->buffers[internal_index]);

    /* Mark buffer as freed */
    display->buffers[internal_index] = NULL;

    return 0;

}

guacenc_buffer* guacenc_display_get_related_buffer(guacenc_display* display,
        int index) {

    /* Retrieve underlying buffer of layer if a layer is requested */
    if (index >= 0) {

        /* Retrieve / allocate layer (if possible */
        guacenc_layer* layer = guacenc_display_get_layer(display, index);
        if (layer == NULL)
            return NULL;

        /* Return underlying buffer */
        return layer->buffer;

    }

    /* Otherwise retrieve buffer directly */
    return guacenc_display_get_buffer(display, index);

}

