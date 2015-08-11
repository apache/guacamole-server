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
#include "guac_rect.h"
#include "guac_surface.h"

#include <cairo/cairo.h>
#include <guacamole/client.h>
#include <guacamole/layer.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>

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
 * Updates the coordinates of the given rectangle to be within the bounds of
 * the given surface.
 *
 * @param surface The surface to use for clipping.
 * @param rect The rectangle to clip.
 * @param sx The X coordinate of the source rectangle, if any.
 * @param sy The Y coordinate of the source rectangle, if any.
 */
static void __guac_common_bound_rect(guac_common_surface* surface,
        guac_common_rect* rect, int* sx, int* sy) {

    guac_common_rect bounds_rect = {
        .x = 0,
        .y = 0,
        .width  = surface->width,
        .height = surface->height
    };

    int orig_x = rect->x;
    int orig_y = rect->y;

    guac_common_rect_constrain(rect, &bounds_rect);

    /* Update source X/Y if given */
    if (sx != NULL) *sx += rect->x - orig_x;
    if (sy != NULL) *sy += rect->y - orig_y;

}

/**
 * Updates the coordinates of the given rectangle to be within the clipping
 * rectangle of the given surface, which must always be within the bounding
 * rectangle of the given surface.
 *
 * @param surface The surface to use for clipping.
 * @param rect The rectangle to clip.
 * @param sx The X coordinate of the source rectangle, if any.
 * @param sy The Y coordinate of the source rectangle, if any.
 */
static void __guac_common_clip_rect(guac_common_surface* surface,
        guac_common_rect* rect, int* sx, int* sy) {

    int orig_x = rect->x;
    int orig_y = rect->y;

    /* Just bound within surface if no clipping rectangle applied */
    if (!surface->clipped) {
        __guac_common_bound_rect(surface, rect, sx, sy);
        return;
    }

    guac_common_rect_constrain(rect, &surface->clip_rect);

    /* Update source X/Y if given */
    if (sx != NULL) *sx += rect->x - orig_x;
    if (sy != NULL) *sy += rect->y - orig_y;

}

/**
 * Returns whether the given rectangle should be combined into the existing
 * dirty rectangle, to be eventually flushed as a "png" instruction.
 *
 * @param surface The surface to be queried.
 * @param rect The update rectangle.
 * @param rect_only Non-zero if this update, by its nature, contains only
 *                  metainformation about the update's rectangle, zero if
 *                  the update also contains image data.
 * @return Non-zero if the update should be combined with any existing update,
 *         zero otherwise.
 */
static int __guac_common_should_combine(guac_common_surface* surface, const guac_common_rect* rect, int rect_only) {

    if (surface->dirty) {

        int combined_cost, dirty_cost, update_cost;

        /* Simulate combination */
        guac_common_rect combined = surface->dirty_rect;
        guac_common_rect_extend(&combined, rect);

        /* Combine if result is still small */
        if (combined.width <= GUAC_SURFACE_NEGLIGIBLE_WIDTH && combined.height <= GUAC_SURFACE_NEGLIGIBLE_HEIGHT)
            return 1;

        /* Estimate costs of the existing update, new update, and both combined */
        combined_cost = GUAC_SURFACE_BASE_COST + combined.width * combined.height;
        dirty_cost    = GUAC_SURFACE_BASE_COST + surface->dirty_rect.width * surface->dirty_rect.height;
        update_cost   = GUAC_SURFACE_BASE_COST + rect->width * rect->height;

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
        if (rect->x == surface->dirty_rect.x && rect->y == surface->dirty_rect.y + surface->dirty_rect.height) {
            if (combined_cost <= (dirty_cost + update_cost) * GUAC_SURFACE_FILL_PATTERN_FACTOR)
                return 1;
        }

    }
    
    /* Otherwise, do not combine */
    return 0;

}

/**
 * Expands the dirty rect of the given surface to contain the rect described by the given
 * coordinates.
 *
 * @param surface The surface to mark as dirty.
 * @param rect The rectangle of the update which is dirtying the surface.
 */
static void __guac_common_mark_dirty(guac_common_surface* surface, const guac_common_rect* rect) {

    /* Ignore empty rects */
    if (rect->width <= 0 || rect->height <= 0)
        return;

    /* If already dirty, update existing rect */
    if (surface->dirty)
        guac_common_rect_extend(&surface->dirty_rect, rect);

    /* Otherwise init dirty rect */
    else {
        surface->dirty_rect = *rect;
        surface->dirty = 1;
    }

}

/**
 * Flushes the PNG update currently described by the dirty rectangle within the
 * given surface to that surface's PNG queue. There MUST be space within the
 * queue.
 *
 * @param surface The surface to flush.
 */
static void __guac_common_surface_flush_to_queue(guac_common_surface* surface) {

    guac_common_surface_png_rect* rect;

    /* Do not flush if not dirty */
    if (!surface->dirty)
        return;

    /* Add new rect to queue */
    rect = &(surface->png_queue[surface->png_queue_length++]);
    rect->rect = surface->dirty_rect;
    rect->flushed = 0;

    /* Surface now flushed */
    surface->dirty = 0;

}

void guac_common_surface_flush_deferred(guac_common_surface* surface) {

    /* Do not flush if not dirty */
    if (!surface->dirty)
        return;

    /* Flush if queue size has reached maximum (space is reserved for the final dirty rect,
     * as guac_common_surface_flush() MAY add an additional rect to the queue */
    if (surface->png_queue_length == GUAC_COMMON_SURFACE_QUEUE_SIZE-1)
        guac_common_surface_flush(surface);

    /* Append dirty rect to queue */
    __guac_common_surface_flush_to_queue(surface);

}

/**
 * Transfers a single uint32_t using the given transfer function.
 *
 * @param op The transfer function to use.
 * @param src The source of the uint32_t value.
 * @param dst THe destination which will hold the result of the transfer.
 * @return Non-zero if the destination value was changed, zero otherwise.
 */
static int __guac_common_surface_transfer_int(guac_transfer_function op, uint32_t* src, uint32_t* dst) {

    uint32_t orig = *dst;

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

    return *dst != orig;

}

/**
 * Draws a rectangle of solid color within the backing surface of the
 * given destination surface.
 *
 * @param dst The destination surface.
 * @param rect The rectangle to draw.
 * @param red The red component of the color of the rectangle.
 * @param green The green component of the color of the rectangle.
 * @param blue The blue component of the color of the rectangle.
 */
static void __guac_common_surface_rect(guac_common_surface* dst, guac_common_rect* rect,
                                       int red, int green, int blue) {

    int x, y;

    int dst_stride;
    unsigned char* dst_buffer;

    uint32_t color = 0xFF000000 | (red << 16) | (green << 8) | blue;

    int min_x = rect->width - 1;
    int min_y = rect->height - 1;
    int max_x = 0;
    int max_y = 0;

    dst_stride = dst->stride;
    dst_buffer = dst->buffer + (dst_stride * rect->y) + (4 * rect->x);

    /* For each row */
    for (y=0; y < rect->height; y++) {

        uint32_t* dst_current = (uint32_t*) dst_buffer;

        /* Set row */
        for (x=0; x < rect->width; x++) {

            uint32_t old_color = *dst_current;

            if (old_color != color) {
                if (x < min_x) min_x = x;
                if (y < min_y) min_y = y;
                if (x > max_x) max_x = x;
                if (y > max_y) max_y = y;
                *dst_current = color;
            }

            dst_current++;
        }

        /* Next row */
        dst_buffer += dst_stride;

    }

    /* Restrict destination rect to only updated pixels */
    if (max_x >= min_x && max_y >= min_y) {
        rect->x += min_x;
        rect->y += min_y;
        rect->width = max_x - min_x + 1;
        rect->height = max_y - min_y + 1;
    }
    else {
        rect->width = 0;
        rect->height = 0;
    }

}

/**
 * Copies data from the given buffer to the surface at the given coordinates.
 * The dimensions and location of the destination rectangle will be altered
 * to remove as many unchanged pixels as possible.
 *
 * @param src_buffer The buffer to copy.
 * @param src_stride The number of bytes in each row of the source buffer.
 * @param sx The X coordinate of the source rectangle.
 * @param sy The Y coordinate of the source rectangle.
 * @param dst The destination surface.
 * @param rect The destination rectangle.
 * @param opaque Non-zero if the source surface is opaque (its alpha channel
 *               should be ignored), zero otherwise.
 */
static void __guac_common_surface_put(unsigned char* src_buffer, int src_stride,
                                      int* sx, int* sy,
                                      guac_common_surface* dst, guac_common_rect* rect,
                                      int opaque) {

    unsigned char* dst_buffer = dst->buffer;
    int dst_stride = dst->stride;

    int x, y;

    int min_x = rect->width;
    int min_y = rect->height;
    int max_x = 0;
    int max_y = 0;

    int orig_x = rect->x;
    int orig_y = rect->y;

    src_buffer += src_stride * (*sy) + 4 * (*sx);
    dst_buffer += (dst_stride * rect->y) + (4 * rect->x);

    /* For each row */
    for (y=0; y < rect->height; y++) {

        uint32_t* src_current = (uint32_t*) src_buffer;
        uint32_t* dst_current = (uint32_t*) dst_buffer;

        /* Copy row */
        for (x=0; x < rect->width; x++) {

            if (opaque || (*src_current & 0xFF000000)) {

                uint32_t new_color = *src_current | 0xFF000000;
                uint32_t old_color = *dst_current;

                if (old_color != new_color) {
                    if (x < min_x) min_x = x;
                    if (y < min_y) min_y = y;
                    if (x > max_x) max_x = x;
                    if (y > max_y) max_y = y;
                    *dst_current = new_color;
                }
            }

            src_current++;
            dst_current++;
        }

        /* Next row */
        src_buffer += src_stride;
        dst_buffer += dst_stride;

    }

    /* Restrict destination rect to only updated pixels */
    if (max_x >= min_x && max_y >= min_y) {
        rect->x += min_x;
        rect->y += min_y;
        rect->width = max_x - min_x + 1;
        rect->height = max_y - min_y + 1;
    }
    else {
        rect->width = 0;
        rect->height = 0;
    }

    /* Update source X/Y */
    *sx += rect->x - orig_x;
    *sy += rect->y - orig_y;

}

/**
 * Fills the given surface with color, using the given buffer as a mask. Color
 * will be added to the given surface iff the corresponding pixel within the
 * buffer is opaque.
 *
 * @param src_buffer The buffer to use as a mask.
 * @param src_stride The number of bytes in each row of the source buffer.
 * @param sx The X coordinate of the source rectangle.
 * @param sy The Y coordinate of the source rectangle.
 * @param dst The destination surface.
 * @param rect The destination rectangle.
 * @param red The red component of the color of the fill.
 * @param green The green component of the color of the fill.
 * @param blue The blue component of the color of the fill.
 */
static void __guac_common_surface_fill_mask(unsigned char* src_buffer, int src_stride,
                                            int sx, int sy,
                                            guac_common_surface* dst, guac_common_rect* rect,
                                            int red, int green, int blue) {

    unsigned char* dst_buffer = dst->buffer;
    int dst_stride = dst->stride;

    uint32_t color = 0xFF000000 | (red << 16) | (green << 8) | blue;
    int x, y;

    src_buffer += src_stride*sy + 4*sx;
    dst_buffer += (dst_stride * rect->y) + (4 * rect->x);

    /* For each row */
    for (y=0; y < rect->height; y++) {

        uint32_t* src_current = (uint32_t*) src_buffer;
        uint32_t* dst_current = (uint32_t*) dst_buffer;

        /* Stencil row */
        for (x=0; x < rect->width; x++) {

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

/**
 * Copies data from the given surface to the given destination surface using
 * the specified transfer function.
 *
 * @param src_buffer The buffer to copy.
 * @param src_stride The number of bytes in each row of the source buffer.
 * @param sx The X coordinate of the source rectangle.
 * @param sy The Y coordinate of the source rectangle.
 * @param op The transfer function to use.
 * @param dst The destination surface.
 * @param rect The destination rectangle.
 */
static void __guac_common_surface_transfer(guac_common_surface* src, int* sx, int* sy,
                                           guac_transfer_function op,
                                           guac_common_surface* dst, guac_common_rect* rect) {

    unsigned char* src_buffer = src->buffer;
    unsigned char* dst_buffer = dst->buffer;

    int x, y;
    int src_stride, dst_stride;
    int step = 1;

    int min_x = rect->width - 1;
    int min_y = rect->height - 1;
    int max_x = 0;
    int max_y = 0;

    int orig_x = rect->x;
    int orig_y = rect->y;

    /* Copy forwards only if destination is in a different surface or is before source */
    if (src != dst || rect->y < *sy || (rect->y == *sy && rect->x < *sx)) {
        src_buffer += src->stride * (*sy) + 4 * (*sx);
        dst_buffer += (dst->stride * rect->y) + (4 * rect->x);
        src_stride = src->stride;
        dst_stride = dst->stride;
        step = 1;
    }

    /* Otherwise, copy backwards */
    else {
        src_buffer += src->stride * (*sy + rect->height - 1) + 4 * (*sx + rect->width - 1);
        dst_buffer += dst->stride * (rect->y + rect->height - 1) + 4 * (rect->x + rect->width - 1);
        src_stride = -src->stride;
        dst_stride = -dst->stride;
        step = -1;
    }

    /* For each row */
    for (y=0; y < rect->height; y++) {

        uint32_t* src_current = (uint32_t*) src_buffer;
        uint32_t* dst_current = (uint32_t*) dst_buffer;

        /* Transfer each pixel in row */
        for (x=0; x < rect->width; x++) {

            if (__guac_common_surface_transfer_int(op, src_current, dst_current)) {
                if (x < min_x) min_x = x;
                if (y < min_y) min_y = y;
                if (x > max_x) max_x = x;
                if (y > max_y) max_y = y;
            }

            src_current += step;
            dst_current += step;
        }

        /* Next row */
        src_buffer += src_stride;
        dst_buffer += dst_stride;

    }

    /* Translate X coordinate space of moving backwards */
    if (step < 0) {
        int old_max_x = max_x;
        max_x = rect->width - 1 - min_x;
        min_x = rect->width - 1 - old_max_x;
    }

    /* Translate Y coordinate space of moving backwards */
    if (dst_stride < 0) {
        int old_max_y = max_y;
        max_y = rect->height - 1 - min_y;
        min_y = rect->height - 1 - old_max_y;
    }

    /* Restrict destination rect to only updated pixels */
    if (max_x >= min_x && max_y >= min_y) {
        rect->x += min_x;
        rect->y += min_y;
        rect->width = max_x - min_x + 1;
        rect->height = max_y - min_y + 1;
    }
    else {
        rect->width = 0;
        rect->height = 0;
    }

    /* Update source X/Y */
    *sx += rect->x - orig_x;
    *sy += rect->y - orig_y;

}

guac_common_surface* guac_common_surface_alloc(guac_client* client,
        guac_socket* socket, const guac_layer* layer, int w, int h) {

    /* Init surface */
    guac_common_surface* surface = malloc(sizeof(guac_common_surface));
    surface->client = client;
    surface->socket = socket;
    surface->layer = layer;
    surface->width = w;
    surface->height = h;
    surface->dirty = 0;
    surface->png_queue_length = 0;

    /* Create corresponding Cairo surface */
    surface->stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, w);
    surface->buffer = calloc(h, surface->stride);

    /* Reset clipping rect */
    guac_common_surface_reset_clip(surface);

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

    unsigned char* old_buffer;
    int old_stride;
    guac_common_rect old_rect;

    int sx = 0;
    int sy = 0;

    /* Copy old surface data */
    old_buffer = surface->buffer;
    old_stride = surface->stride;
    guac_common_rect_init(&old_rect, 0, 0, surface->width, surface->height);

    /* Re-initialize at new size */
    surface->width  = w;
    surface->height = h;
    surface->stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, w);
    surface->buffer = calloc(h, surface->stride);
    __guac_common_bound_rect(surface, &surface->clip_rect, NULL, NULL);

    /* Copy relevant old data */
    __guac_common_bound_rect(surface, &old_rect, NULL, NULL);
    __guac_common_surface_put(old_buffer, old_stride, &sx, &sy, surface, &old_rect, 1);

    /* Free old data */
    free(old_buffer);

    /* Resize dirty rect to fit new surface dimensions */
    if (surface->dirty) {
        __guac_common_bound_rect(surface, &surface->dirty_rect, NULL, NULL);
        if (surface->dirty_rect.width <= 0 || surface->dirty_rect.height <= 0)
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

    guac_common_rect rect;
    guac_common_rect_init(&rect, x, y, w, h);

    /* Clip operation */
    __guac_common_clip_rect(surface, &rect, &sx, &sy);
    if (rect.width <= 0 || rect.height <= 0)
        return;

    /* Update backing surface */
    __guac_common_surface_put(buffer, stride, &sx, &sy, surface, &rect, format != CAIRO_FORMAT_ARGB32);
    if (rect.width <= 0 || rect.height <= 0)
        return;

    /* Flush if not combining */
    if (!__guac_common_should_combine(surface, &rect, 0))
        guac_common_surface_flush_deferred(surface);

    /* Always defer draws */
    __guac_common_mark_dirty(surface, &rect);

}

void guac_common_surface_paint(guac_common_surface* surface, int x, int y, cairo_surface_t* src,
                               int red, int green, int blue) {

    unsigned char* buffer = cairo_image_surface_get_data(src);
    int stride = cairo_image_surface_get_stride(src);
    int w = cairo_image_surface_get_width(src);
    int h = cairo_image_surface_get_height(src);

    int sx = 0;
    int sy = 0;

    guac_common_rect rect;
    guac_common_rect_init(&rect, x, y, w, h);

    /* Clip operation */
    __guac_common_clip_rect(surface, &rect, &sx, &sy);
    if (rect.width <= 0 || rect.height <= 0)
        return;

    /* Update backing surface */
    __guac_common_surface_fill_mask(buffer, stride, sx, sy, surface, &rect, red, green, blue);

    /* Flush if not combining */
    if (!__guac_common_should_combine(surface, &rect, 0))
        guac_common_surface_flush_deferred(surface);

    /* Always defer draws */
    __guac_common_mark_dirty(surface, &rect);

}

void guac_common_surface_copy(guac_common_surface* src, int sx, int sy, int w, int h,
                              guac_common_surface* dst, int dx, int dy) {

    guac_socket* socket = dst->socket;
    const guac_layer* src_layer = src->layer;
    const guac_layer* dst_layer = dst->layer;

    guac_common_rect rect;
    guac_common_rect_init(&rect, dx, dy, w, h);

    /* Clip operation */
    __guac_common_clip_rect(dst, &rect, &sx, &sy);
    if (rect.width <= 0 || rect.height <= 0)
        return;

    /* Update backing surface first only if destination rect cannot intersect source rect */
    if (src != dst) {
        __guac_common_surface_transfer(src, &sx, &sy, GUAC_TRANSFER_BINARY_SRC, dst, &rect);
        if (rect.width <= 0 || rect.height <= 0)
            return;
    }

    /* Defer if combining */
    if (__guac_common_should_combine(dst, &rect, 1))
        __guac_common_mark_dirty(dst, &rect);

    /* Otherwise, flush and draw immediately */
    else {
        guac_common_surface_flush(dst);
        guac_common_surface_flush(src);
        guac_protocol_send_copy(socket, src_layer, sx, sy, rect.width, rect.height,
                                GUAC_COMP_OVER, dst_layer, rect.x, rect.y);
        dst->realized = 1;
    }

    /* Update backing surface last if destination rect can intersect source rect */
    if (src == dst)
        __guac_common_surface_transfer(src, &sx, &sy, GUAC_TRANSFER_BINARY_SRC, dst, &rect);

}

void guac_common_surface_transfer(guac_common_surface* src, int sx, int sy, int w, int h,
                                  guac_transfer_function op, guac_common_surface* dst, int dx, int dy) {

    guac_socket* socket = dst->socket;
    const guac_layer* src_layer = src->layer;
    const guac_layer* dst_layer = dst->layer;

    guac_common_rect rect;
    guac_common_rect_init(&rect, dx, dy, w, h);

    /* Clip operation */
    __guac_common_clip_rect(dst, &rect, &sx, &sy);
    if (rect.width <= 0 || rect.height <= 0)
        return;

    /* Update backing surface first only if destination rect cannot intersect source rect */
    if (src != dst) {
        __guac_common_surface_transfer(src, &sx, &sy, op, dst, &rect);
        if (rect.width <= 0 || rect.height <= 0)
            return;
    }

    /* Defer if combining */
    if (__guac_common_should_combine(dst, &rect, 1))
        __guac_common_mark_dirty(dst, &rect);

    /* Otherwise, flush and draw immediately */
    else {
        guac_common_surface_flush(dst);
        guac_common_surface_flush(src);
        guac_protocol_send_transfer(socket, src_layer, sx, sy, rect.width, rect.height, op, dst_layer, rect.x, rect.y);
        dst->realized = 1;
    }

    /* Update backing surface last if destination rect can intersect source rect */
    if (src == dst)
        __guac_common_surface_transfer(src, &sx, &sy, op, dst, &rect);

}

void guac_common_surface_rect(guac_common_surface* surface,
                              int x, int y, int w, int h,
                              int red, int green, int blue) {

    guac_socket* socket = surface->socket;
    const guac_layer* layer = surface->layer;

    guac_common_rect rect;
    guac_common_rect_init(&rect, x, y, w, h);

    /* Clip operation */
    __guac_common_clip_rect(surface, &rect, NULL, NULL);
    if (rect.width <= 0 || rect.height <= 0)
        return;

    /* Update backing surface */
    __guac_common_surface_rect(surface, &rect, red, green, blue);
    if (rect.width <= 0 || rect.height <= 0)
        return;

    /* Defer if combining */
    if (__guac_common_should_combine(surface, &rect, 1))
        __guac_common_mark_dirty(surface, &rect);

    /* Otherwise, flush and draw immediately */
    else {
        guac_common_surface_flush(surface);
        guac_protocol_send_rect(socket, layer, rect.x, rect.y, rect.width, rect.height);
        guac_protocol_send_cfill(socket, GUAC_COMP_OVER, layer, red, green, blue, 0xFF);
        surface->realized = 1;
    }

}

void guac_common_surface_clip(guac_common_surface* surface, int x, int y, int w, int h) {

    guac_common_rect clip;

    /* Init clipping rectangle if clipping not already applied */
    if (!surface->clipped) {
        guac_common_rect_init(&surface->clip_rect, 0, 0, surface->width, surface->height);
        surface->clipped = 1;
    }

    guac_common_rect_init(&clip, x, y, w, h);
    guac_common_rect_constrain(&surface->clip_rect, &clip);

}

void guac_common_surface_reset_clip(guac_common_surface* surface) {
    surface->clipped = 0;
}

/**
 * Flushes the PNG update currently described by the dirty rectangle within the
 * given surface directly to a "png" instruction, which is sent on the socket
 * associated with the surface.
 *
 * @param surface The surface to flush.
 */
static void __guac_common_surface_flush_to_png(guac_common_surface* surface) {

    if (surface->dirty) {

        guac_socket* socket = surface->socket;
        const guac_layer* layer = surface->layer;

        /* Get Cairo surface for specified rect */
        unsigned char* buffer = surface->buffer + surface->dirty_rect.y * surface->stride + surface->dirty_rect.x * 4;
        cairo_surface_t* rect = cairo_image_surface_create_for_data(buffer, CAIRO_FORMAT_RGB24,
                                                                    surface->dirty_rect.width,
                                                                    surface->dirty_rect.height,
                                                                    surface->stride);

        /* Send PNG for rect */
        guac_protocol_send_png(socket, GUAC_COMP_OVER, layer, surface->dirty_rect.x, surface->dirty_rect.y, rect);
        cairo_surface_destroy(rect);
        surface->realized = 1;

        /* Surface is no longer dirty */
        surface->dirty = 0;

    }

}

/**
 * Comparator for instances of guac_common_surface_png_rect, the elements
 * which make up a surface's PNG buffer.
 *
 * @see qsort
 */
static int __guac_common_surface_png_rect_compare(const void* a, const void* b) {

    guac_common_surface_png_rect* ra = (guac_common_surface_png_rect*) a;
    guac_common_surface_png_rect* rb = (guac_common_surface_png_rect*) b;

    /* Order roughly top to bottom, left to right */
    if (ra->rect.y != rb->rect.y) return ra->rect.y - rb->rect.y;
    if (ra->rect.x != rb->rect.x) return ra->rect.x - rb->rect.x;

    /* Wider updates should come first (more likely to intersect later) */
    if (ra->rect.width != rb->rect.width) return rb->rect.width - ra->rect.width;

    /* Shorter updates should come first (less likely to increase cost) */
    return ra->rect.height - rb->rect.height;

}

void guac_common_surface_flush(guac_common_surface* surface) {

    guac_common_surface_png_rect* current = surface->png_queue;

    int i, j;
    int original_queue_length;
    int flushed = 0;

    /* Flush final dirty rect to queue */
    __guac_common_surface_flush_to_queue(surface);
    original_queue_length = surface->png_queue_length;

    /* Sort updates to make combination less costly */
    qsort(surface->png_queue, surface->png_queue_length, sizeof(guac_common_surface_png_rect),
          __guac_common_surface_png_rect_compare);

    /* Flush all rects in queue */
    for (i=0; i < surface->png_queue_length; i++) {

        /* Get next unflushed candidate */
        guac_common_surface_png_rect* candidate = current;
        if (!candidate->flushed) {

            int combined = 0;

            /* Build up rect as much as possible */
            for (j=i; j < surface->png_queue_length; j++) {

                if (!candidate->flushed) {

                    /* Clip candidate within current bounds */
                    __guac_common_bound_rect(surface, &candidate->rect, NULL, NULL);
                    if (candidate->rect.width <= 0 || candidate->rect.height <= 0)
                        candidate->flushed = 1;

                    /* Combine if reasonable */
                    else if (__guac_common_should_combine(surface, &candidate->rect, 0) || !surface->dirty) {
                        __guac_common_mark_dirty(surface, &candidate->rect);
                        candidate->flushed = 1;
                        combined++;
                    }

                }

                candidate++;

            }

            /* Re-add to queue if there's room and this update was modified or we expect others might be */
            if ((combined > 1 || i < original_queue_length)
                    && surface->png_queue_length < GUAC_COMMON_SURFACE_QUEUE_SIZE)
                __guac_common_surface_flush_to_queue(surface);

            /* Flush as PNG otherwise */
            else {
                if (surface->dirty) flushed++;
                __guac_common_surface_flush_to_png(surface);
            }

        }

        current++;

    }

    /* Flush complete */
    surface->png_queue_length = 0;

}

