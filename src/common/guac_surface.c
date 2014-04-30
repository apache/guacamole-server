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

        /* Otherwise, do not combine */
        return 0;

    }
    
    /* Always combine with nothing */
    return 1;

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

    guac_protocol_send_size(socket, layer, w, h);
    return surface;
}

void guac_common_surface_free(guac_common_surface* surface) {
    guac_protocol_send_dispose(surface->socket, surface->layer);
    cairo_surface_destroy(surface->surface);
    cairo_destroy(surface->cairo);
    free(surface->buffer);
    free(surface);
}

void guac_common_surface_draw(guac_common_surface* surface, int x, int y, cairo_surface_t* src) {

    guac_socket* socket = surface->socket;

    int w = cairo_image_surface_get_width(src);
    int h = cairo_image_surface_get_height(src);

    /* Flush if not combining */
    if (!__guac_common_should_combine(surface, x, y, w, h)) {
        guac_protocol_send_log(socket, "Refusing to combine rect (%i, %i) %ix%i", x, y, w, h);
        guac_common_surface_flush(surface);
    }

    /* Draw with given surface */
    cairo_save(surface->cairo);
    cairo_set_source_surface(surface->cairo, src, x, y);
    cairo_rectangle(surface->cairo, x, y, w, h);
    cairo_fill(surface->cairo);
    cairo_restore(surface->cairo);

    __guac_common_mark_dirty(surface, x, y, w, h);

}

void guac_common_surface_copy(guac_common_surface* src, int sx, int sy, int w, int h,
                              guac_common_surface* dst, int dx, int dy) {

    guac_socket* socket = dst->socket;
    const guac_layer* src_layer = src->layer;
    const guac_layer* dst_layer = dst->layer;

    /* Flush if not combining */
    if (!__guac_common_should_combine(dst, dx, dy, w, h)) {
        guac_protocol_send_log(socket, "Refusing to combine rect (%i, %i) %ix%i", dx, dy, w, h);
        guac_common_surface_flush(dst);
    }

    if (dst->dirty) {
        /* STUB */
        guac_protocol_send_log(socket, "NOTE - would rewrite as PNG instead of sending copy");
        __guac_common_mark_dirty(dst, dx, dy, w, h);
    }

    else {
        guac_common_surface_flush(src);
        guac_protocol_send_copy(socket, src_layer, sx, sy, w, h, GUAC_COMP_OVER, dst_layer, dx, dy);
    }

}

void guac_common_surface_transfer(guac_common_surface* src, int sx, int sy, int w, int h,
                                  guac_transfer_function op, guac_common_surface* dst, int dx, int dy) {

    guac_socket* socket = dst->socket;
    const guac_layer* src_layer = src->layer;
    const guac_layer* dst_layer = dst->layer;

    /* Flush if not combining */
    if (!__guac_common_should_combine(dst, dx, dy, w, h)) {
        guac_protocol_send_log(socket, "Refusing to combine rect (%i, %i) %ix%i", dx, dy, w, h);
        guac_common_surface_flush(dst);
    }

    if (dst->dirty) {
        /* STUB */
        guac_protocol_send_log(socket, "NOTE - would rewrite as PNG instead of sending transfer");
        __guac_common_mark_dirty(dst, dx, dy, w, h);
    }

    else {
        guac_common_surface_flush(src);
        guac_protocol_send_transfer(socket, src_layer, sx, sy, w, h, op, dst_layer, dx, dy);
    }

}

void guac_common_surface_rect(guac_common_surface* surface,
                              int x, int y, int w, int h,
                              int red, int green, int blue) {

    guac_socket* socket = surface->socket;
    const guac_layer* layer = surface->layer;

    /* Flush if not combining */
    if (!__guac_common_should_combine(surface, x, y, w, h)) {
        guac_protocol_send_log(socket, "Refusing to combine rect (%i, %i) %ix%i", x, y, w, h);
        guac_common_surface_flush(surface);
    }

    if (surface->dirty) {
        /* STUB */
        guac_protocol_send_log(socket, "NOTE - would rewrite as PNG instead of sending rect+cfill");
        __guac_common_mark_dirty(surface, x, y, w, h);
    }

    else {
        guac_protocol_send_rect(socket, layer, x, y, w, h);
        guac_protocol_send_cfill(socket, GUAC_COMP_OVER, layer, red, green, blue, 0xFF);
    }

}

void guac_common_surface_flush(guac_common_surface* surface) {

    if (surface->dirty) {

        guac_socket* socket = surface->socket;
        const guac_layer* layer = surface->layer;
        cairo_surface_t* rect;

        guac_protocol_send_log(socket, "Flushing surface %i: (%i, %i) %ix%i",
                               surface->layer->index,
                               surface->dirty_x, surface->dirty_y,
                               surface->dirty_width, surface->dirty_height);

        /* Send PNG for dirty rect */
        rect = cairo_surface_create_for_rectangle(surface->surface,
                                                  surface->dirty_x, surface->dirty_y,
                                                  surface->dirty_width, surface->dirty_height);
        guac_protocol_send_png(socket, GUAC_COMP_OVER, layer, surface->dirty_x, surface->dirty_y, rect);
        cairo_surface_destroy(rect);

        surface->dirty = 0;

    }

}

