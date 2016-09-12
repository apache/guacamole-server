
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

    int r = rand() & 0xFF;
    int g = rand() & 0xFF;
    int b = rand() & 0xFF;

    /* Draw rectangle with random color */
    guac_common_surface_rect(drawable->layer->surface, dx, dy, w, h, r, g, b);

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
    guac_drv_drawable_stub(drawable, 0, 0, width, height);

    guac_drv_drawable_unlock(drawable);

}

#if 0
/**
 * 32bpp-specific PutImage
 */
static void _guac_drv_drawable_put32(guac_drv_drawable* drawable,
        char* data, int stride, int dx, int dy, int w, int h,
        guac_drv_rect* dirty) {

    int x, y;
    unsigned char* row = (unsigned char*) drawable->operations
                       + dy*drawable->operations_stride
                       + dx*sizeof(guac_drv_drawable_operation);

    uint32_t* pixel = (uint32_t*) data;

    /* Overall bounds */
    int max_x = 0;
    int max_y = 0;
    int min_x = w;
    int min_y = h;
 
    /* Copy each pixel as a new SET operation */
    for (y=0; y<h; y++) {

        guac_drv_drawable_operation* current = (guac_drv_drawable_operation*) row;
        for (x=0; x<w; x++) {

            int new_color = *pixel;
            int old_color = current->old_color;

            /* If color different, set as SET */
            if (new_color != old_color) {
                current->type = GUAC_DRV_DRAWABLE_SET;
                current->order = drawable->operations_pending;
                current->color = new_color;

                /* Update bounds */
                if (x > max_x) max_x = x;
                if (x < min_x) min_x = x;
                if (y > max_y) max_y = y;
                if (y < min_y) min_y = y;

            }

            /* Otherwise, no operation */
            else {
                current->type = GUAC_DRV_DRAWABLE_NOP;
                current->order = drawable->operations_pending;
                current->color = old_color;
            }

            /* Next pixel/operation */
            pixel++;
            current++;

        }

        row += drawable->operations_stride;

    }

    /* Save real dirty rect */
    if (max_x > min_x && max_y > min_y)
        guac_drv_rect_init(dirty,
                dx+min_x, dy+min_y,
                max_x - min_x + 1, max_y - min_y + 1);
    else
        guac_drv_rect_clear(dirty);

}

/**
 * 24bpp-specific PutImage
 */
static void _guac_drv_drawable_put24(guac_drv_drawable* drawable,
        char* data, int stride, int dx, int dy, int w, int h,
        guac_drv_rect* dirty) {

    int x, y;
    unsigned char* row = (unsigned char*) drawable->operations
                       + dy*drawable->operations_stride
                       + dx*sizeof(guac_drv_drawable_operation);

    uint32_t* pixel = (uint32_t*) data;

    /* Overall bounds */
    int max_x = 0;
    int max_y = 0;
    int min_x = w;
    int min_y = h;

    /* Copy each pixel as a new SET operation */
    for (y=0; y<h; y++) {

        guac_drv_drawable_operation* current = (guac_drv_drawable_operation*) row;
        for (x=0; x<w; x++) {

            int new_color = *pixel | 0xFF000000;
            int old_color = current->old_color;

            /* If color different, set as SET */
            if (new_color != old_color) {
                current->type = GUAC_DRV_DRAWABLE_SET;
                current->order = drawable->operations_pending;
                current->color = new_color;

                /* Update bounds */
                if (x > max_x) max_x = x;
                if (x < min_x) min_x = x;
                if (y > max_y) max_y = y;
                if (y < min_y) min_y = y;

            }

            /* Otherwise, no operation */
            else {
                current->type = GUAC_DRV_DRAWABLE_NOP;
                current->color = old_color;
            }

            /* Next pixel/operation */
            pixel++;
            current++;

        }

        row += drawable->operations_stride;

    }

    /* Save real dirty rect */
    if (max_x > min_x && max_y > min_y)
        guac_drv_rect_init(dirty,
                dx+min_x, dy+min_y,
                max_x - min_x + 1, max_y - min_y + 1);
    else
        guac_drv_rect_clear(dirty);

}
#endif

void guac_drv_drawable_put(guac_drv_drawable* drawable,
        char* data, guac_drv_drawable_format format, int stride,
        int dx, int dy, int w, int h) {

    guac_drv_drawable_lock(drawable);

    /* Call appropriate format-specific implementation */
    switch (format) {

#if 0
        /* 32bpp */
        case GUAC_DRV_DRAWABLE_ARGB_32:
            _guac_drv_drawable_put32(drawable, data, stride,
                    dst_rect.x, dst_rect.y, dst_rect.width, dst_rect.height,
                    &dirty);
            break;

        /* 24bpp */
        case GUAC_DRV_DRAWABLE_RGB_24:
            _guac_drv_drawable_put24(drawable, data, stride,
                    dst_rect.x, dst_rect.y, dst_rect.width, dst_rect.height,
                    &dirty);
            break;
#endif

        /* Use stub by default */
        default:
            guac_drv_drawable_stub(drawable, dx, dy, w, h);

    }

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

void guac_drv_drawable_destroy(guac_drv_drawable* drawable) {
    guac_drv_drawable_lock(drawable);
    /* FIXME: Destroy */
    guac_drv_drawable_unlock(drawable);
}

