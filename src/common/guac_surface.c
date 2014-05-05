/*
 * Copyright (C) 2014 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"
#include "guac_surface.h"

#include <cairo/cairo.h>
#include <guacamole/layer.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


/**
 * The width of an update which should be considered negible and thus
 * trivial overhead compared ot the cost of two updates.
 */
#define GUAC_SURFACE_NEGLIGIBLE_WIDTH 64

/**
 * The height of an update which should be considered negible and thus
 * trivial overhead compared ot the cost of two updates.
 */
#define GUAC_SURFACE_NEGLIGIBLE_HEIGHT 64

/**
 * The proportional increase in cost contributed by transfer and processing of
 * image data, compared to processing an equivalent amount of client-side
 * data.
 */
#define GUAC_SURFACE_DATA_FACTOR 16

/**
 * The base cost of every update. Each update should be considered to have
 * this starting cost, plus any additional cost estimated from its
 * content.
 */
#define GUAC_SURFACE_BASE_COST 4096

/**
 * An increase in cost is negligible if it is less than
 * 1/GUAC_SURFACE_NEGLIGIBLE_INCREASE of the old cost.
 */
#define GUAC_SURFACE_NEGLIGIBLE_INCREASE 4

/**
 * If combining an update because it appears to be follow a fill pattern,
 * the combined cost must not exceed
 * GUAC_SURFACE_FILL_PATTERN_FACTOR * (total uncombined cost).
 */
#define GUAC_SURFACE_FILL_PATTERN_FACTOR 3

/* Define cairo_format_stride_for_width() if missing */
#ifndef HAVE_CAIRO_FORMAT_STRIDE_FOR_WIDTH
#define cairo_format_stride_for_width(format, width) (width*4)
#endif

/**
 * Updates the coordinates of the given rectangle to be within the bounds of the given surface.
 * 
 * @param surface The surface to use for clipping.
 * @param x The X coordinate of the rectangle to clip.
 * @param y The Y coordinate of the rectangle to clip.
 * @param w The width of the rectangle to clip.
 * @param h The height of the rectangle to clip.
 * @param sx The X coordinate of the source rectangle, if any.
 * @param sy The Y coordinate of the source rectangle, if any.
 */
static void __guac_common_bound_rect(guac_common_surface* surface, int* x, int* y, int* w, int* h,
                                     int* sx, int* sy) {

    int bounds_left   = surface->bounds_x;
    int bounds_top    = surface->bounds_y;
    int bounds_right  = bounds_left + surface->bounds_width;
    int bounds_bottom = bounds_top  + surface->bounds_height;

    /* Get rect coordinates */
    int clipped_left   = *x;
    int clipped_top    = *y;
    int clipped_right  = clipped_left + *w;
    int clipped_bottom = clipped_top  + *h;

    /* Clip to bounds */
    if (clipped_left   < bounds_left)   clipped_left   = bounds_left;
    if (clipped_top    < bounds_top)    clipped_top    = bounds_top;
    if (clipped_right  > bounds_right)  clipped_right  = bounds_right;
    if (clipped_bottom > bounds_bottom) clipped_bottom = bounds_bottom;

    /* Update source X/Y if given */
    if (sx != NULL) *sx += clipped_left - *x;
    if (sy != NULL) *sy += clipped_top  - *y;

    /* Store new rect dimensions */
    *x = clipped_left;
    *y = clipped_top;
    *w = clipped_right  - clipped_left;
    *h = clipped_bottom - clipped_top;

}

/**
 * Returns whether the given rectangle should be combined into the existing
 * dirty rectangle, to be eventually flushed as a "png" instruction.
 *
 * @param surface The surface to be queried.
 * @param x The X coordinate of the upper-left corner of the update rectangle.
 * @param y The Y coordinate of the upper-left corner of the update rectangle.
 * @param w The width of the update rectangle.
 * @param h The height of the update rectangle.
 * @param rect_only Non-zero if this update, by its nature, contains only
 *                  metainformation about the update's rectangle, zero if
 *                  the update also contains image data.
 * @return Non-zero if the update should be combined with any existing update,
 *         zero otherwise.
 */
static int __guac_common_should_combine(guac_common_surface* surface, int x, int y, int w, int h, int rect_only) {

    if (surface->dirty) {

        int combined_cost, dirty_cost, update_cost;
        int combined_width, combined_height;

        /* Calculate extents of existing dirty rect */
        int dirty_left   = surface->dirty_x;
        int dirty_top    = surface->dirty_y;
        int dirty_right  = dirty_left + surface->dirty_width;
        int dirty_bottom = dirty_top  + surface->dirty_height;

        /* Calculate missing extents of given new rect */
        int right  = x + w;
        int bottom = y + h;

        /* Update minimums */
        if (x      < dirty_left)   dirty_left   = x;
        if (y      < dirty_top)    dirty_top    = y;
        if (right  > dirty_right)  dirty_right  = right;
        if (bottom > dirty_bottom) dirty_bottom = bottom;

        combined_width  = dirty_right - dirty_left;
        combined_height = dirty_bottom - dirty_top;

        /* Combine if result is still small */
        if (combined_width <= GUAC_SURFACE_NEGLIGIBLE_WIDTH && combined_height <= GUAC_SURFACE_NEGLIGIBLE_HEIGHT)
            return 1;

        /* Estimate costs of the existing update, new update, and both combined */
        combined_cost = GUAC_SURFACE_BASE_COST + combined_width * combined_height;
        dirty_cost    = GUAC_SURFACE_BASE_COST + surface->dirty_width * surface->dirty_height;
        update_cost   = GUAC_SURFACE_BASE_COST + w*h;

        /* Reduce cost if no image data */
        if (rect_only)
            update_cost /= GUAC_SURFACE_DATA_FACTOR;

        /* Combine if cost estimate shows benefit */
        if (combined_cost <= update_cost + dirty_cost)
            return 1;

        /* Combine if increase in cost is negligible */
        if (combined_cost - dirty_cost <= dirty_cost / GUAC_SURFACE_NEGLIGIBLE_INCREASE)
            return 1;

        if (combined_cost - update_cost <= update_cost / GUAC_SURFACE_NEGLIGIBLE_INCREASE)
            return 1;

        /* Combine if we anticipate further updates, as this update follows a common fill pattern */
        if (x == surface->dirty_x && y == surface->dirty_y + surface->dirty_height) {
            if (combined_cost <= (dirty_cost + update_cost) * GUAC_SURFACE_FILL_PATTERN_FACTOR)
                return 1;
        }

    }
    
    /* Otherwise, do not combine */
    return 0;

}

static void __guac_common_mark_dirty(guac_common_surface* surface, int x, int y, int w, int h) {

    /* If already dirty, update existing rect */
    if (surface->dirty) {

        /* Calculate extents of existing dirty rect */
        int dirty_left   = surface->dirty_x;
        int dirty_top    = surface->dirty_y;
        int dirty_right  = dirty_left + surface->dirty_width;
        int dirty_bottom = dirty_top  + surface->dirty_height;

        /* Calculate missing extents of given new rect */
        int right  = x + w;
        int bottom = y + h;

        /* Update minimums */
        if (x      < dirty_left)   dirty_left   = x;
        if (y      < dirty_top)    dirty_top    = y;
        if (right  > dirty_right)  dirty_right  = right;
        if (bottom > dirty_bottom) dirty_bottom = bottom;

        /* Commit rect */
        surface->dirty_x      = dirty_left;
        surface->dirty_y      = dirty_top;
        surface->dirty_width  = dirty_right  - dirty_left;
        surface->dirty_height = dirty_bottom - dirty_top;

    }

    /* Otherwise init dirty rect */
    else {
        surface->dirty_x = x;
        surface->dirty_y = y;
        surface->dirty_width = w;
        surface->dirty_height = h;
        surface->dirty = 1;
    }

}

static void __guac_common_surface_transfer_int(guac_transfer_function op, uint32_t* src, uint32_t* dst) {

    switch (op) {

        case GUAC_TRANSFER_BINARY_BLACK:
            *dst = 0xFF000000;
            break;

        case GUAC_TRANSFER_BINARY_WHITE:
            *dst = 0xFFFFFFFF;
            break;

        case GUAC_TRANSFER_BINARY_SRC:
            *dst = *src;
            break;

        case GUAC_TRANSFER_BINARY_DEST:
            /* NOP */
            break;

        case GUAC_TRANSFER_BINARY_NSRC:
            *dst = ~(*src);
            break;

        case GUAC_TRANSFER_BINARY_NDEST:
            *dst = ~(*dst);
            break;

        case GUAC_TRANSFER_BINARY_AND:
            *dst = (*dst) & (*src);
            break;

        case GUAC_TRANSFER_BINARY_NAND:
            *dst = ~((*dst) & (*src));
            break;

        case GUAC_TRANSFER_BINARY_OR:
            *dst = (*dst) | (*src);
            break;

        case GUAC_TRANSFER_BINARY_NOR:
            *dst = ~((*dst) | (*src));
            break;

        case GUAC_TRANSFER_BINARY_XOR:
            *dst = (*dst) ^ (*src);
            break;

        case GUAC_TRANSFER_BINARY_XNOR:
            *dst = ~((*dst) ^ (*src));
            break;

        case GUAC_TRANSFER_BINARY_NSRC_AND:
            *dst = (*dst) & ~(*src);
            break;

        case GUAC_TRANSFER_BINARY_NSRC_NAND:
            *dst = ~((*dst) & ~(*src));
            break;

        case GUAC_TRANSFER_BINARY_NSRC_OR:
            *dst = (*dst) | ~(*src);
            break;

        case GUAC_TRANSFER_BINARY_NSRC_NOR:
            *dst = ~((*dst) | ~(*src));
            break;

    }
}

static void __guac_common_surface_rect(guac_common_surface* dst, int dx, int dy, int w, int h,
                                       int red, int green, int blue) {

    int x, y;

    int dst_stride;
    unsigned char* dst_buffer;

    uint32_t color = 0xFF000000 | (red << 16) | (green << 8) | blue;

    dst_stride = dst->stride;
    dst_buffer = dst->buffer + dst_stride*dy + 4*dx;

    /* For each row */
    for (y=0; y<h; y++) {

        uint32_t* dst_current = (uint32_t*) dst_buffer;

        /* Set row */
        for (x=0; x<w; x++) {
            *dst_current = color;
            dst_current++;
        }

        /* Next row */
        dst_buffer += dst_stride;

    }

}

static void __guac_common_surface_put(unsigned char* src_buffer, int src_stride,
                                      int sx, int sy, int w, int h,
                                      guac_common_surface* dst, int dx, int dy,
                                      int opaque) {

    unsigned char* dst_buffer = dst->buffer;
    int dst_stride = dst->stride;

    int x, y;

    src_buffer += src_stride*sy + 4*sx;
    dst_buffer += dst_stride*dy + 4*dx;

    /* For each row */
    for (y=0; y<h; y++) {

        uint32_t* src_current = (uint32_t*) src_buffer;
        uint32_t* dst_current = (uint32_t*) dst_buffer;

        /* Copy row */
        for (x=0; x<w; x++) {

            if (opaque || (*src_current & 0xFF000000))
                *dst_current = *src_current | 0xFF000000;

            src_current++;
            dst_current++;
        }

        /* Next row */
        src_buffer += src_stride;
        dst_buffer += dst_stride;

    }

}

static void __guac_common_surface_fill_mask(unsigned char* src_buffer, int src_stride,
                                            int sx, int sy, int w, int h,
                                            guac_common_surface* dst, int dx, int dy,
                                            int red, int green, int blue) {

    unsigned char* dst_buffer = dst->buffer;
    int dst_stride = dst->stride;

    uint32_t color = 0xFF000000 | (red << 16) | (green << 8) | blue;
    int x, y;

    src_buffer += src_stride*sy + 4*sx;
    dst_buffer += dst_stride*dy + 4*dx;

    /* For each row */
    for (y=0; y<h; y++) {

        uint32_t* src_current = (uint32_t*) src_buffer;
        uint32_t* dst_current = (uint32_t*) dst_buffer;

        /* Stencil row */
        for (x=0; x<w; x++) {

            /* Fill with color if opaque */
            if (*src_current & 0xFF000000)
                *dst_current = color;

            src_current++;
            dst_current++;
        }

        /* Next row */
        src_buffer += src_stride;
        dst_buffer += dst_stride;

    }

}

static void __guac_common_surface_transfer(guac_common_surface* src, int sx, int sy, int w, int h,
                                           guac_transfer_function op, guac_common_surface* dst, int dx, int dy) {

    unsigned char* src_buffer = src->buffer;
    unsigned char* dst_buffer = dst->buffer;

    int x, y;
    int src_stride, dst_stride;
    int step = 1;

    /* Copy forwards only if destination is in a different surface or is before source */
    if (src != dst || dy < sy || (dy == sy && dx < sx)) {
        src_buffer += src->stride*sy + 4*sx;
        dst_buffer += dst->stride*dy + 4*dx;
        src_stride = src->stride;
        dst_stride = dst->stride;
        step = 1;
    }

    /* Otherwise, copy backwards */
    else {
        src_buffer += src->stride*(sy+h-1) + 4*(sx+w-1);
        dst_buffer += dst->stride*(dy+h-1) + 4*(dx+w-1);
        src_stride = -src->stride;
        dst_stride = -dst->stride;
        step = -1;
    }

    /* For each row */
    for (y=0; y<h; y++) {

        uint32_t* src_current = (uint32_t*) src_buffer;
        uint32_t* dst_current = (uint32_t*) dst_buffer;

        /* Transfer each pixel in row */
        for (x=0; x<w; x++) {
            __guac_common_surface_transfer_int(op, src_current, dst_current);
            src_current += step;
            dst_current += step;
        }

        /* Next row */
        src_buffer += src_stride;
        dst_buffer += dst_stride;

    }

}

guac_common_surface* guac_common_surface_alloc(guac_socket* socket, const guac_layer* layer, int w, int h) {

    /* Init surface */
    guac_common_surface* surface = malloc(sizeof(guac_common_surface));
    surface->layer = layer;
    surface->socket = socket;
    surface->width = w;
    surface->height = h;
    surface->dirty = 0;

    /* Create corresponding Cairo surface */
    surface->stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, w);
    surface->buffer = malloc(surface->stride * h);

    /* Reset clipping rect */
    guac_common_surface_reset_clip(surface);

    /* Init with black */
    __guac_common_surface_rect(surface, 0, 0, w, h, 0x00, 0x00, 0x00); 

    /* Layers must initially exist */
    if (layer->index >= 0) {
        guac_protocol_send_size(socket, layer, w, h);
        surface->realized = 1;
    }

    /* Defer creation of buffers */
    else
        surface->realized = 0;

    return surface;
}

void guac_common_surface_free(guac_common_surface* surface) {

    /* Only dispose of surface if it exists */
    if (surface->realized)
        guac_protocol_send_dispose(surface->socket, surface->layer);

    free(surface->buffer);
    free(surface);

}

void guac_common_surface_resize(guac_common_surface* surface, int w, int h) {

    guac_socket* socket = surface->socket;
    const guac_layer* layer = surface->layer;

    /* Copy old surface data */
    unsigned char* old_buffer = surface->buffer;
    int old_stride = surface->stride;
    int old_width = surface->width;
    int old_height = surface->height;

    /* Create new buffer */
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, w);
    unsigned char* buffer = malloc(stride * h);

    /* Assign new data */
    surface->width = w;
    surface->height = h;
    surface->stride = stride;
    surface->buffer = buffer;

    /* Reset clipping rect */
    guac_common_surface_reset_clip(surface);

    /* Init with old data */
    if (old_width > w)  old_width = w;
    if (old_height > h) old_height = h;
    __guac_common_surface_put(old_buffer, old_stride, 0, 0, old_width, old_height,
                              surface, 0, 0, 1);

    /* Free old data */
    free(old_buffer);

    /* Clip dirty rect */
    if (surface->dirty) {

        int dirty_left = surface->dirty_x;
        int dirty_top  = surface->dirty_y;

        /* Clip dirty rect if still on screen */
        if (dirty_left < w && dirty_top < h) {

            int dirty_right  = dirty_left + surface->dirty_width;
            int dirty_bottom = dirty_top  + surface->dirty_height;

            if (dirty_right  > w) dirty_right  = w;
            if (dirty_bottom > h) dirty_bottom = h;

            surface->dirty_width  = dirty_right  - dirty_left;
            surface->dirty_height = dirty_bottom - dirty_top;

        }

        /* Otherwise, no longer dirty */
        else
            surface->dirty = 0;

    }

    /* Update Guacamole layer */
    if (surface->realized)
        guac_protocol_send_size(socket, layer, w, h);

}

void guac_common_surface_draw(guac_common_surface* surface, int x, int y, cairo_surface_t* src) {

    unsigned char* buffer = cairo_image_surface_get_data(src);
    cairo_format_t format = cairo_image_surface_get_format(src);
    int stride = cairo_image_surface_get_stride(src);
    int w = cairo_image_surface_get_width(src);
    int h = cairo_image_surface_get_height(src);

    int sx = 0;
    int sy = 0;

    /* Clip operation */
    __guac_common_bound_rect(surface, &x, &y, &w, &h, &sx, &sy);
    if (w <= 0 || h <= 0)
        return;

    /* Flush if not combining */
    if (!__guac_common_should_combine(surface, x, y, w, h, 0))
        guac_common_surface_flush(surface);

    /* Always defer draws */
    __guac_common_mark_dirty(surface, x, y, w, h);

    /* Update backing surface */
    __guac_common_surface_put(buffer, stride, sx, sy, w, h, surface, x, y, format != CAIRO_FORMAT_ARGB32);

}

void guac_common_surface_paint(guac_common_surface* surface, int x, int y, cairo_surface_t* src,
                               int red, int green, int blue) {

    unsigned char* buffer = cairo_image_surface_get_data(src);
    int stride = cairo_image_surface_get_stride(src);
    int w = cairo_image_surface_get_width(src);
    int h = cairo_image_surface_get_height(src);

    int sx = 0;
    int sy = 0;

    /* Clip operation */
    __guac_common_bound_rect(surface, &x, &y, &w, &h, &sx, &sy);
    if (w <= 0 || h <= 0)
        return;

    /* Flush if not combining */
    if (!__guac_common_should_combine(surface, x, y, w, h, 0))
        guac_common_surface_flush(surface);

    /* Always defer draws */
    __guac_common_mark_dirty(surface, x, y, w, h);

    /* Update backing surface */
    __guac_common_surface_fill_mask(buffer, stride, sx, sy, w, h, surface, x, y, red, green, blue);

}

void guac_common_surface_copy(guac_common_surface* src, int sx, int sy, int w, int h,
                              guac_common_surface* dst, int dx, int dy) {

    guac_socket* socket = dst->socket;
    const guac_layer* src_layer = src->layer;
    const guac_layer* dst_layer = dst->layer;

    /* Clip operation */
    __guac_common_bound_rect(dst, &dx, &dy, &w, &h, &sx, &sy);
    if (w <= 0 || h <= 0)
        return;

    /* Defer if combining */
    if (__guac_common_should_combine(dst, dx, dy, w, h, 1))
        __guac_common_mark_dirty(dst, dx, dy, w, h);

    /* Otherwise, flush and draw immediately */
    else {
        guac_common_surface_flush(dst);
        guac_common_surface_flush(src);
        guac_protocol_send_copy(socket, src_layer, sx, sy, w, h, GUAC_COMP_OVER, dst_layer, dx, dy);
        dst->realized = 1;
    }

    /* Update backing surface */
    __guac_common_surface_transfer(src, sx, sy, w, h, GUAC_TRANSFER_BINARY_SRC, dst, dx, dy);

}

void guac_common_surface_transfer(guac_common_surface* src, int sx, int sy, int w, int h,
                                  guac_transfer_function op, guac_common_surface* dst, int dx, int dy) {

    guac_socket* socket = dst->socket;
    const guac_layer* src_layer = src->layer;
    const guac_layer* dst_layer = dst->layer;

    /* Clip operation */
    __guac_common_bound_rect(dst, &dx, &dy, &w, &h, &sx, &sy);
    if (w <= 0 || h <= 0)
        return;

    /* Defer if combining */
    if (__guac_common_should_combine(dst, dx, dy, w, h, 1))
        __guac_common_mark_dirty(dst, dx, dy, w, h);

    /* Otherwise, flush and draw immediately */
    else {
        guac_common_surface_flush(dst);
        guac_common_surface_flush(src);
        guac_protocol_send_transfer(socket, src_layer, sx, sy, w, h, op, dst_layer, dx, dy);
        dst->realized = 1;
    }

    /* Update backing surface */
    __guac_common_surface_transfer(src, sx, sy, w, h, op, dst, dx, dy);

}

void guac_common_surface_rect(guac_common_surface* surface,
                              int x, int y, int w, int h,
                              int red, int green, int blue) {

    guac_socket* socket = surface->socket;
    const guac_layer* layer = surface->layer;

    /* Clip operation */
    __guac_common_bound_rect(surface, &x, &y, &w, &h, NULL, NULL);
    if (w <= 0 || h <= 0)
        return;

    /* Defer if combining */
    if (__guac_common_should_combine(surface, x, y, w, h, 1))
        __guac_common_mark_dirty(surface, x, y, w, h);

    /* Otherwise, flush and draw immediately */
    else {
        guac_common_surface_flush(surface);
        guac_protocol_send_rect(socket, layer, x, y, w, h);
        guac_protocol_send_cfill(socket, GUAC_COMP_OVER, layer, red, green, blue, 0xFF);
        surface->realized = 1;
    }

    /* Update backing surface */
    __guac_common_surface_rect(surface, x, y, w, h, red, green, blue);

}

void guac_common_surface_clip(guac_common_surface* surface, int x, int y, int w, int h) {

    /* Calculate extents of existing bounds rect */
    int bounds_left   = surface->bounds_x;
    int bounds_top    = surface->bounds_y;
    int bounds_right  = bounds_left + surface->bounds_width;
    int bounds_bottom = bounds_top  + surface->bounds_height;

    /* Calculate missing extents of given new rect */
    int right  = x + w;
    int bottom = y + h;

    /* Update maximums */
    if (x      > bounds_left)   bounds_left   = x;
    if (y      > bounds_top)    bounds_top    = y;
    if (right  < bounds_right)  bounds_right  = right;
    if (bottom < bounds_bottom) bounds_bottom = bottom;

    /* Commit rect */
    surface->bounds_x      = bounds_left;
    surface->bounds_y      = bounds_top;
    surface->bounds_width  = bounds_right  - bounds_left;
    surface->bounds_height = bounds_bottom - bounds_top;

    /* Clamp dimensions at 0x0 */
    if (surface->bounds_width  < 0) surface->bounds_width  = 0;
    if (surface->bounds_height < 0) surface->bounds_height = 0;

}

void guac_common_surface_reset_clip(guac_common_surface* surface) {
    surface->bounds_x = 0;
    surface->bounds_y = 0;
    surface->bounds_width = surface->width;
    surface->bounds_height = surface->height;
}

void guac_common_surface_flush(guac_common_surface* surface) {

    if (surface->dirty) {

        guac_socket* socket = surface->socket;
        const guac_layer* layer = surface->layer;
        unsigned char* buffer = surface->buffer + surface->dirty_y * surface->stride + surface->dirty_x * 4;

        cairo_surface_t* rect = cairo_image_surface_create_for_data(buffer, CAIRO_FORMAT_RGB24,
                                                                    surface->dirty_width, surface->dirty_height,
                                                                    surface->stride);

        /* Send PNG for dirty rect */
        guac_protocol_send_png(socket, GUAC_COMP_OVER, layer, surface->dirty_x, surface->dirty_y, rect);
        cairo_surface_destroy(rect);

        surface->dirty = 0;
        surface->realized = 1;

    }

}

