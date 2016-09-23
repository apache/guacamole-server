
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

#include "config.h"
#include "guac_drawable.h"
#include "guac_rect.h"
#include "list.h"

#include <xf86.h>

#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <cairo/cairo.h>

void guac_drv_drawable_stub(guac_drv_drawable* drawable, int dx, int dy,
        int w, int h) {

    /* Draw rectangle with The Magenta of Failure */
    guac_common_surface_rect(drawable->layer->surface, dx, dy, w, h,
            0xFF, 0x00, 0xFF);

}

guac_drv_drawable* guac_drv_drawable_alloc(guac_common_display_layer* layer) {

    guac_drv_drawable* drawable = malloc(sizeof(guac_drv_drawable));

    /* Init underlying layer */
    drawable->layer = layer;

    /* Init mutex */
    pthread_mutex_init(&(drawable->lock), NULL);

    return drawable;

}

void guac_drv_drawable_free(guac_drv_drawable* drawable) {
    pthread_mutex_destroy(&(drawable->lock));
    free(drawable);
}

void guac_drv_drawable_lock(guac_drv_drawable* drawable) {
    pthread_mutex_lock(&(drawable->lock));
}

void guac_drv_drawable_unlock(guac_drv_drawable* drawable) {
    pthread_mutex_unlock(&(drawable->lock));
}

void guac_drv_drawable_resize(guac_drv_drawable* drawable,
        int width, int height) {

    guac_drv_drawable_lock(drawable);

    /* Set new dimensions */
    guac_common_surface_resize(drawable->layer->surface, width, height);

    guac_drv_drawable_unlock(drawable);

}

void guac_drv_drawable_put(guac_drv_drawable* drawable,
        char* data, guac_drv_drawable_format format, int stride,
        int dx, int dy, int w, int h) {

    guac_drv_drawable_lock(drawable);

    cairo_surface_t* surface;

    /* Call appropriate format-specific implementation */
    switch (format) {

        /* 32bpp */
        case GUAC_DRV_DRAWABLE_ARGB_32:
            surface = cairo_image_surface_create_for_data((unsigned char*) data,
                    CAIRO_FORMAT_ARGB32, w, h, stride);
            guac_common_surface_draw(drawable->layer->surface, dx, dy, surface);
            break;

        /* 24bpp */
        case GUAC_DRV_DRAWABLE_RGB_24:
            surface = cairo_image_surface_create_for_data((unsigned char*) data,
                    CAIRO_FORMAT_RGB24, w, h, stride);
            guac_common_surface_draw(drawable->layer->surface, dx, dy, surface);
            break;

        /* Use stub by default */
        default:
            guac_drv_drawable_stub(drawable, dx, dy, w, h);

    }

    guac_drv_drawable_unlock(drawable);

}

void guac_drv_drawable_drect(guac_drv_drawable* drawable, int x, int y,
        int w, int h, guac_drv_drawable* fill) {
    guac_drv_drawable_lock(drawable);
    guac_drv_drawable_stub(drawable, x, y, w, h);
    guac_drv_drawable_unlock(drawable);
}

void guac_drv_drawable_copy(guac_drv_drawable* src, int srcx, int srcy,
        int w, int h, guac_drv_drawable* dst, int dstx, int dsty) {

    /* Lock surfaces */
    guac_drv_drawable_lock(dst);
    if (src != dst)
        guac_drv_drawable_lock(src);

    /* Perform copy */
    guac_common_surface_copy(src->layer->surface, srcx, srcy, w, h,
            dst->layer->surface, dstx, dsty);

    /* Unlock surfaces */
    guac_drv_drawable_unlock(dst);
    if (src != dst)
        guac_drv_drawable_unlock(src);

}

void guac_drv_drawable_shade(guac_drv_drawable* drawable, int opacity) {
    guac_drv_drawable_lock(drawable);
    guac_common_surface_set_opacity(drawable->layer->surface, opacity);
    guac_drv_drawable_unlock(drawable);
}

void guac_drv_drawable_move(guac_drv_drawable* drawable, int x, int y) {
    guac_drv_drawable_lock(drawable);
    guac_common_surface_move(drawable->layer->surface, x, y);
    guac_drv_drawable_unlock(drawable);
}

void guac_drv_drawable_stack(guac_drv_drawable* drawable, int z) {
    guac_drv_drawable_lock(drawable);
    guac_common_surface_stack(drawable->layer->surface, z);
    guac_drv_drawable_unlock(drawable);
}

void guac_drv_drawable_reparent(guac_drv_drawable* drawable,
        guac_drv_drawable* parent) {
    guac_drv_drawable_lock(drawable);

    if (parent != NULL)
        guac_common_surface_set_parent(drawable->layer->surface,
                parent->layer->surface->layer);

    else
        guac_common_surface_set_parent(drawable->layer->surface,
                GUAC_DEFAULT_LAYER);

    guac_drv_drawable_unlock(drawable);
}

