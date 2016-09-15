
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
#include "common/display.h"
#include "guac_rect.h"
#include "list.h"

#include <pthread.h>
#include <stdint.h>

#include <cairo/cairo.h>

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

typedef struct guac_drv_drawable {

    /**
     * The underlying graphical surface which should be replicated across all
     * connected clients.
     */
    guac_common_display_layer* layer;

    /**
     * Mutex protecting this drawable from simultaneous access.
     */
    pthread_mutex_t lock;

    /**
     * Arbitrary data associated with this drawable.
     */
    void* data;

} guac_drv_drawable;

/**
 * Allocates a new drawable surface.
 */
guac_drv_drawable* guac_drv_drawable_alloc(guac_common_display_layer* layer);

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

void guac_drv_drawable_drect(guac_drv_drawable* drawable, int x, int y,
        int w, int h, guac_drv_drawable* fill);

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

#endif

