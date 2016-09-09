
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

#ifndef __GUAC_DRAWABLE_H
#define __GUAC_DRAWABLE_H

#include "config.h"
#include "guac_rect.h"
#include "list.h"

#include <pthread.h>
#include <stdint.h>

#include <cairo/cairo.h>

typedef struct guac_drv_drawable guac_drv_drawable;

/**
 * All available operations. Each operation affects a single pixel.
 */
typedef enum guac_drv_drawable_operation_type {

    /**
     * Operation which does nothing.
     */
    GUAC_DRV_DRAWABLE_NOP = 0,

    /**
     * Operation which copies a single pixel from a given location in another
     * drawable.
     */
    GUAC_DRV_DRAWABLE_COPY,

    /**
     * Operation which sets a single pixel value.
     */
    GUAC_DRV_DRAWABLE_SET

} guac_drv_drawable_operation_type;

/**
 * All supported types of drawables.
 */
typedef enum guac_drv_drawable_format {

    /**
     * 32bpp format with the high-order byte being alpha and the low-order byte
     * being blue.
     */
    GUAC_DRV_DRAWABLE_ARGB_32,

    /**
     * 24bpp format with the high-order byte being red and the low-order byte
     * being blue. This is actually a 32bpp format, but the highest-order byte
     * is unused.
     */
    GUAC_DRV_DRAWABLE_RGB_24,

    /**
     * Any as-of-yet unsupported format.
     */
    GUAC_DRV_DRAWABLE_UNSUPPORTED

} guac_drv_drawable_format;

/**
 * A pairing of a guac_drv_drawable_operation_type and all parameters required
 * by that operation type.
 */
typedef struct guac_drv_drawable_operation {

    /**
     * The type of operation to perform.
     */
    guac_drv_drawable_operation_type type;

    /**
     * The index representing the sort order of this operation with
     * respect to others in the same drawable.
     */
    int order;

    /**
     * The ARGB color (alpha high, blue low) that was assigned to this pixel
     * after the previous flush.
     */
    uint32_t old_color;

    /**
     * The ARGB color (alpha high, blue low) to assign to the pixel. This is
     * only really applicable to GUAC_DRV_DRAWABLE_SET, but will always contain
     * the color of the corresponding pixel, even after flush, even if copied.
     */
    uint32_t color;

    /**
     * The source drawable to copy pixel data from. This is only applicable
     * to GUAC_DRV_DRAWABLE_COPY.
     */
    guac_drv_drawable* source;

    /**
     * The X coordinate of the pixel to copy from. This is only applicable
     * to GUAC_DRV_DRAWABLE_COPY.
     */
    int x;

    /**
     * The Y coordinate of the pixel to copy from. This is only applicable
     * to GUAC_DRV_DRAWABLE_COPY.
     */
    int y;

} guac_drv_drawable_operation;

/**
 * The current synchronization state of a drawable.
 */
typedef enum guac_drv_drawable_sync_state {

    /**
     * The drawable is newly created, and thus is not yet synced to any client.
     */
    GUAC_DRV_DRAWABLE_NEW,

    /**
     * The drawable is synced to all clients. If a new client connects, it will
     * be synced to that client, too.
     */
    GUAC_DRV_DRAWABLE_SYNCED,

    /**
     * The drawable is offline, and will not be synced to any client.
     */
    GUAC_DRV_DRAWABLE_OFFLINE,

    /**
     * The drawable is no longer in use and must be cleaned up and removed
     * from all clients on next flush.
     */
    GUAC_DRV_DRAWABLE_DESTROYED,

} guac_drv_drawable_sync_state;

/**
 * All available types of drawables.
 */
typedef enum guac_drv_drawable_type {

    /**
     * A Guacamole buffer.
     */
    GUAC_DRV_DRAWABLE_BUFFER,

    /**
     * A Guacamole layer.
     */
    GUAC_DRV_DRAWABLE_LAYER

} guac_drv_drawable_type;

/**
 * The state of a drawable.
 */
typedef struct guac_drv_drawable_state {

    /**
     * The parent drawable, if any.
     */
    guac_drv_drawable* parent;

    /**
     * The current rectangle representing the location and size of the
     * drawable.
     */
    guac_drv_rect rect;

    /**
     * The level of opacity. Fully opaque is 255, while fully transparent is
     * 0.
     */
    int opacity;

    /**
     * The Z-order of this drawable, relative to sibling drawables.
     */
    int z;

} guac_drv_drawable_state;

struct guac_drv_drawable {

    /**
     * The type of drawable.
     */
    guac_drv_drawable_type type;

    /**
     * The layer index to associate with this drawable.
     */
    int index;

    /**
     * Whether the drawable has been allocated an index and sent to the client.
     */
    int realized;

    /**
     * The number of rows of data in the backing surface.
     */
    int rows;

    /**
     * The number of drawing operations currently pending.
     */
    int operations_pending;

    /**
     * The size of each row of the backing surface, in bytes.
     */
    int image_stride;

    /**
     * Raw image data backing the Cairo surface.
     */
    unsigned char* image_data;

    /**
     * The Cairo surface holding any associated image data.
     */
    cairo_surface_t* surface;

    /**
     * The size of each row operations buffer, in bytes.
     */
    int operations_stride;

    /**
     * All pending operations.
     */
    guac_drv_drawable_operation* operations;

    /**
     * The current state of this drawable.
     */
    guac_drv_drawable_sync_state sync_state;

    /**
     * The extent of the dirty (changed) area of this drawable. If nothing
     * is changed, the width and height of the dirty rect will be 0.
     */
    guac_drv_rect dirty;

    /**
     * Current drawable state (already flushed).
     */
    guac_drv_drawable_state current;

    /**
     * Pending drawable state (waiting to be flushed).
     */
    guac_drv_drawable_state pending;

    /**
     * Mutex protecting this drawable from simultaneous access.
     */
    pthread_mutex_t lock;

    /**
     * Arbitrary data associated with this drawable.
     */
    void* data;

};

/**
 * Allocates a new drawable surface.
 */
guac_drv_drawable* guac_drv_drawable_alloc(guac_drv_drawable_type type,
        guac_drv_drawable* parent, int x, int y, int z,
        int width, int height,
        int opacity, int online);

/**
 * Frees the given drawable and any associated resources.
 */
void guac_drv_drawable_free(guac_drv_drawable* drawable);

/**
 * Locks this drawable, preventing access from other threads.
 */
void guac_drv_drawable_lock(guac_drv_drawable* drawable);

/**
 * Unlocks this drawable, allowing access from other threads.
 */
void guac_drv_drawable_unlock(guac_drv_drawable* drawable);

/**
 * Resizes the given drawable to the given width and height.
 */
void guac_drv_drawable_resize(guac_drv_drawable* drawable,
        int width, int height);

/**
 * Initializes the contents of a drawable to a checkerboard pattern having a
 * random base color.
 */
void guac_drv_drawable_stub(guac_drv_drawable* drawable, int dx, int dy,
        int w, int h);

/**
 * Copies the contents of the given buffer having the given stride to the
 * given location.
 */
void guac_drv_drawable_put(guac_drv_drawable* drawable,
        char* data, guac_drv_drawable_format format, int stride,
        int dx, int dy, int w, int h);

/**
 * Copies the contents of the given drawable to the given location.
 */
void guac_drv_drawable_copy(
        guac_drv_drawable* src, int srcx, int srcy, int w, int h,
        guac_drv_drawable* dst, int dstx, int dsty);

/**
 * Change the opacity of the given drawable.
 */
void guac_drv_drawable_shade(guac_drv_drawable* drawable, int opacity);

/**
 * Move the given drawable to the given location.
 */
void guac_drv_drawable_move(guac_drv_drawable* drawable, int x, int y);

/**
 * Change the stacking order of the given drawable.
 */
void guac_drv_drawable_stack(guac_drv_drawable* drawable, int z);

/**
 * Change the parent of the given drawable.
 */
void guac_drv_drawable_reparent(guac_drv_drawable* drawable,
        guac_drv_drawable* parent);

/**
 * Destroys the given drawable, but does not free any server-side resources.
 */
void guac_drv_drawable_destroy(guac_drv_drawable* drawable);

#endif

