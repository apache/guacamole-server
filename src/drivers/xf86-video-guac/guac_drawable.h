
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

#ifndef __GUAC_DRAWABLE_H
#define __GUAC_DRAWABLE_H

#include "config.h"
#include "common/display.h"
#include "guac_rect.h"
#include "list.h"

#include <stdint.h>

#include <cairo/cairo.h>

#include <xorg-server.h>
#include <xf86.h>
#include <fb.h>

/**
 * Repeatedly calls the given function with the given arguments, once for each
 * clipping rectangle. For each clipping rectangle, the clipping rectangle will
 * be applied to the guac_common_surface associated with the given
 * guac_drv_drawable, the provided function will be invoked, and the clipping
 * rectangle will be unset.
 *
 * @param guac_drawable
 *     A pointer to the guac_drv_drawable associated with the destination
 *     drawable. This is the drawable which will be temporarily clipped
 *     according to the given clipping rectangles.
 *
 * @param drawable
 *     A DrawablePtr pointing to the destination drawable.
 *
 * @param clip
 *     A collection of clipping rectangles, as returned by
 *     fbGetCompositeClip().
 *
 * @param fn
 *     The function to invoke for each clipping rectangle.
 *
 * @param ...
 *     The parameters to pass to the function when it is invoked for each
 *     clipping rectangle.
 */
#define GUAC_DRV_DRAWABLE_CLIP(guac_drawable, drawable, clip, fn, ...)        \
    do {                                                                      \
                                                                              \
        /* Get underlying surface of drawable */                              \
        guac_common_surface* GUAC_DRV_DRAWABLE_CLIP__surface =                \
                guac_drawable->layer->surface;                                \
                                                                              \
        /* Get clipping rectangles */                                         \
        int GUAC_DRV_DRAWABLE_CLIP__num_rects = REGION_NUM_RECTS(clip);       \
        BoxPtr GUAC_DRV_DRAWABLE_CLIP__rect = REGION_RECTS(clip);             \
                                                                              \
        /* Get screen-absolute coordinates of drawablw */                     \
        int GUAC_DRV_DRAWABLE_CLIP__screen_x = drawable->x;                   \
        int GUAC_DRV_DRAWABLE_CLIP__screen_y = drawable->y;                   \
                                                                              \
        /* Clip operation by defined clipping path */                         \
        while (GUAC_DRV_DRAWABLE_CLIP__num_rects > 0) {                       \
                                                                              \
            {                                                                 \
                /* Get clipping rectangle bounds (screen-absolute) */         \
                int x1 = GUAC_DRV_DRAWABLE_CLIP__rect->x1;                    \
                int y1 = GUAC_DRV_DRAWABLE_CLIP__rect->y1;                    \
                int x2 = GUAC_DRV_DRAWABLE_CLIP__rect->x2;                    \
                int y2 = GUAC_DRV_DRAWABLE_CLIP__rect->y2;                    \
                                                                              \
                /* Clip draw operation (drawable-relative) */                 \
                guac_common_surface_clip(GUAC_DRV_DRAWABLE_CLIP__surface,     \
                        x1 - GUAC_DRV_DRAWABLE_CLIP__screen_x,                \
                        y1 - GUAC_DRV_DRAWABLE_CLIP__screen_y,                \
                        x2 - x1,                                              \
                        y2 - y1);                                             \
            }                                                                 \
                                                                              \
            fn(__VA_ARGS__);                                                  \
                                                                              \
            /* Reset clip for next rectangle */                               \
            guac_common_surface_reset_clip(GUAC_DRV_DRAWABLE_CLIP__surface);  \
                                                                              \
            GUAC_DRV_DRAWABLE_CLIP__rect++;                                   \
            GUAC_DRV_DRAWABLE_CLIP__num_rects--;                              \
                                                                              \
        }                                                                     \
                                                                              \
    } while (0)

/**
 * Wraps the given integer value such that it fits between 0 and max - 1,
 * inclusive. The behavior of this macro for non-integer types is undefined.
 * This is essentially equivalent to the modulo operator, except when negative
 * values are involved. For example, the following relations are true:
 *
 *     GUAC_DRV_DRAWABLE_WRAP(-6, 5) == 4
 *     GUAC_DRV_DRAWABLE_WRAP(-5, 5) == 0
 *     GUAC_DRV_DRAWABLE_WRAP(-4, 5) == 1
 *     GUAC_DRV_DRAWABLE_WRAP(-3, 5) == 2
 *     GUAC_DRV_DRAWABLE_WRAP(-2, 5) == 3
 *     GUAC_DRV_DRAWABLE_WRAP(-1, 5) == 4
 *     GUAC_DRV_DRAWABLE_WRAP(0, 5) == 0
 *     GUAC_DRV_DRAWABLE_WRAP(1, 5) == 1
 *     GUAC_DRV_DRAWABLE_WRAP(2, 5) == 2
 *     GUAC_DRV_DRAWABLE_WRAP(3, 5) == 3
 *     GUAC_DRV_DRAWABLE_WRAP(4, 5) == 4
 *     GUAC_DRV_DRAWABLE_WRAP(5, 5) == 0
 *     GUAC_DRV_DRAWABLE_WRAP(6, 5) == 1
 *
 * @param val
 *     The value to wrap between 0 and max - 1, inclusive.
 *
 * @param max
 *     The maximum against which all given values will be wrapped.
 *
 * @return
 *     The given value, wrapped as necessary to fit between 0 and max - 1,
 *     inclusive.
 */
#define GUAC_DRV_DRAWABLE_WRAP(val, max) ((((val) % (max)) + (max)) % (max));

/**
 * Draws a rectangle of color within the clipping area of the given drawable
 * and GC, logging an appropriate "STUB" message to the X.Org logs noting the
 * location of the macro invocation. The color of the rectangle is derived from
 * the location of the macro invocation, and thus should be unique for each
 * context.
 *
 * @param drawable
 *     The drawable being drawn to.
 *
 * @param gc
 *     The graphical context applying to the drawing operation.
 *
 * @param x
 *     The destination X coordinate of the draw operation within the given
 *     drawable.
 *
 * @param y
 *     The destination Y coordinate of the draw operation within the given
 *     drawable.
 *
 * @param w
 *     The width of the draw operation within the given drawable.
 *
 * @param h
 *     The height of the draw operation within the given drawable.
 */
#define GUAC_DRV_DRAWABLE_STUB_RECT(drawable, gc, x, y, w, h)                 \
    do {                                                                      \
                                                                              \
        int GUAC_DRV_DRAWABLE_STUB_RECT__color = 53 ^ __LINE__;               \
        {                                                                     \
                                                                              \
            const char* current;                                              \
                                                                              \
            for (current = __func__; *current != 0; current++) {              \
                GUAC_DRV_DRAWABLE_STUB_RECT__color *= 113;                    \
                GUAC_DRV_DRAWABLE_STUB_RECT__color ^= *current;               \
            }                                                                 \
                                                                              \
            for (current = __FILE__; *current != 0; current++) {              \
                GUAC_DRV_DRAWABLE_STUB_RECT__color *= 113;                    \
                GUAC_DRV_DRAWABLE_STUB_RECT__color ^= *current;               \
            }                                                                 \
                                                                              \
            GUAC_DRV_DRAWABLE_STUB_RECT__color &= 0xFFFFFF;                   \
                                                                              \
        }                                                                     \
                                                                              \
        xf86Msg(X_INFO, "guac: STUB %06X: %s:%d: %s()\n",                     \
                GUAC_DRV_DRAWABLE_STUB_RECT__color,                           \
                __FILE__, __LINE__, __func__);                                \
                                                                              \
        guac_drv_drawable* GUAC_DRV_DRAWABLE_STUB_RECT__guac_drawable =       \
            guac_drv_get_drawable(drawable);                                  \
                                                                              \
        GUAC_DRV_DRAWABLE_CLIP(GUAC_DRV_DRAWABLE_STUB_RECT__guac_drawable,    \
            drawable, fbGetCompositeClip(gc), guac_drv_drawable_stub,         \
            GUAC_DRV_DRAWABLE_STUB_RECT__guac_drawable, x, y, w, h,           \
            GUAC_DRV_DRAWABLE_STUB_RECT__color);                              \
                                                                              \
    } while (0)

/**
 * Fills the entire clipping area of the given drawable and GC with color,
 * logging an appropriate "STUB" message to the X.Org logs noting the location
 * of the macro invocation. The color is derived from the location of the macro
 * invocation, and thus should be unique for each context.
 *
 * @param drawable
 *     The drawable being drawn to.
 *
 * @param gc
 *     The graphical context applying to the drawing operation.
 */
#define GUAC_DRV_DRAWABLE_STUB_OP(drawable, gc)                               \
    do {                                                                      \
        GUAC_DRV_DRAWABLE_STUB_RECT(drawable, gc, 0, 0,                       \
                drawable->width, drawable->height);                           \
    } while (0)

/**
 * All supported types of drawables.
 */
typedef enum guac_drv_drawable_format {

    /**
     * 32bpp format with the high-order byte being alpha and the low-order byte
     * being blue.
     */
    GUAC_DRV_DRAWABLE_ARGB_32,

    /**
     * 24bpp format with the high-order byte being red and the low-order byte
     * being blue. This is actually a 32bpp format, but the highest-order byte
     * is unused.
     */
    GUAC_DRV_DRAWABLE_RGB_24,

    /**
     * Any as-of-yet unsupported format.
     */
    GUAC_DRV_DRAWABLE_UNSUPPORTED

} guac_drv_drawable_format;

typedef struct guac_drv_drawable {

    /**
     * The underlying graphical surface which should be replicated across all
     * connected clients.
     */
    guac_common_display_layer* layer;

    /**
     * Arbitrary data associated with this drawable.
     */
    void* data;

} guac_drv_drawable;

/**
 * Allocates a new drawable surface.
 */
guac_drv_drawable* guac_drv_drawable_alloc(guac_common_display_layer* layer);

/**
 * Frees the given drawable and any associated resources.
 */
void guac_drv_drawable_free(guac_drv_drawable* drawable);

/**
 * Resizes the given drawable to the given width and height.
 */
void guac_drv_drawable_resize(guac_drv_drawable* drawable,
        int width, int height);

/**
 * Draws a bright magenta rectangle within the given drawable, representing an
 * unimplemented operation.
 */
void guac_drv_drawable_stub(guac_drv_drawable* drawable, int dx, int dy,
        int w, int h, uint32_t color);

/**
 * Copies a rectangle of image data from a given source drawable, drawing it
 * to the given Guacamole-specific drawable via a normal image update. The
 * image data is copied from the "fb" module, and is thus independent of any
 * Guacamole-specific image state.
 */
void guac_drv_drawable_copy_fb(DrawablePtr src, int srcx, int srcy,
        int srcw, int srch, guac_drv_drawable* dst, int dstx, int dsty);

/**
 * Copies the contents of the given buffer having the given stride to the
 * given location.
 */
void guac_drv_drawable_put(guac_drv_drawable* drawable,
        char* data, guac_drv_drawable_format format, int stride,
        int dx, int dy, int w, int h);

/**
 * Copies the contents of the given drawable to the given location.
 */
void guac_drv_drawable_copy(
        guac_drv_drawable* src, int srcx, int srcy, int w, int h,
        guac_drv_drawable* dst, int dstx, int dsty);

/**
 * Fills the given rectangular region of the given drawable with the provided
 * color.
 */
void guac_drv_drawable_crect(guac_drv_drawable* drawable, int x, int y,
        int w, int h, int fill);

/**
 * Fills the given rectangular region of the given drawable with the provided
 * pattern.
 */
void guac_drv_drawable_drect(guac_drv_drawable* drawable, int x, int y,
        int w, int h, guac_drv_drawable* fill);

/**
 * Change the opacity of the given drawable.
 */
void guac_drv_drawable_shade(guac_drv_drawable* drawable, int opacity);

/**
 * Move the given drawable to the given location.
 */
void guac_drv_drawable_move(guac_drv_drawable* drawable, int x, int y);

/**
 * Change the stacking order of the given drawable.
 */
void guac_drv_drawable_stack(guac_drv_drawable* drawable, int z);

/**
 * Change the parent of the given drawable.
 */
void guac_drv_drawable_reparent(guac_drv_drawable* drawable,
        guac_drv_drawable* parent);

#endif

