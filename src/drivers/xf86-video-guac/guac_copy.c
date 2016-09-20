
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
#include "guac_copy.h"
#include "guac_display.h"
#include "guac_drawable.h"
#include "guac_gc.h"
#include "guac_screen.h"
#include "guac_pixmap.h"
#include "list.h"

#include <xorg-server.h>
#include <xf86.h>
#include <fb.h>

RegionPtr guac_drv_copyarea(DrawablePtr src, DrawablePtr dst, GCPtr gc,
        int srcx, int srcy, int w, int h, int dstx, int dsty) {

    /* Get guac_drv_screen */
    guac_drv_screen* guac_screen = 
        (guac_drv_screen*) dixGetPrivate(&(gc->devPrivates),
                                     GUAC_GC_PRIVATE);

    /* Get source and destination drawables */
    guac_drv_drawable* guac_src = guac_drv_get_drawable(src);
    guac_drv_drawable* guac_dst = guac_drv_get_drawable(dst);

    /* Perform operation, clipped by current clipping path */
    GUAC_DRV_DRAWABLE_CLIP(guac_dst, dst, fbGetCompositeClip(gc),
            guac_drv_drawable_copy, guac_src, srcx, srcy, w, h,
            guac_dst, dstx, dsty);

    guac_drv_display_touch(guac_screen->display);

    return fbCopyArea(src, dst, gc, srcx, srcy, w, h, dstx, dsty);
}

RegionPtr guac_drv_copyplane(DrawablePtr src, DrawablePtr dst, GCPtr gc,
        int srcx, int srcy, int w, int h, int dstx, int dsty,
        unsigned long bitplane) {
    /* STUB */
    xf86Msg(X_INFO, "guac: STUB: %s src_layer=%i dst_layer=%i\n", __func__,
        guac_drv_get_drawable(src)->layer->layer->index,
        guac_drv_get_drawable(dst)->layer->layer->index);
    return fbCopyPlane(src, dst, gc, srcx, srcy, w, h, dstx, dsty, bitplane);
}

