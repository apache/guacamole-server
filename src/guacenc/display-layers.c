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

#include <guacamole/client.h>

#include <stdlib.h>

guacenc_layer* guacenc_display_get_layer(guacenc_display* display,
        int index) {

    /* Do not lookup / allocate if index is invalid */
    if (index < 0 || index > GUACENC_DISPLAY_MAX_LAYERS) {
        guacenc_log(GUAC_LOG_WARNING, "Layer index out of bounds: %i", index);
        return NULL;
    }

    /* Lookup layer, allocating a new layer if necessary */
    guacenc_layer* layer = display->layers[index];
    if (layer == NULL) {

        /* Attempt to allocate layer */
        layer = guacenc_layer_alloc();
        if (layer == NULL) {
            guacenc_log(GUAC_LOG_WARNING, "Layer allocation failed");
            return NULL;
        }

        /* The default layer has no parent */
        if (index == 0)
            layer->parent_index = GUACENC_LAYER_NO_PARENT;

        /* Store layer within display for future retrieval / management */
        display->layers[index] = layer;

    }

    return layer;

}

int guacenc_display_get_depth(guacenc_display* display, guacenc_layer* layer) {

    /* Non-existent layers have a depth of 0 */
    if (layer == NULL)
        return 0;

    /* Layers with no parent have a depth of 0 */
    if (layer->parent_index == GUACENC_LAYER_NO_PARENT)
        return 0;

    /* Retrieve parent layer */
    guacenc_layer* parent =
        guacenc_display_get_layer(display, layer->parent_index);

    /* Current layer depth is the depth of the parent + 1 */
    return guacenc_display_get_depth(display, parent) + 1;

}

int guacenc_display_free_layer(guacenc_display* display,
        int index) {

    /* Do not lookup / allocate if index is invalid */
    if (index < 0 || index > GUACENC_DISPLAY_MAX_LAYERS) {
        guacenc_log(GUAC_LOG_WARNING, "Layer index out of bounds: %i", index);
        return 1;
    }

    /* Free layer (if allocated) */
    guacenc_layer_free(display->layers[index]);

    /* Mark layer as freed */
    display->layers[index] = NULL;

    return 0;

}

