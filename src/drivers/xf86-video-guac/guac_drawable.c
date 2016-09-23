
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

    return drawable;

}

void guac_drv_drawable_free(guac_drv_drawable* drawable) {
    free(drawable);
}

void guac_drv_drawable_resize(guac_drv_drawable* drawable,
        int width, int height) {

    /* Set new dimensions */
    guac_common_surface_resize(drawable->layer->surface, width, height);

}

void guac_drv_drawable_put(guac_drv_drawable* drawable,
        char* data, guac_drv_drawable_format format, int stride,
        int dx, int dy, int w, int h) {

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

}

void guac_drv_drawable_drect(guac_drv_drawable* drawable, int x, int y,
        int w, int h, guac_drv_drawable* fill) {
    guac_drv_drawable_stub(drawable, x, y, w, h);
}

void guac_drv_drawable_copy(guac_drv_drawable* src, int srcx, int srcy,
        int w, int h, guac_drv_drawable* dst, int dstx, int dsty) {

    /* Perform copy */
    guac_common_surface_copy(src->layer->surface, srcx, srcy, w, h,
            dst->layer->surface, dstx, dsty);

}

void guac_drv_drawable_shade(guac_drv_drawable* drawable, int opacity) {
    guac_common_surface_set_opacity(drawable->layer->surface, opacity);
}

void guac_drv_drawable_move(guac_drv_drawable* drawable, int x, int y) {
    guac_common_surface_move(drawable->layer->surface, x, y);
}

void guac_drv_drawable_stack(guac_drv_drawable* drawable, int z) {
    guac_common_surface_stack(drawable->layer->surface, z);
}

void guac_drv_drawable_reparent(guac_drv_drawable* drawable,
        guac_drv_drawable* parent) {

    if (parent != NULL)
        guac_common_surface_set_parent(drawable->layer->surface,
                parent->layer->surface->layer);

    else
        guac_common_surface_set_parent(drawable->layer->surface,
                GUAC_DEFAULT_LAYER);

}

