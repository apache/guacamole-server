
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
#include "guac_gc.h"
#include "guac_screen.h"
#include "guac_pixmap.h"
#include "list.h"

#include <xorg-server.h>
#include <xf86.h>
#include <fb.h>

RegionPtr guac_drv_copyarea(DrawablePtr src, DrawablePtr dst, GCPtr gc,
        int srcx, int srcy, int w, int h, int dstx, int dsty) {

    int i;
	RegionPtr clip = fbGetCompositeClip(gc);
    int num_rects = REGION_NUM_RECTS(clip);
    BoxPtr current_rect = REGION_RECTS(clip);

    /* Get guac_drv_screen */
    guac_drv_screen* guac_screen = 
        (guac_drv_screen*) dixGetPrivate(&(gc->devPrivates),
                                     GUAC_GC_PRIVATE);

    /* Get source and destination drawables */
    guac_drv_drawable* guac_src = guac_drv_get_drawable(src);
    guac_drv_drawable* guac_dst = guac_drv_get_drawable(dst);

    /* Perform operation */
    for (i=0; i<num_rects; i++) {

        int x1 = dstx;
        int y1 = dsty;
        int x2 = dstx+w;
        int y2 = dsty+h;

        int clip_x1 = current_rect->x1 - dst->x;
        int clip_y1 = current_rect->y1 - dst->y;
        int clip_x2 = current_rect->x2 - dst->x;
        int clip_y2 = current_rect->y2 - dst->y;

        if (clip_x1 > x1) x1 = clip_x1;
        if (clip_y1 > y1) y1 = clip_y1;
        if (clip_x2 < x2) x2 = clip_x2;
        if (clip_y2 < y2) y2 = clip_y2;

        if (x1 < x2 && y1 < y2)
            guac_drv_drawable_copy(
                    guac_src, srcx + (x1-dstx), srcy + (y1-dsty), x2-x1, y2-y1,
                    guac_dst, x1, y1);

        current_rect++;

    }

    guac_drv_display_touch(guac_screen->display);

    /* STUB */
    /*xf86Msg(X_INFO, "guac: STUB: %s src_layer=%i (%i, %i) "
                    "dst_layer=%i (%i, %i) %ix%i\n", __func__,
        guac_src->index, srcx, srcy,
        guac_dst->index, dstx, dsty, w, h);*/

    return fbCopyArea(src, dst, gc, srcx, srcy, w, h, dstx, dsty);
}

RegionPtr guac_drv_copyplane(DrawablePtr src, DrawablePtr dst, GCPtr gc,
        int srcx, int srcy, int w, int h, int dstx, int dsty,
        unsigned long bitplane) {
    /* STUB */
    xf86Msg(X_INFO, "guac: STUB: %s src_layer=%i dst_layer=%i\n", __func__,
        guac_drv_get_drawable(src)->index,
        guac_drv_get_drawable(dst)->index);
    return fbCopyPlane(src, dst, gc, srcx, srcy, w, h, dstx, dsty, bitplane);
}

