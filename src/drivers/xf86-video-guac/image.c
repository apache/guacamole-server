
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
#include "display.h"
#include "drawable.h"
#include "gc.h"
#include "image.h"
#include "pixmap.h"
#include "screen.h"
#include "window.h"
#include "list.h"

void guac_drv_putimage(DrawablePtr drawable, GCPtr gc, int depth,
        int x, int y, int w, int h, int left_pad, int format,
        char* bits) {

    /* Call framebuffer version */
    fbPutImage(drawable, gc, depth, x, y, w, h, left_pad, format, bits);

    /* Get drawable */
    guac_drv_drawable* guac_drawable = guac_drv_get_drawable(drawable);

    /* Draw to windows only */
    if (guac_drawable != NULL) {

        /* Get guac_drv_screen */
        guac_drv_screen* guac_screen =
            (guac_drv_screen*) dixGetPrivate(&(gc->devPrivates),
                                         GUAC_GC_PRIVATE);

        /* Copy framebuffer state within clipping area */
        GUAC_DRV_DRAWABLE_CLIP(guac_drawable, drawable, fbGetCompositeClip(gc),
                guac_drv_drawable_copy_fb, drawable, x, y, w, h,
                guac_drawable, x, y);

        guac_drv_display_touch(guac_screen->display);

    }

}

void guac_drv_pushpixels(GCPtr gc, PixmapPtr bitmap, DrawablePtr dst,
        int w, int h, int x, int y) {

    /* Call framebuffer version */
    fbPushPixels(gc, bitmap, dst, w, h, x, y);

    /* Get destination drawable */
    guac_drv_drawable* guac_dst = guac_drv_get_drawable(dst);

    /* Draw to windows only */
    if (guac_dst != NULL) {

        /* Get guac_drv_screen */
        guac_drv_screen* guac_screen =
            (guac_drv_screen*) dixGetPrivate(&(gc->devPrivates),
                                         GUAC_GC_PRIVATE);

        /* Copy framebuffer state within clipping area */
        GUAC_DRV_DRAWABLE_CLIP(guac_dst, dst, fbGetCompositeClip(gc),
                guac_drv_drawable_copy_fb, dst, x, y, w, h,
                guac_dst, x, y);

        guac_drv_display_touch(guac_screen->display);

    }

}

