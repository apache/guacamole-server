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

#include <stdlib.h>

static int __guac_common_should_combine(guac_common_surface* surface, int x, int y, int w, int h) {

    if (surface->dirty) {

        int new_pixels, dirty_pixels, update_pixels;
        int new_width, new_height;

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

        new_width  = dirty_right - dirty_left;
        new_height = dirty_bottom - dirty_top;

        /* Combine if result is still small */
        if (new_width <= 64 && new_height <= 64)
            return 1;

        new_pixels  = new_width*new_height;
        dirty_pixels = surface->dirty_width*surface->dirty_height;
        update_pixels = w*h;

        /* Combine if increase in cost is likely negligible */
        if (new_pixels - dirty_pixels <= update_pixels*4) return 1;
        if (new_pixels / dirty_pixels <= 2) return 1;

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
    surface->surface = cairo_image_surface_create_for_data(surface->buffer, CAIRO_FORMAT_RGB24,
                                                           w, h, surface->stride);
    surface->cairo = cairo_create(surface->surface);

    /* Init with black */
    cairo_set_source_rgb(surface->cairo, 0, 0, 0);
    cairo_paint(surface->cairo);

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

    cairo_surface_destroy(surface->surface);
    cairo_destroy(surface->cairo);
    free(surface->buffer);
    free(surface);

}

void guac_common_surface_resize(guac_common_surface* surface, int w, int h) {

    guac_socket* socket = surface->socket;
    const guac_layer* layer = surface->layer;

    /* Create new buffer */
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, w);
    unsigned char* buffer = malloc(stride * h);

    /* Create corresponding cairo objects */
    cairo_surface_t* cairo_surface = cairo_image_surface_create_for_data(buffer, CAIRO_FORMAT_RGB24, w, h, stride);
    cairo_t* cairo = cairo_create(cairo_surface);

    /* Init with old data */
    cairo_set_source_surface(cairo, surface->surface, 0, 0);
    cairo_rectangle(cairo, 0, 0, surface->width, surface->height);
    cairo_paint(cairo);

    /* Destroy old data */
    cairo_surface_destroy(surface->surface);
    cairo_destroy(surface->cairo);
    free(surface->buffer);

    /* Assign new data */
    surface->width = w;
    surface->height = h;
    surface->stride = stride;
    surface->buffer = buffer;
    surface->surface = cairo_surface;
    surface->cairo = cairo;

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
    guac_protocol_send_size(socket, layer, w, h);
    surface->realized = 1;

}

void guac_common_surface_draw(guac_common_surface* surface, int x, int y, cairo_surface_t* src) {

    int w = cairo_image_surface_get_width(src);
    int h = cairo_image_surface_get_height(src);

    /* Flush if not combining */
    if (!__guac_common_should_combine(surface, x, y, w, h))
        guac_common_surface_flush(surface);

    /* Always defer draws */
    __guac_common_mark_dirty(surface, x, y, w, h);

    /* Update backing surface */
    cairo_save(surface->cairo);
    cairo_set_source_surface(surface->cairo, src, x, y);
    cairo_rectangle(surface->cairo, x, y, w, h);
    cairo_fill(surface->cairo);
    cairo_restore(surface->cairo);

}

void guac_common_surface_copy(guac_common_surface* src, int sx, int sy, int w, int h,
                              guac_common_surface* dst, int dx, int dy) {

    guac_socket* socket = dst->socket;
    const guac_layer* src_layer = src->layer;
    const guac_layer* dst_layer = dst->layer;

    /* Defer if combining */
    if (__guac_common_should_combine(dst, dx, dy, w, h))
        __guac_common_mark_dirty(dst, dx, dy, w, h);

    /* Otherwise, flush and draw immediately */
    else {
        guac_common_surface_flush(dst);
        guac_common_surface_flush(src);
        guac_protocol_send_copy(socket, src_layer, sx, sy, w, h, GUAC_COMP_OVER, dst_layer, dx, dy);
        dst->realized = 1;
    }

    /* Update backing surface */
    cairo_surface_flush(src->surface);
    cairo_surface_flush(dst->surface);
    __guac_common_surface_transfer(src, sx, sy, w, h, GUAC_TRANSFER_BINARY_SRC, dst, dx, dy);

}

void guac_common_surface_transfer(guac_common_surface* src, int sx, int sy, int w, int h,
                                  guac_transfer_function op, guac_common_surface* dst, int dx, int dy) {

    guac_socket* socket = dst->socket;
    const guac_layer* src_layer = src->layer;
    const guac_layer* dst_layer = dst->layer;

    /* Defer if combining */
    if (__guac_common_should_combine(dst, dx, dy, w, h))
        __guac_common_mark_dirty(dst, dx, dy, w, h);

    /* Otherwise, flush and draw immediately */
    else {
        guac_common_surface_flush(dst);
        guac_common_surface_flush(src);
        guac_protocol_send_transfer(socket, src_layer, sx, sy, w, h, op, dst_layer, dx, dy);
        dst->realized = 1;
    }

    /* Update backing surface */
    cairo_surface_flush(src->surface);
    cairo_surface_flush(dst->surface);
    __guac_common_surface_transfer(src, sx, sy, w, h, op, dst, dx, dy);

}

void guac_common_surface_rect(guac_common_surface* surface,
                              int x, int y, int w, int h,
                              int red, int green, int blue) {

    guac_socket* socket = surface->socket;
    const guac_layer* layer = surface->layer;

    /* Defer if combining */
    if (__guac_common_should_combine(surface, x, y, w, h))
        __guac_common_mark_dirty(surface, x, y, w, h);

    /* Otherwise, flush and draw immediately */
    else {
        guac_common_surface_flush(surface);
        guac_protocol_send_rect(socket, layer, x, y, w, h);
        guac_protocol_send_cfill(socket, GUAC_COMP_OVER, layer, red, green, blue, 0xFF);
        surface->realized = 1;
    }

    /* Update backing surface */
    cairo_save(surface->cairo);
    cairo_set_source_rgb(surface->cairo, red, green, blue);
    cairo_rectangle(surface->cairo, x, y, w, h);
    cairo_fill(surface->cairo);
    cairo_restore(surface->cairo);

}

void guac_common_surface_flush(guac_common_surface* surface) {

    if (surface->dirty) {

        guac_socket* socket = surface->socket;
        const guac_layer* layer = surface->layer;
        cairo_surface_t* rect;

        /* Send PNG for dirty rect */
        rect = cairo_surface_create_for_rectangle(surface->surface,
                                                  surface->dirty_x, surface->dirty_y,
                                                  surface->dirty_width, surface->dirty_height);
        guac_protocol_send_png(socket, GUAC_COMP_OVER, layer, surface->dirty_x, surface->dirty_y, rect);
        cairo_surface_destroy(rect);

        surface->dirty = 0;
        surface->realized = 1;

    }

}

