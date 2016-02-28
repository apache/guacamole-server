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
#include <guacamole/protocol.h>
#include <guacamole/timestamp.h>

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
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

int guacenc_display_sync(guacenc_display* display, guac_timestamp timestamp) {

    /* Verify timestamp is not decreasing */
    if (timestamp < display->last_sync) {
        guacenc_log(GUAC_LOG_WARNING, "Decreasing sync timestamp");
        return 1;
    }

    /* Get buffer for display frame rendering */
    guacenc_buffer* frame = display->frame;

    /* Update timestamp of display */
    display->last_sync = timestamp;

    /* Ensure frame is the same size as the default layer */
    guacenc_buffer* def_layer = guacenc_display_get_related_buffer(display, 0);
    guacenc_buffer_resize(frame, def_layer->width, def_layer->height);

    /* Render all layers to frame */
    cairo_t* cairo = frame->cairo;
    if (cairo != NULL) {

        int i;
        guacenc_layer* render_order[GUACENC_DISPLAY_MAX_LAYERS];

        /* Copy list of layers within display */
        memcpy(render_order, display->layers, sizeof(render_order));

        /* Sort layers by depth, parent, and Z */
        __qsort_display = display;
        qsort(render_order, GUACENC_DISPLAY_MAX_LAYERS, sizeof(guacenc_layer*),
                guacenc_display_layer_comparator);

        /* Render each layer, in order */
        for (i = 0; i < GUACENC_DISPLAY_MAX_LAYERS; i++) {

            /* Pull current layer, ignoring unallocated layers */
            guacenc_layer* layer = render_order[i];
            if (layer == NULL)
                continue;

            /* Skip fully-transparent layers */
            if (layer->opacity == 0)
                continue;

            /* Pull underlying buffer */
            guacenc_buffer* buffer = layer->buffer;

            /* Ignore layers with empty buffers */
            if (buffer->surface == NULL)
                continue;

            /* TODO: Determine actual location relative to parent layer */
            int x = layer->x;
            int y = layer->y;

            /* Render buffer to layer */
            cairo_reset_clip(cairo);
            cairo_rectangle(cairo, x, y, buffer->width, buffer->height);
            cairo_clip(cairo);

            cairo_set_source_surface(cairo, buffer->surface, x, y);
            cairo_paint_with_alpha(cairo, layer->opacity / 255.0);

        }

    }

    /* STUB: Write frame as PNG */
    char filename[256];
    sprintf(filename, "frame-%" PRId64 ".png", timestamp);
    cairo_surface_t* surface = frame->surface;
    if (surface != NULL)
        cairo_surface_write_to_png(surface, filename);

    return 0;

}

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

cairo_operator_t guacenc_display_cairo_operator(guac_composite_mode mask) {

    /* Translate Guacamole channel mask into Cairo operator */
    switch (mask) {

        /* Source */
        case GUAC_COMP_SRC:
            return CAIRO_OPERATOR_SOURCE;

        /* Over */
        case GUAC_COMP_OVER:
            return CAIRO_OPERATOR_OVER;

        /* In */
        case GUAC_COMP_IN:
            return CAIRO_OPERATOR_IN;

        /* Out */
        case GUAC_COMP_OUT:
            return CAIRO_OPERATOR_OUT;

        /* Atop */
        case GUAC_COMP_ATOP:
            return CAIRO_OPERATOR_ATOP;

        /* Over (source/destination reversed) */
        case GUAC_COMP_ROVER:
            return CAIRO_OPERATOR_DEST_OVER;

        /* In (source/destination reversed) */
        case GUAC_COMP_RIN:
            return CAIRO_OPERATOR_DEST_IN;

        /* Out (source/destination reversed) */
        case GUAC_COMP_ROUT:
            return CAIRO_OPERATOR_DEST_OUT;

        /* Atop (source/destination reversed) */
        case GUAC_COMP_RATOP:
            return CAIRO_OPERATOR_DEST_ATOP;

        /* XOR */
        case GUAC_COMP_XOR:
            return CAIRO_OPERATOR_XOR;

        /* Additive */
        case GUAC_COMP_PLUS:
            return CAIRO_OPERATOR_ADD;

        /* If unrecognized, just default to CAIRO_OPERATOR_OVER */
        default:
            return CAIRO_OPERATOR_OVER;

    }

}

guacenc_display* guacenc_display_alloc() {

    /* Allocate display */
    guacenc_display* display =
        (guacenc_display*) calloc(1, sizeof(guacenc_display));

    /* Allocate buffer for frame rendering */
    display->frame = guacenc_buffer_alloc();

    return display;

}

int guacenc_display_free(guacenc_display* display) {

    int i;

    /* Ignore NULL display */
    if (display == NULL)
        return 0;

    /* Free internal frame buffer */
    guacenc_buffer_free(display->frame);

    /* Free all buffers */
    for (i = 0; i < GUACENC_DISPLAY_MAX_BUFFERS; i++)
        guacenc_buffer_free(display->buffers[i]);

    /* Free all layers */
    for (i = 0; i < GUACENC_DISPLAY_MAX_LAYERS; i++)
        guacenc_layer_free(display->layers[i]);

    /* Free all streams */
    for (i = 0; i < GUACENC_DISPLAY_MAX_STREAMS; i++)
        guacenc_image_stream_free(display->image_streams[i]);

    free(display);
    return 0;

}

