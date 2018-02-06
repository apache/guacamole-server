
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
#include "gc.h"
#include "glyph.h"
#include "pixmap.h"
#include "screen.h"

#include <xorg-server.h>
#include <xf86.h>
#include <dixfont.h>
#include <fb.h>
#include <X11/fonts/fontutil.h>

/**
 * Common base implementation of ImageGlyphBlt / PolyGlyphBlt (both use the
 * same information for determining extents).
 */
static void guac_drv_copy_glyphs(DrawablePtr drawable, GCPtr gc, int x, int y,
        unsigned int nglyph, CharInfoPtr* char_info, pointer glyph_base) {

    /* Get drawable */
    guac_drv_drawable* guac_drawable = guac_drv_get_drawable(drawable);

    /* Draw to windows only */
    if (guac_drawable == NULL)
        return;

    /* Get glyph extents */
    ExtentInfoRec extents;
    QueryGlyphExtents(gc->font, char_info, nglyph, &extents);

    /* Get guac_drv_screen */
    guac_drv_screen* guac_screen =
        (guac_drv_screen*) dixGetPrivate(&(gc->devPrivates),
                                     GUAC_GC_PRIVATE);

    x += extents.overallLeft;
    y -= extents.overallAscent;

    /* Copy framebuffer state within clipping area */
    GUAC_DRV_DRAWABLE_CLIP(guac_drawable, drawable, fbGetCompositeClip(gc),
            guac_drv_drawable_copy_fb, drawable, x, y,
            extents.overallRight - extents.overallLeft,
            extents.overallDescent + extents.overallAscent,
            guac_drawable, x, y);

    guac_drv_display_touch(guac_screen->display);

}

void guac_drv_imageglyphblt(DrawablePtr drawable, GCPtr gc, int x, int y,
        unsigned int nglyph, CharInfoPtr* char_info, pointer glyph_base) {

    /* Call framebuffer version */
    fbImageGlyphBlt(drawable, gc, x, y, nglyph, char_info, glyph_base);

    /* Copy the results from the framebuffer */
    guac_drv_copy_glyphs(drawable, gc, x, y, nglyph, char_info, glyph_base);

}

void guac_drv_polyglyphblt(DrawablePtr drawable, GCPtr gc, int x, int y,
        unsigned int nglyph, CharInfoPtr* char_info, pointer glyph_base) {

    /* Call framebuffer version */
    fbPolyGlyphBlt(drawable, gc, x, y, nglyph, char_info, glyph_base);

    /* Copy the results from the framebuffer */
    guac_drv_copy_glyphs(drawable, gc, x, y, nglyph, char_info, glyph_base);

}

