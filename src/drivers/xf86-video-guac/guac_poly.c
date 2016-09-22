
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
#include "guac_drawable.h"
#include "guac_gc.h"
#include "guac_pixmap.h"
#include "guac_poly.h"
#include "guac_screen.h"
#include "list.h"

#include <xorg-server.h>
#include <xf86.h>
#include <fb.h>

void guac_drv_polypoint(DrawablePtr drawable, GCPtr gc, int mode, int npt,
        DDXPointPtr init) {
    /* STUB */
    xf86Msg(X_INFO, "guac: STUB: %s layer=%i\n", __func__,
        guac_drv_get_drawable(drawable)->layer->layer->index);
    fbPolyPoint(drawable, gc, mode, npt, init);
}

void guac_drv_polyline(DrawablePtr drawable, GCPtr gc, int mode, int npt,
        DDXPointPtr init) {
    /* STUB */
    xf86Msg(X_INFO, "guac: STUB: %s layer=%i\n", __func__,
        guac_drv_get_drawable(drawable)->layer->layer->index);
    fbPolyLine(drawable, gc, mode, npt, init);
}

void guac_drv_polysegment(DrawablePtr drawable, GCPtr gc, int nseg,
        xSegment* segs) {
    /* STUB */
    xf86Msg(X_INFO, "guac: STUB: %s layer=%i\n", __func__,
        guac_drv_get_drawable(drawable)->layer->layer->index);
    fbPolySegment(drawable, gc, nseg, segs);
}

void guac_drv_polyrectangle(DrawablePtr drawable, GCPtr gc, int nrects,
        xRectangle* rects) {
    /* STUB */
    xf86Msg(X_INFO, "guac: STUB: %s layer=%i\n", __func__,
        guac_drv_get_drawable(drawable)->layer->layer->index);
    fbPolyRectangle(drawable, gc, nrects, rects);
}

void guac_drv_polyarc(DrawablePtr drawable, GCPtr gc, int narcs,
        xArc* arcs) {
    /* STUB */
    xf86Msg(X_INFO, "guac: STUB: %s layer=%i\n", __func__,
        guac_drv_get_drawable(drawable)->layer->layer->index);
    fbPolyArc(drawable, gc, narcs, arcs);
}

void guac_drv_fillpolygon(DrawablePtr drawable, GCPtr gc, int shape, int mode,
        int count, DDXPointPtr pts) {
    /* STUB */
    xf86Msg(X_INFO, "guac: STUB: %s layer=%i\n", __func__,
        guac_drv_get_drawable(drawable)->layer->layer->index);
    fbFillPolygon(drawable, gc, shape, mode, count, pts);
}

void guac_drv_polyfillrect(DrawablePtr drawable, GCPtr gc, int nrects,
        xRectangle* rects) {

    int i;

    /* Get guac_drv_screen */
    guac_drv_screen* guac_screen = 
        (guac_drv_screen*) dixGetPrivate(&(gc->devPrivates),
                                     GUAC_GC_PRIVATE);

    /* Get drawable */
    guac_drv_drawable* guac_drawable = guac_drv_get_drawable(drawable);

    /* Draw all rects */
    for (i=0; i<nrects; i++) {
        xRectangle* rect = &(rects[i]);

        /* If tiled, fill with pixmap */
        if ((gc->fillStyle == FillTiled || gc->fillStyle == FillOpaqueStippled)
                && !gc->tileIsPixel) {

            guac_drv_drawable* guac_fill_drawable =
                guac_drv_get_drawable((DrawablePtr) gc->tile.pixmap);

            /* Get dimensions of tile drawable */
            int tile_w = guac_fill_drawable->layer->surface->width;
            int tile_h = guac_fill_drawable->layer->surface->height;

            /* Calculate coordinates of pattern within tile given GC origin */
            int tile_x = GUAC_DRV_DRAWABLE_WRAP(rect->x - gc->patOrg.x, tile_w);
            int tile_y = GUAC_DRV_DRAWABLE_WRAP(rect->y - gc->patOrg.y, tile_h);

            /* Represent with a simple copy whenever possible */
            if (tile_x + rect->width <= tile_w
                    && tile_y + rect->height <= tile_h)
                GUAC_DRV_DRAWABLE_CLIP(guac_drawable, drawable,
                    fbGetCompositeClip(gc), guac_drv_drawable_copy,
                    guac_fill_drawable, tile_x, tile_y, rect->width,
                    rect->height, guac_drawable, rect->x, rect->y);

            /* Otherwise, use an actual pattern fill */
            else
                GUAC_DRV_DRAWABLE_CLIP(guac_drawable, drawable,
                    fbGetCompositeClip(gc), guac_drv_drawable_drect,
                    guac_drawable, rect->x, rect->y, rect->width, rect->height,
                    guac_fill_drawable);

        }

        /* Otherwise, STUB with color */
        else
            GUAC_DRV_DRAWABLE_CLIP(guac_drawable, drawable,
                fbGetCompositeClip(gc), guac_drv_drawable_stub, guac_drawable,
                rect->x, rect->y, rect->width, rect->height);

    }

    /* Signal change */
    guac_drv_display_touch(guac_screen->display);

    /* STUB */
    /*xf86Msg(X_INFO, "guac: STUB: %s layer=%i\n", __func__,
            guac_drv_get_drawable(drawable)->layer->layer->index);*/
    fbPolyFillRect(drawable, gc, nrects, rects);
}

void guac_drv_polyfillarc(DrawablePtr drawable, GCPtr gc, int narcs,
        xArc* arcs) {
    /* STUB */
    xf86Msg(X_INFO, "guac: STUB: %s layer=%i\n", __func__,
        guac_drv_get_drawable(drawable)->layer->layer->index);
    fbPolyFillArc(drawable, gc, narcs, arcs);
}

