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

#ifndef __GUAC_COMMON_SURFACE_H
#define __GUAC_COMMON_SURFACE_H

#include "config.h"
#include "guac_rect.h"

#include <cairo/cairo.h>
#include <guacamole/client.h>
#include <guacamole/layer.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>

/**
 * The maximum number of updates to allow within the PNG queue.
 */
#define GUAC_COMMON_SURFACE_QUEUE_SIZE 256

/**
 * Representation of a PNG update, having a rectangle of image data (stored
 * elsewhere) and a flushed/not-flushed state.
 */
typedef struct guac_common_surface_png_rect {

    /**
     * Whether this rectangle has been flushed.
     */
    int flushed;

    /**
     * The rectangle containing the PNG update.
     */
    guac_common_rect rect;

} guac_common_surface_png_rect;

/**
 * Surface which backs a Guacamole buffer or layer, automatically
 * combining updates when possible.
 */
typedef struct guac_common_surface {

    /**
     * The layer this surface will draw to.
     */
    const guac_layer* layer;

    /**
     * The client associated with this surface.
     */
    guac_client* client;

    /**
     * The socket to send instructions on when flushing.
     */
    guac_socket* socket;

    /**
     * The width of this layer, in pixels.
     */
    int width;

    /**
     * The height of this layer, in pixels.
     */
    int height;

    /**
     * The size of each image row, in bytes.
     */
    int stride;

    /**
     * The underlying buffer of the Cairo surface.
     */
    unsigned char* buffer;

    /**
     * Non-zero if this surface is dirty and needs to be flushed, 0 otherwise.
     */
    int dirty;

    /**
     * The dirty rectangle.
     */
    guac_common_rect dirty_rect;

    /**
     * Whether the surface actually exists on the client.
     */
    int realized;

    /**
     * Whether drawing operations are currently clipped by the clipping
     * rectangle.
     */
    int clipped;

    /**
     * The clipping rectangle.
     */
    guac_common_rect clip_rect;

    /**
     * The number of updates in the PNG queue.
     */
    int png_queue_length;

    /**
     * All queued PNG updates.
     */
    guac_common_surface_png_rect png_queue[GUAC_COMMON_SURFACE_QUEUE_SIZE];

} guac_common_surface;

/**
 * Allocates a new guac_common_surface, assigning it to the given layer.
 *
 * @param client
 *     The client associated with the given layer.
 *
 * @param socket
 *     The socket to send instructions on when flushing.
 *
 * @param layer
 *     The layer to associate with the new surface.
 *
 * @param w
 *     The width of the surface.
 *
 * @param h
 *     The height of the surface.
 *
 * @return
 *     A newly-allocated guac_common_surface.
 */
guac_common_surface* guac_common_surface_alloc(guac_client* client,
        guac_socket* socket, const guac_layer* layer, int w, int h);

/**
 * Frees the given guac_common_surface. Beware that this will NOT free any
 * associated layers, which must be freed manually.
 *
 * @param surface The surface to free.
 */
void guac_common_surface_free(guac_common_surface* surface);

 /**
 * Resizes the given surface to the given size.
 *
 * @param surface The surface to resize.
 * @param w The width of the surface.
 * @param h The height of the surface.
 */
void guac_common_surface_resize(guac_common_surface* surface, int w, int h);

/**
 * Draws the given data to the given guac_common_surface.
 *
 * @param surface The surface to draw to.
 * @param x The X coordinate of the draw location.
 * @param y The Y coordinate of the draw location.
 * @param src The Cairo surface to retrieve data from.
 */
void guac_common_surface_draw(guac_common_surface* surface, int x, int y, cairo_surface_t* src);

/**
 * Paints to the given guac_common_surface using the given data as a stencil,
 * filling opaque regions with the specified color, and leaving transparent
 * regions untouched.
 *
 * @param surface The surface to draw to.
 * @param x The X coordinate of the draw location.
 * @param y The Y coordinate of the draw location.
 * @param src The Cairo surface to retrieve data from.
 * @param red The red component of the fill color.
 * @param green The green component of the fill color.
 * @param blue The blue component of the fill color.
 */
void guac_common_surface_paint(guac_common_surface* surface, int x, int y, cairo_surface_t* src,
                              int red, int green, int blue);

/**
 * Copies a rectangle of data between two surfaces.
 *
 * @param src The source surface.
 * @param sx The X coordinate of the upper-left corner of the source rect.
 * @param sy The Y coordinate of the upper-left corner of the source rect.
 * @param w The width of the source rect.
 * @param h The height of the source rect.
 * @param dst The destination surface.
 * @param dx The X coordinate of the upper-left corner of the destination rect.
 * @param dy The Y coordinate of the upper-left corner of the destination rect.
 */
void guac_common_surface_copy(guac_common_surface* src, int sx, int sy, int w, int h,
                              guac_common_surface* dst, int dx, int dy);

/**
 * Transfers a rectangle of data between two surfaces.
 *
 * @param src The source surface.
 * @param sx The X coordinate of the upper-left corner of the source rect.
 * @param sy The Y coordinate of the upper-left corner of the source rect.
 * @param w The width of the source rect.
 * @param h The height of the source rect.
 * @param op The transfer function.
 * @param dst The destination surface.
 * @param dx The X coordinate of the upper-left corner of the destination rect.
 * @param dy The Y coordinate of the upper-left corner of the destination rect.
 */
void guac_common_surface_transfer(guac_common_surface* src, int sx, int sy, int w, int h,
                                  guac_transfer_function op, guac_common_surface* dst, int dx, int dy);

/**
 * Draws a solid color rectangle at the given coordinates on the given surface.
 *
 * @param surface The surface to draw upon.
 * @param x The X coordinate of the upper-left corner of the rectangle.
 * @param y The Y coordinate of the upper-left corner of the rectangle.
 * @param w The width of the rectangle.
 * @param h The height of the rectangle.
 * @param red The red component of the color of the rectangle.
 * @param green The green component of the color of the rectangle.
 * @param blue The blue component of the color of the rectangle.
 */
void guac_common_surface_rect(guac_common_surface* surface,
                              int x, int y, int w, int h,
                              int red, int green, int blue);

/**
 * Given the coordinates and dimensions of a rectangle, clips all future
 * operations within that rectangle.
 *
 * @param surface The surface whose clipping rectangle should be changed.
 * @param x The X coordinate of the upper-left corner of the bounding rectangle.
 * @param y The Y coordinate of the upper-left corner of the bounding rectangle.
 * @param w The width of the bounding rectangle.
 * @param h The height of the bounding rectangle.
 */
void guac_common_surface_clip(guac_common_surface* surface, int x, int y, int w, int h);

/**
 * Resets the clipping rectangle, allowing drawing operations throughout the
 * entire surface.
 *
 * @param surface The surface whose clipping rectangle should be reset.
 */
void guac_common_surface_reset_clip(guac_common_surface* surface);

/**
 * Flushes the given surface, drawing any pending operations on the remote
 * display.
 *
 * @param surface The surface to flush.
 */
void guac_common_surface_flush(guac_common_surface* surface);

/**
 * Schedules a deferred flush of the given surface. This will not immediately
 * flush the surface to the client. Instead, the result of the flush is
 * added to a queue which is reinspected and combined (if possible) with other
 * deferred flushes during the call to guac_common_surface_flush().
 *
 * @param surface The surface to flush.
 */
void guac_common_surface_flush_deferred(guac_common_surface* surface);

#endif

