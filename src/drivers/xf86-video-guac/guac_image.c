
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
#include "guac_display.h"
#include "guac_gc.h"
#include "guac_image.h"
#include "guac_pixmap.h"
#include "guac_screen.h"
#include "guac_window.h"
#include "list.h"

#include <xorg-server.h>
#include <xf86.h>
#include <fb.h>

void guac_drv_putimage(DrawablePtr drawable, GCPtr gc, int depth,
        int x, int y, int w, int h, int left_pad, int format,
        char* bits) {

    guac_drv_drawable_format guac_format;

    /* Get guac_drv_screen */
    guac_drv_screen* guac_screen = 
        (guac_drv_screen*) dixGetPrivate(&(gc->devPrivates),
                                     GUAC_GC_PRIVATE);

    /* Get drawable */
    guac_drv_drawable* guac_drawable = guac_drv_get_drawable(drawable);

    /* Find appropriate drawable format */
    if (format == PIXMAN_TYPE_ARGB && left_pad == 0) {
        if (depth == 32)
            guac_format = GUAC_DRV_DRAWABLE_ARGB_32;
        else if (depth == 24)
            guac_format = GUAC_DRV_DRAWABLE_RGB_24;
        else
            guac_format = GUAC_DRV_DRAWABLE_UNSUPPORTED;
    }
    else {
        guac_format = GUAC_DRV_DRAWABLE_UNSUPPORTED;
        xf86Msg(X_INFO, "guac: unsupported PutImage: layer=%i format=0x%x"
                        " depth=%i left_pad=%i\n",
                        guac_drawable->layer->layer->index, format, depth, left_pad);
    }

    /* Perform draw operation */
    guac_drv_drawable_put(guac_drawable, bits, guac_format, w*4, x, y, w, h);
    guac_drv_display_touch(guac_screen->display);

    /* Call framebuffer version */
    fbPutImage(drawable, gc, depth, x, y, w, h, left_pad, format, bits);
}

void guac_drv_pushpixels(GCPtr gc, PixmapPtr bitmap, DrawablePtr dst,
        int w, int h, int x, int y) {

    /* Get guac_drv_screen */
    guac_drv_screen* guac_screen = 
        (guac_drv_screen*) dixGetPrivate(&(gc->devPrivates),
                                     GUAC_GC_PRIVATE);

    /* Get source and destination drawables */
    guac_drv_drawable* guac_src = guac_drv_get_drawable((DrawablePtr) bitmap);
    guac_drv_drawable* guac_dst = guac_drv_get_drawable(dst);

    /* Perform operation */
    guac_drv_drawable_copy(guac_src, 0, 0, w, h, guac_dst, x, y);
    guac_drv_display_touch(guac_screen->display);

    /* STUB */
    xf86Msg(X_INFO, "guac: STUB: %s src_layer=%i dst_layer=%i\n", __func__,
        guac_drv_get_drawable((DrawablePtr) bitmap)->layer->layer->index,
        guac_drv_get_drawable(dst)->layer->layer->index);
    fbPushPixels(gc, bitmap, dst, w, h, x, y);
}

