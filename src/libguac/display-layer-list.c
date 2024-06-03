/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "display-priv.h"
#include "guacamole/assert.h"
#include "guacamole/client.h"
#include "guacamole/display.h"
#include "guacamole/layer.h"
#include "guacamole/mem.h"
#include "guacamole/rwlock.h"

#include <cairo/cairo.h>
#include <stdlib.h>
#include <string.h>

/**
 * Performs a bulk copy of image data from a source buffer to a destination
 * buffer. The two buffers need not match in size and stride. If the
 * destination is smaller than the desired source, the source dimensions will
 * be adjusted to fit the available space.
 *
 * @param dst
 *     A pointer to the first byte of image data in the destination buffer.
 *
 * @param dst_stride
 *     The number of bytes in each row of image data in the destination buffer.
 *
 * @param dst_width
 *     The width of the destination buffer relative to the provided first byte,
 *     in pixels.
 *
 * @param dst_height
 *     The height of the destination buffer relative to the provided first byte,
 *     in pixels.
 *
 * @param src
 *     A pointer to the first byte of image data in the source buffer.
 *
 * @param src_stride
 *     The number of bytes in each row of image data in the source buffer.
 *
 * @param src_width
 *     The width of the source buffer relative to the provided first byte, in
 *     pixels. If this value is larger than dst_width, it will be adjusted to
 *     fit the available space.
 *
 * @param src_height
 *     The height of the source buffer relative to the provided first byte, in
 *     pixels. If this value is larger than dst_height, it will be adjusted to
 *     fit the available space.
 *
 * @param pixel_size
 *     The size of each pixel of image data, in bytes. The size of each pixel
 *     in both the destination and source buffers must be identical.
 */
static void guac_imgcpy(void* dst, size_t dst_stride, int dst_width, int dst_height,
        void* src, size_t src_stride, int src_width, int src_height,
        size_t pixel_size) {

    int width = dst_width;
    int height = dst_height;

    if (src_width  < width)  width  = src_width;
    if (src_height < height) height = src_height;

    GUAC_ASSERT(width >= 0);
    GUAC_ASSERT(height >= 0);

    size_t length = guac_mem_ckd_mul_or_die(width, pixel_size);

    for (size_t i = 0; i < height; i++) {
        memcpy(dst, src, length);
        dst = ((char*) dst) + dst_stride;
        src = ((char*) src) + src_stride;
    }

}

/**
 * Resizes the layer represented by the given pair of layer states to the given
 * dimensions, allocating a larger underlying image buffer if necessary. If no
 * image buffer has yet been allocated, an image buffer large enough to hold
 * the given dimensions will be automatically allocated.
 *
 * This function DOES NOT resize the pending cells array, which is not stored
 * on the guac_display_layer_state. When resizing a layer, the pending cells
 * array must be separately resized with a call to
 * PFW_guac_display_layer_pending_frame_cells_resize().
 *
 * @param last_frame
 *     The guac_display_layer_state representing the state of the layer at the
 *     end of the last frame sent to connected clients.
 *
 * @param pending_frame
 *     The guac_display_layer_state representing the current pending state of
 *     the layer for the upcoming frame to be eventually sent to connected
 *     clients.
 *
 * @param width
 *     The new width, in pixels.
 *
 * @param height
 *     The new height, in pixels.
 */
static void PFW_LFW_guac_display_layer_buffers_resize(guac_display_layer_state* last_frame,
        guac_display_layer_state* pending_frame, int width, int height) {

    GUAC_ASSERT(last_frame->buffer_width == pending_frame->buffer_width);
    GUAC_ASSERT(last_frame->buffer_height == pending_frame->buffer_height);

    /* Round up to nearest multiple of resize factor */
    width  = ((width  + GUAC_DISPLAY_RESIZE_FACTOR - 1) / GUAC_DISPLAY_RESIZE_FACTOR) * GUAC_DISPLAY_RESIZE_FACTOR;
    height = ((height + GUAC_DISPLAY_RESIZE_FACTOR - 1) / GUAC_DISPLAY_RESIZE_FACTOR) * GUAC_DISPLAY_RESIZE_FACTOR;

    /* Do nothing if size isn't actually changing */
    if (width == last_frame->buffer_width
            && height == last_frame->buffer_height)
        return;

    /* The request to resize applies only to the pending frame, but space for
     * the last frame must be maintained. If either requested dimension is
     * smaller than the last frame dimensions, the relevant dimension of the
     * last frame must be used instead. */

    int new_buffer_width = last_frame->buffer_width;
    if (width > new_buffer_width)
        new_buffer_width = width;

    int new_buffer_height = last_frame->buffer_height;
    if (height > new_buffer_height)
        new_buffer_height = height;

    /* Determine details of shared buffer space sufficient for both the
     * established last frame and the resized pending frame. Allocate new
     * shared buffer space for last and pending frames, interleaving their
     * rows.
     *
     * NOTE: We interleave the rows of the last and pending frames to promote
     * locality of reference. The comparisons performed between last and
     * pending frames to determine what has changed are faster when the rows
     * are interleaved, as data relevant to those comparisons will tend to be
     * present in the CPU cache. */

    int new_last_frame_offset = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, new_buffer_width);
    int new_common_stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, new_last_frame_offset * 2);
    unsigned char* new_buffer_base = guac_mem_zalloc(new_buffer_height, new_common_stride);
    unsigned char* new_pending_frame_buffer = new_buffer_base;
    unsigned char* new_last_frame_buffer = new_buffer_base + new_last_frame_offset;

    /* Copy over data from old shared buffer, if that data exists and is
     * relevant */

    if (last_frame->buffer != NULL && pending_frame->buffer != NULL) {

        guac_imgcpy(

                /* Copy to newly-allocated pending frame buffer ... */
                new_pending_frame_buffer, new_common_stride,
                new_buffer_width, new_buffer_height,

                /* ... from old pending frame buffer. */
                pending_frame->buffer, pending_frame->buffer_stride,
                pending_frame->buffer_width, pending_frame->buffer_height,

                /* All pixels are 32-bit */
                GUAC_DISPLAY_LAYER_RAW_BPP);

        guac_imgcpy(

                /* Copy to newly-allocated last frame buffer ... */
                new_last_frame_buffer, new_common_stride,
                last_frame->buffer_width, last_frame->buffer_height,

                /* ... from old last frame buffer. */
                last_frame->buffer, last_frame->buffer_stride,
                last_frame->buffer_width, last_frame->buffer_height,

                /* All pixels are 32-bit */
                GUAC_DISPLAY_LAYER_RAW_BPP);

    }

    guac_mem_free(pending_frame->buffer);
    last_frame->buffer = new_buffer_base + new_last_frame_offset;
    pending_frame->buffer = new_buffer_base;

    last_frame->buffer_width = pending_frame->buffer_width = new_buffer_width;
    last_frame->buffer_height = pending_frame->buffer_height = new_buffer_height;
    last_frame->buffer_stride = pending_frame->buffer_stride = new_common_stride;

}

/**
 * Fully initializes the last and pending frame states for a newly-allocated
 * layer, including its underlying image buffers.
 *
 * @param last_frame
 *     The guac_display_layer_state representing the state of the layer at the
 *     end of the last frame sent to connected clients.
 *
 * @param pending_frame
 *     The guac_display_layer_state representing the current pending state of
 *     the layer for the upcoming frame to be eventually sent to connected
 *     clients.
 */
static void PFW_LFW_guac_display_layer_state_init(guac_display_layer_state* last_frame,
        guac_display_layer_state* pending_frame) {

    last_frame->width = pending_frame->width = GUAC_DISPLAY_RESIZE_FACTOR;
    last_frame->height = pending_frame->height = GUAC_DISPLAY_RESIZE_FACTOR;
    last_frame->opacity = pending_frame->opacity = 0xFF;
    last_frame->parent = pending_frame->parent = GUAC_DEFAULT_LAYER;

    /* Allocate shared buffer space for last and pending frames, interleaving
     * their rows */

    PFW_LFW_guac_display_layer_buffers_resize(last_frame, pending_frame,
            pending_frame->width, pending_frame->height);

}

/**
 * Resizes the pending_frame_cells array of the given layer to the given
 * dimensions.
 *
 * @param layer
 *     The layer whose pending_frame_cells array should be resized.
 *
 * @param width
 *     The new width, in pixels.
 *
 * @param height
 *     The new height, in pixels.
 */
static void PFW_guac_display_layer_pending_frame_cells_resize(guac_display_layer* layer,
        int width, int height) {

    int new_pending_frame_cells_width = GUAC_DISPLAY_CELL_DIMENSION(width);
    int new_pending_frame_cells_height = GUAC_DISPLAY_CELL_DIMENSION(height);

    /* Do nothing if size isn't actually changing */
    if (new_pending_frame_cells_width == layer->pending_frame_cells_width
            && new_pending_frame_cells_height == layer->pending_frame_cells_height)
        return;

    guac_display_layer_cell* new_pending_frame_cells = guac_mem_zalloc(sizeof(guac_display_layer_cell),
            new_pending_frame_cells_width, new_pending_frame_cells_height);

    /* Copy existing cells over to new memory if present */
    if (layer->pending_frame_cells != NULL) {

        size_t new_stride = guac_mem_ckd_mul_or_die(new_pending_frame_cells_width, sizeof(guac_display_layer_cell));
        size_t old_stride = guac_mem_ckd_mul_or_die(layer->pending_frame_cells_width, sizeof(guac_display_layer_cell));

        guac_imgcpy(

                /* Copy to newly-allocated pending frame cells ... */
                new_pending_frame_cells, new_stride,
                new_pending_frame_cells_width, new_pending_frame_cells_height,

                /* ... from old pending frame cells. */
                layer->pending_frame_cells, old_stride,
                layer->pending_frame_cells_width, layer->pending_frame_cells_height,

                /* All "pixels" are guac_display_layer_cell structures */
                sizeof(guac_display_layer_cell));

    }

    guac_mem_free(layer->pending_frame_cells);
    layer->pending_frame_cells = new_pending_frame_cells;
    layer->pending_frame_cells_width = new_pending_frame_cells_width;
    layer->pending_frame_cells_height = new_pending_frame_cells_height;

}

guac_display_layer* guac_display_add_layer(guac_display* display, guac_layer* layer, int opaque) {

    guac_rwlock_acquire_write_lock(&display->pending_frame.lock);

    /* Init core layer members */
    guac_display_layer* display_layer = guac_mem_zalloc(sizeof(guac_display_layer));
    display_layer->display = display;
    display_layer->layer = layer;
    display_layer->opaque = opaque;

    /* Init tracking of pending and last frames (NOTE: We need not acquire the
     * display-wide last_frame.lock here as this new layer will not actually be
     * part of the last frame layer list until the pending frame is flushed) */
    PFW_LFW_guac_display_layer_state_init(&display_layer->last_frame, &display_layer->pending_frame);
    display_layer->last_frame_buffer = guac_client_alloc_buffer(display->client);
    PFW_guac_display_layer_pending_frame_cells_resize(display_layer,
            display_layer->pending_frame.width,
            display_layer->pending_frame.height);

    /* Insert list element as the new head */
    guac_display_layer* old_head = display->pending_frame.layers;
    display_layer->pending_frame.prev = NULL;
    display_layer->pending_frame.next = old_head;
    display->pending_frame.layers = display_layer;

    /* Update old head to point to new element, if it existed */
    if (old_head != NULL)
        old_head->pending_frame.prev = display_layer;

    guac_rwlock_release_lock(&display->pending_frame.lock);

    return display_layer;

}

void guac_display_remove_layer(guac_display_layer* display_layer) {

    guac_display* display = display_layer->display;

    /*
     * Remove layer from pending frame
     */

    guac_rwlock_acquire_write_lock(&display->pending_frame.lock);

    /* Update previous element, if it exists */
    if (display_layer->pending_frame.prev != NULL)
        display_layer->pending_frame.prev->pending_frame.next = display_layer->pending_frame.next;

    /* If there is no previous element, then this element is the list head.
     * Update the list head accordingly. */
    else {
        GUAC_ASSERT(display->pending_frame.layers == display_layer);
        display->pending_frame.layers = display_layer->pending_frame.next;
    }

    /* Update next element, if it exists */
    if (display_layer->pending_frame.next != NULL)
        display_layer->pending_frame.next->pending_frame.prev = display_layer->pending_frame.prev;

    guac_rwlock_release_lock(&display->pending_frame.lock);

    /*
     * Remove layer from last frame
     */

    guac_rwlock_acquire_write_lock(&display->last_frame.lock);

    /* Update previous element, if it exists */
    if (display_layer->last_frame.prev != NULL)
        display_layer->last_frame.prev->last_frame.next = display_layer->last_frame.next;

    /* If there is no previous element, then this element is the list head.
     * Update the list head accordingly. */
    else {
        GUAC_ASSERT(display->last_frame.layers == display_layer);
        display->last_frame.layers = display_layer->last_frame.next;
    }

    /* Update next element, if it exists */
    if (display_layer->last_frame.next != NULL)
        display_layer->last_frame.next->last_frame.prev = display_layer->last_frame.prev;

    guac_rwlock_release_lock(&display->last_frame.lock);

    /*
     * Layer has now been removed from both pending and last frame lists and
     * can be safely freed
     */

    guac_client* client = display->client;
    guac_client_free_buffer(client, display_layer->last_frame_buffer);

    /* Release any Cairo resources */
    guac_display_layer_cairo_context* cairo_context = &(display_layer->pending_frame_cairo_context);
    if (cairo_context->surface != NULL) {

        cairo_surface_destroy(cairo_context->surface);
        cairo_context->surface = NULL;

        cairo_destroy(cairo_context->cairo);
        cairo_context->cairo = NULL;

    }

    /* Free memory for underlying image surface and change tracking cells
     * (NOTE: Freeing pending_frame.buffer inherently also frees
     * last_frame.buffer because they are actually interleaved views of the
     * same block) */
    guac_mem_free(display_layer->pending_frame.buffer);
    guac_mem_free(display_layer->pending_frame_cells);

    guac_mem_free(display_layer);

}

void PFW_LFW_guac_display_layer_resize(guac_display_layer* layer, int width, int height) {

    /* Flush and destroy any cached Cairo context */
    guac_display_layer_cairo_context* cairo_context = &(layer->pending_frame_cairo_context);
    if (cairo_context->surface != NULL) {

        cairo_surface_flush(cairo_context->surface);
        cairo_surface_destroy(cairo_context->surface);
        cairo_destroy(cairo_context->cairo);

        cairo_context->surface = NULL;
        cairo_context->cairo = NULL;

    }

    PFW_LFW_guac_display_layer_buffers_resize(&layer->last_frame, &layer->pending_frame, width, height);
    PFW_guac_display_layer_pending_frame_cells_resize(layer, width, height);

    layer->pending_frame.width = width;
    layer->pending_frame.height = height;

}
