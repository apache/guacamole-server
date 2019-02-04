
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
#include "pixmap.h"
#include "screen.h"
#include "spans.h"
#include "log.h"

#include <xorg-server.h>
#include <xf86.h>
#include <fb.h>

/**
 * Common base implementation of FillSpans / SetSpans (both use the same
 * information to determine extents).
 */
static void guac_drv_copy_spans(DrawablePtr drawable, GCPtr gc, int npoints,
        DDXPointPtr points, int* width, int sorted) {

    /* Get drawable */
    guac_drv_drawable* guac_drawable = guac_drv_get_drawable(drawable);

    /* Draw to windows only */
    if (guac_drawable == NULL)
        return;

    guac_drv_log(GUAC_LOG_DEBUG, "guac_drv_copy_spans layer=%i",
            guac_drawable->layer->layer->index);

    /* Get guac_drv_screen */
    guac_drv_screen* guac_screen =
        (guac_drv_screen*) dixGetPrivate(&(gc->devPrivates),
                                     GUAC_GC_PRIVATE);

    /* Init extents with first span */
    int x1 = points->x;
    int y1 = points->y;
    int x2 = x1 + *width;
    int y2 = y1;

    /* Expand extents for all remaining spans */
    int remaining = npoints - 1;
    if (remaining > 0) {

        DDXPointPtr current_point = &points[1];
        int* current_width = &width[1];

        /* For each span (point/width pair), expand rectangle to fit */
        do {

            /* Get current span extents (single pixel line) */
            int current_x1 = current_point->x;
            int current_x2 = current_x1 + *current_width;
            int current_y = current_point->y;

            /* Expand left/right depending on span left/right */
            if (current_x1 < x1)
                x1 = current_x1;
            if (current_x2 > x2)
                x2 = current_x2;

            /* Expand top/bottom depending on span Y coordinate */
            if (current_y < y1)
                y1 = current_y;
            else if (current_y > y2)
                y2 = current_y;

            /* Advance to next span */
            remaining--;
            current_point++;
            current_width++;

        } while (remaining > 0);

    }

    /* Copy relevant region from framebuffer */
    GUAC_DRV_DRAWABLE_CLIP(guac_drawable, drawable,
        fbGetCompositeClip(gc), guac_drv_drawable_copy_fb,
        drawable, x1, y1, x2 - x1, y2 - y1 + 1, guac_drawable, x1, y1);

    /* Signal change */
    guac_drv_display_touch(guac_screen->display);

}

void guac_drv_fillspans(DrawablePtr drawable, GCPtr gc, int npoints,
        DDXPointPtr points, int* width, int sorted) {

    /* Call framebuffer version */
    fbFillSpans(drawable, gc, npoints, points, width, sorted);

    /* Copy the results from the framebuffer */
    guac_drv_copy_spans(drawable, gc, npoints, points, width, sorted);

}

void guac_drv_setspans(DrawablePtr drawable, GCPtr gc, char* src,
        DDXPointPtr points, int* width, int nspans, int sorted) {

    /* Call framebuffer version */
    fbSetSpans(drawable, gc, src, points, width, nspans, sorted);

    /* Copy the results from the framebuffer */
    guac_drv_copy_spans(drawable, gc, nspans, points, width, sorted);

}

