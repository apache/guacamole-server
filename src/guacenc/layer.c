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
#include "buffer.h"
#include "layer.h"

#include <stdlib.h>

guacenc_layer* guacenc_layer_alloc() {

    /* Allocate new layer */
    guacenc_layer* layer = (guacenc_layer*) calloc(1, sizeof(guacenc_layer));
    if (layer == NULL)
        return NULL;

    /* Allocate associated buffer (width, height, and image storage) */
    layer->buffer = guacenc_buffer_alloc();
    if (layer->buffer == NULL) {
        free(layer);
        return NULL;
    }

    /* Layers default to fully opaque */
    layer->opacity = 0xFF;

    /* Default to unparented */
    layer->parent_index = GUACENC_LAYER_NO_PARENT;

    return layer;

}

void guacenc_layer_free(guacenc_layer* layer) {

    /* Ignore NULL layers */
    if (layer == NULL)
        return;

    /* Free underlying buffer */
    guacenc_buffer_free(layer->buffer);

    free(layer);

}

