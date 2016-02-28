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

#include <cairo/cairo.h>

#include <stdlib.h>
#include <string.h>

/**
 * The Guacamole video encoder display related to the current qsort()
 * operation. As qsort() does not provide a means of passing arbitrary data to
 * the comparitor, this value must be set prior to invoking qsort() with
 * guacenc_display_layer_comparator.
 */
guacenc_display* __qsort_display;

/**
 * Comparator which orders layer pointers such that (1) NULL pointers are last,
 * (2) layers with the same parent_index are adjacent, and (3) layers with the
 * same parent_index are ordered by Z.
 *
 * @see qsort()
 */
static int guacenc_display_layer_comparator(const void* a, const void* b) {

    guacenc_layer* layer_a = *((guacenc_layer**) a);
    guacenc_layer* layer_b = *((guacenc_layer**) b);

    /* If a is NULL, sort it to bottom */
    if (layer_a == NULL) {

        /* ... unless b is also NULL, in which case they are equal */
        if (layer_b == NULL)
            return 0;

        return 1;
    }

    /* If b is NULL (and a is not NULL), sort it to bottom */
    if (layer_b == NULL)
        return -1;

    /* Order such that the deepest layers are first */
    int a_depth = guacenc_display_get_depth(__qsort_display, layer_a);
    int b_depth = guacenc_display_get_depth(__qsort_display, layer_b);
    if (b_depth != a_depth)
        return b_depth - a_depth;

    /* Order such that sibling layers are adjacent */
    if (layer_b->parent_index != layer_a->parent_index)
        return layer_b->parent_index - layer_a->parent_index;

    /* Order sibling layers according to descending Z */
    return layer_b->z - layer_a->z;

}

int guacenc_display_flatten(guacenc_display* display) {

    int i;
    guacenc_layer* render_order[GUACENC_DISPLAY_MAX_LAYERS];

    /* Copy list of layers within display */
    memcpy(render_order, display->layers, sizeof(render_order));

    /* Sort layers by depth, parent, and Z */
    __qsort_display = display;
    qsort(render_order, GUACENC_DISPLAY_MAX_LAYERS, sizeof(guacenc_layer*),
            guacenc_display_layer_comparator);

    /* Reset layer frame buffers */
    for (i = 0; i < GUACENC_DISPLAY_MAX_LAYERS; i++) {

        /* Pull current layer, ignoring unallocated layers */
        guacenc_layer* layer = render_order[i];
        if (layer == NULL)
            continue;

        /* Get source buffer and destination frame buffer */
        guacenc_buffer* buffer = layer->buffer;
        guacenc_buffer* frame = layer->frame;

        /* Reset frame contents */
        guacenc_buffer_copy(frame, buffer);

    }

    /* Render each layer, in order */
    for (i = 0; i < GUACENC_DISPLAY_MAX_LAYERS; i++) {

        /* Pull current layer, ignoring unallocated layers */
        guacenc_layer* layer = render_order[i];
        if (layer == NULL)
            continue;

        /* Skip fully-transparent layers */
        if (layer->opacity == 0)
            continue;

        /* Ignore layers without a parent */
        int parent_index = layer->parent_index;
        if (parent_index == GUACENC_LAYER_NO_PARENT)
            continue;

        /* Retrieve parent layer, ignoring layers with invalid parents */
        guacenc_layer* parent = guacenc_display_get_layer(display, parent_index);
        if (parent == NULL)
            continue;

        /* Get source and destination frame buffer */
        guacenc_buffer* src = layer->frame;
        guacenc_buffer* dst = parent->frame;

        /* Ignore layers with empty buffers */
        cairo_surface_t* surface = src->surface;
        if (surface == NULL)
            continue;

        /* Ignore if parent has no pixels */
        cairo_t* cairo = dst->cairo;
        if (cairo == NULL)
            continue;

        /* Render buffer to layer */
        cairo_reset_clip(cairo);
        cairo_rectangle(cairo, layer->x, layer->y, src->width, src->height);
        cairo_clip(cairo);

        cairo_set_source_surface(cairo, surface, layer->x, layer->y);
        cairo_paint_with_alpha(cairo, layer->opacity / 255.0);

    }

    return 0;

}

