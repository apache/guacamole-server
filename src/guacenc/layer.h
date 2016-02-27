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

#ifndef GUACENC_LAYER_H
#define GUACENC_LAYER_H

#include "config.h"
#include "buffer.h"

/**
 * The value assigned to the parent_index property of a guacenc_layer if it has
 * no parent.
 */
#define GUACENC_LAYER_NO_PARENT -1

/**
 * A visible Guacamole layer.
 */
typedef struct guacenc_layer {

    /**
     * The actual image contents of this layer, as well as this layer's size
     * (width and height).
     */
    guacenc_buffer buffer;

    /**
     * The index of the layer that contains this layer. If this layer is the
     * default layer (and thus has no parent), this will be
     * GUACENC_LAYER_NO_PARENT.
     */
    int parent_index;

    /**
     * The X coordinate of the upper-left corner of this layer within the
     * Guacamole display.
     */
    int x;

    /**
     * The Y coordinate of the upper-left corner of this layer within the
     * Guacamole display.
     */
    int y;

    /**
     * The relative stacking order of this layer with respect to other sibling
     * layers.
     */
    int z;

    /**
     * The opacity of this layer, where 0 is completely transparent and 255 is
     * completely opaque.
     */
    int opacity;

} guacenc_layer;

#endif

