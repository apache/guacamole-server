
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
#include "drawable.h"
#include "list.h"

#include <xf86.h>

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <cairo/cairo.h>

void guac_drv_drawable_stub(guac_drv_drawable* drawable, int dx, int dy,
        int w, int h, uint32_t color) {
    guac_drv_drawable_crect(drawable, dx, dy, w, h, color);
}

void guac_drv_drawable_copy_fb(DrawablePtr src, int srcx, int srcy,
        int srcw, int srch, guac_drv_drawable* dst, int dstx, int dsty) {

    /* Retrieve image contents */
    char* buffer = malloc(srcw * srch * 4);
    fbGetImage(src, srcx, srcy, srcw, srch, ZPixmap, FB_ALLONES, buffer);

    /* Draw to destination surface */
    guac_drv_drawable_put(dst, buffer, GUAC_DRV_DRAWABLE_RGB_24, srcw * 4,
            dstx, dsty, srcw, srch);

    /* Buffer no longer needed */
    free(buffer);

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
            xf86Msg(X_INFO, "guac: STUB FFFF00: %s:%d: %s()\n",
                    __FILE__, __LINE__, __func__);
            guac_drv_drawable_stub(drawable, dx, dy, w, h, 0xFFFF00);

    }

}

void guac_drv_drawable_crect(guac_drv_drawable* drawable, int x, int y,
        int w, int h, int fill) {

    /* Pull RGB components from color */
    int r = (fill >> 16) & 0xFF;
    int g = (fill >> 8)  & 0xFF;
    int b =  fill        & 0xFF;

    /* Draw rectangle with requested color */
    guac_common_surface_rect(drawable->layer->surface, x, y, w, h, r, g, b);

}

void guac_drv_drawable_drect(guac_drv_drawable* drawable, int x, int y,
        int w, int h, guac_drv_drawable* fill) {
    xf86Msg(X_INFO, "guac: STUB 00FFFF: %s:%d: %s()\n",
            __FILE__, __LINE__, __func__);
    guac_drv_drawable_stub(drawable, x, y, w, h, 0x00FFFF);
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

