
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
#include "copy.h"
#include "display.h"
#include "drawable.h"
#include "gc.h"
#include "screen.h"
#include "pixmap.h"
#include "list.h"
#include "log.h"

#include <xorg-server.h>
#include <xf86.h>
#include <fb.h>

RegionPtr guac_drv_copyarea(DrawablePtr src, DrawablePtr dst, GCPtr gc,
        int srcx, int srcy, int w, int h, int dstx, int dsty) {

    /* Call framebuffer version */
    RegionPtr ret = fbCopyArea(src, dst, gc, srcx, srcy, w, h, dstx, dsty);

    /* Get destination drawable */
    guac_drv_drawable* guac_dst = guac_drv_get_drawable(dst);

    /* Draw to windows only */
    if (guac_dst != NULL) {

        guac_drv_log(GUAC_LOG_DEBUG, "guac_drv_copyarea layer=%i "
                "(%i, %i) %ix%i", guac_dst->layer->layer->index,
                dstx, dsty, w, h);

        /* Get guac_drv_screen */
        guac_drv_screen* guac_screen =
            (guac_drv_screen*) dixGetPrivate(&(gc->devPrivates),
                                         GUAC_GC_PRIVATE);

        /* Perform operation only if simple */
        guac_drv_drawable* guac_src = guac_drv_get_drawable(src);
        if (guac_src != NULL && gc->subWindowMode == ClipByChildren)
            GUAC_DRV_DRAWABLE_CLIP(guac_dst, dst, fbGetCompositeClip(gc),
                    guac_drv_drawable_copy, guac_src, srcx, srcy, w, h,
                    guac_dst, dstx, dsty);

        /* Otherwise copy framebuffer state */
        else
            GUAC_DRV_DRAWABLE_CLIP(guac_dst, dst, fbGetCompositeClip(gc),
                    guac_drv_drawable_copy_fb, dst, dstx, dsty, w, h,
                    guac_dst, dstx, dsty);

        guac_drv_display_touch(guac_screen->display);

    }

    return ret;

}

RegionPtr guac_drv_copyplane(DrawablePtr src, DrawablePtr dst, GCPtr gc,
        int srcx, int srcy, int w, int h, int dstx, int dsty,
        unsigned long bitplane) {

    /* Call framebuffer version */
    RegionPtr ret = fbCopyPlane(src, dst, gc, srcx, srcy, w, h,
            dstx, dsty, bitplane);

    /* Get destination drawable */
    guac_drv_drawable* guac_dst = guac_drv_get_drawable(dst);

    /* Draw to windows only */
    if (guac_dst != NULL) {

        guac_drv_log(GUAC_LOG_DEBUG, "guac_drv_copyplane layer=%i "
                "(%i, %i) %ix%i", guac_dst->layer->layer->index,
                dstx, dsty, w, h);

        /* Get guac_drv_screen */
        guac_drv_screen* guac_screen =
            (guac_drv_screen*) dixGetPrivate(&(gc->devPrivates),
                                         GUAC_GC_PRIVATE);

        /* Copy framebuffer state */
        GUAC_DRV_DRAWABLE_CLIP(guac_dst, dst, fbGetCompositeClip(gc),
                guac_drv_drawable_copy_fb, dst, dstx, dsty, w, h,
                guac_dst, dstx, dsty);

        guac_drv_display_touch(guac_screen->display);

    }

    return ret;

}

