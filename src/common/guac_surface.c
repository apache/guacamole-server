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
#include "guac_surface_smoothness.h"

#include <cairo/cairo.h>
#include <guacamole/client.h>
#include <guacamole/layer.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/timestamp.h>

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

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
 * The JPEG compression minimum block size. This defines the optimal rectangle
 * block size factor for JPEG compression to reduce artifacts. Usually this is
 * 8 (8x8), but use 16 to reduce the occurence of ringing artifacts further.
 */
#define GUAC_SURFACE_JPEG_BLOCK_SIZE 16

/**
 * Minimum JPEG bitmap size (area). If the bitmap is smaller than this
 * threshold, it should be compressed as a PNG image to avoid the JPEG
 * compression tax.
 */
#define GUAC_SURFACE_JPEG_MIN_BITMAP_SIZE 4096

/**
 * The JPEG image quality ('quantization') setting to use. Range 0-100 where
 * 100 is the highest quality/largest file size, and 0 is the lowest
 * quality/smallest file size.
 */
#define GUAC_SURFACE_JPEG_IMAGE_QUALITY 90

/**
 * Time (msec) between each time the surface's heat map is recalculated.
 */
#define GUAC_COMMON_SURFACE_HEAT_MAP_UPDATE_FREQ 2000

/**
 * Refresh frequency threshold for when an area should be refreshed lossy.
 */
#define GUAC_COMMON_SURFACE_LOSSY_REFRESH_FREQUENCY 3

/**
 * Time delay threshold between two updates where a lossy area will be moved
 * to the non-lossy refresh pipe.
 */
#define GUAC_COMMON_SURFACE_NON_LOSSY_REFRESH_THRESHOLD 3000

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
 * Flush a surface's lossy area to the dirty rectangle. This will make the
 * rectangle refresh through the normal non-lossy refresh path.
 *
 * @param surface
 *     The surface whose lossy area will be moved to the dirty refresh
 *     queue.
 *
 * @param x
 *     The x coordinate of the area to move.
 *
 * @param y
 *     The y coordinate of the area to move.
 */
static void __guac_common_surface_flush_lossy_rect_to_dirty_rect(
        guac_common_surface* surface, int x, int y) {

    /* Get the heat map index. */
    int hx = x / GUAC_COMMON_SURFACE_HEAT_MAP_CELL;
    int hy = y / GUAC_COMMON_SURFACE_HEAT_MAP_CELL;

    /* Don't update if this rect was not previously sent as a lossy refresh. */
    if (!surface->lossy_rect[hy][hx]) {
        return;
    }

    /* Clear the lossy status for this heat map rectangle. */
    surface->lossy_rect[hy][hx] = 0;

    guac_common_rect lossy_rect;
    guac_common_rect_init(&lossy_rect, x, y,
            GUAC_COMMON_SURFACE_HEAT_MAP_CELL, GUAC_COMMON_SURFACE_HEAT_MAP_CELL);
    int sx = 0;
    int sy = 0;

    /* Clip operation */
    __guac_common_clip_rect(surface, &lossy_rect, &sx, &sy);
    if (lossy_rect.width <= 0 || lossy_rect.height <= 0)
        return;

    /* Flush the rectangle if not combining. */
    if (!__guac_common_should_combine(surface, &lossy_rect, 0))
        guac_common_surface_flush_deferred(surface);

    /* Always defer draws */
    __guac_common_mark_dirty(surface, &lossy_rect);

}

/**
 * Actual method which flushes a bitmap described by the dirty rectangle
 * on the socket associated with the surface.
 *
 * The bitmap will be sent as a "jpeg" or "png" instruction based on the lossy
 * flag. Certain conditions may override the lossy flag and send a lossless
 * update.
 *
 * @param surface
 *     The surface whose dirty area will be flushed.
 *
 * @param dirty_rect
 *     The dirty rectangle.
 *
 * @param lossy
 *     Flag indicating whether this refresh should be lossy.
 */
static void __guac_common_surface_flush_to_bitmap_impl(guac_common_surface* surface,
        guac_common_rect* dirty_rect, int lossy) {

    guac_socket* socket = surface->socket;
    const guac_layer* layer = surface->layer;
    int send_jpeg = 0;

    /* Set the JPEG flag indicating whether this bitmap should be sent as JPEG.
     * Only send as a JPEG if the dirty is larger than the minimum JPEG bitmap
     * size to avoid the JPEG image compression tax. */
    if (lossy &&
        (dirty_rect->width * dirty_rect->height) > GUAC_SURFACE_JPEG_MIN_BITMAP_SIZE) {

        /* Check the smoothness of the dirty rectangle. If smooth, do not send
         * a JPEG as it has a higher overhead than standard PNG. */
        if (!guac_common_surface_rect_is_smooth(surface, dirty_rect)) {

            send_jpeg = 1;

            /* Tweak the rectangle if it is to be sent as JPEG so the size
             * matches the JPEG block size. */
            guac_common_rect max;
            guac_common_rect_init(&max, 0, 0, surface->width, surface->height);

            guac_common_rect_expand_to_grid(GUAC_SURFACE_JPEG_BLOCK_SIZE,
                                            dirty_rect, &max);
        }

    }

    /* Get Cairo surface for specified rect.
     * The buffer is created with 4 bytes per pixel because Cairo's 24 bit RGB
     * really is 32 bit BGRx */
    unsigned char* buffer = surface->buffer + dirty_rect->y * surface->stride + dirty_rect->x * 4;
    cairo_surface_t* rect = cairo_image_surface_create_for_data(buffer, CAIRO_FORMAT_RGB24,
                                                                dirty_rect->width,
                                                                dirty_rect->height,
                                                                surface->stride);

    /* Send bitmap update for the dirty rectangle */
    if (send_jpeg) {
        guac_client_stream_jpeg(surface->client, socket, GUAC_COMP_OVER, layer,
                                dirty_rect->x, dirty_rect->y, rect,
                                GUAC_SURFACE_JPEG_IMAGE_QUALITY);
    }
    else {
        guac_client_stream_png(surface->client, socket, GUAC_COMP_OVER, layer,
                               dirty_rect->x, dirty_rect->y, rect);
    }

    cairo_surface_destroy(rect);

}

/**
 * Flushes the rectangle to the given surface's bitmap queue. There MUST be
 * space within the queue.
 *
 * @param surface The surface queue to flush to.
 * @param rect The rectangle to flush.
 */
static void __guac_common_surface_flush_rect_to_queue(guac_common_surface* surface,
        const guac_common_rect* rect) {
    guac_common_surface_bitmap_rect* bitmap_rect;

    /* Add new rect to queue */
    bitmap_rect = &(surface->bitmap_queue[surface->bitmap_queue_length++]);
    bitmap_rect->rect = *rect;
    bitmap_rect->flushed = 0;
}

/**
 * Flushes the bitmap update currently described by a lossy rectangle within the
 * given surface.
 *
 * Scans through the regular bitmap update queue and excludes any rectangles
 * covered by the lossy rectangle.
 *
 *  @param surface
 *     The surface whose lossy area will be flushed.
 */
static void __guac_common_surface_flush_lossy_bitmap(
        guac_common_surface* surface) {

    if (surface->lossy_dirty) {

        guac_common_surface_bitmap_rect* current = surface->bitmap_queue;
        int original_queue_length = surface->bitmap_queue_length;

        /* Identify all bitmaps in queue which are
         * covered by the lossy rectangle. */
        for (int i=0; i < original_queue_length; i++) {

            int intersects = guac_common_rect_intersects(&current->rect,
                                                    &surface->lossy_dirty_rect);
            /* Complete intersection. */
            if (intersects == 2) {

                /* Exclude this from the normal refresh as it is completely
                 * covered by the lossy dirty rectangle. */
                current->flushed = 1;

            }

            /* Partial intersection.
             * The rectangle will be split if there is room on the queue. */
            else if (intersects == 1 &&
                     surface->bitmap_queue_length < GUAC_COMMON_SURFACE_QUEUE_SIZE-5) {

                /* Clip and split rectangle into rectangles that are outside the
                 * lossy rectangle which are added to the normal refresh queue.
                 * The remaining rectangle which overlaps with the lossy
                 * rectangle is marked flushed to not be refreshed in the normal
                 * refresh cycle.
                 */
                guac_common_rect split_rect;
                while (guac_common_rect_clip_and_split(&current->rect,
                       &surface->lossy_dirty_rect, &split_rect)) {

                    /* Add new rectangle to update queue */
                    __guac_common_surface_flush_rect_to_queue(surface,
                                                              &split_rect);

                }

                /* Exclude the remaining part of the dirty rectangle
                 * which is completely covered by the lossy dirty rectangle. */
                current->flushed = 1;

            }
            current++;

        }

        /* Flush the lossy bitmap */
        __guac_common_surface_flush_to_bitmap_impl(surface,
                                                 &surface->lossy_dirty_rect, 1);

        /* Flag this area as lossy so it can be moved back to the
         * dirty rect and refreshed normally when refreshed less frequently. */
        int x = surface->lossy_dirty_rect.x;
        int y = surface->lossy_dirty_rect.y;
        int w = (x + surface->lossy_dirty_rect.width) / GUAC_COMMON_SURFACE_HEAT_MAP_CELL;
        int h = (y + surface->lossy_dirty_rect.height) / GUAC_COMMON_SURFACE_HEAT_MAP_CELL;
        x /= GUAC_COMMON_SURFACE_HEAT_MAP_CELL;
        y /= GUAC_COMMON_SURFACE_HEAT_MAP_CELL;

        for (int j = y; j <= h; j++) {
            for (int i = x; i <= w; i++) {
                surface->lossy_rect[j][i] = 1;
            }
        }

        /* Clear the lossy dirty flag. */
        surface->lossy_dirty = 0;
    }

}

/**
 * Calculate the current average refresh frequency for a given area on the
 * surface.
 *
 * @param surface
 *     The surface on which the refresh frequency will be calculated.
 *
 * @param x
 *     The x coordinate for the area.
 *
 * @param y
 *     The y coordinate for the area.
 *
 * @param w
 *     The area width.
 *
 * @param h
 *     The area height.
 *
 * @return
 *     The average refresh frequency.
 */
static unsigned int __guac_common_surface_calculate_refresh_frequency(
                                                   guac_common_surface* surface,
                                                   int x, int y, int w, int h)
{

    w = (x + w) / GUAC_COMMON_SURFACE_HEAT_MAP_CELL;
    h = (y + h) / GUAC_COMMON_SURFACE_HEAT_MAP_CELL;
    x /= GUAC_COMMON_SURFACE_HEAT_MAP_CELL;
    y /= GUAC_COMMON_SURFACE_HEAT_MAP_CELL;

    unsigned int sum_frequency = 0;
    unsigned int count = 0;
    /* Iterate over all the heat map cells for the area
     * and calculate the average refresh frequency. */
    for (int hy = y; hy <= h; hy++) {
        for (int hx = x; hx <= w; hx++) {

            const guac_common_surface_heat_rect* heat_rect = &surface->heat_map[hy][hx];
            sum_frequency += heat_rect->frequency;
            count++;

        }
    }

    /* Calculate the average. */
    if (count) {
        return sum_frequency / count;
    }
    else {
        return 0;
    }
}

/**
 * Update the heat map for the surface and re-calculate the refresh frequencies.
 *
 * Any areas of the surface which have not been updated within a given threshold
 * will be moved from the lossy to the normal refresh path.
 *
 * @param surface
 *     The surface on which the heat map will be refreshed.
 *
 * @param now
 *     The current time.
 */
static void __guac_common_surface_update_heat_map(guac_common_surface* surface,
                                                  guac_timestamp now)
{

    /* Only update the heat map at the given interval. */
    if (now - surface->last_heat_map_update < GUAC_COMMON_SURFACE_HEAT_MAP_UPDATE_FREQ) {
        return;
    }
    surface->last_heat_map_update = now;

    const int width = surface->width / GUAC_COMMON_SURFACE_HEAT_MAP_CELL;
    const int height = surface->height / GUAC_COMMON_SURFACE_HEAT_MAP_CELL;
    int hx, hy;

    for (hy = 0; hy < height; hy++) {
        for (hx = 0; hx < width; hx++) {

            guac_common_surface_heat_rect* heat_rect = &surface->heat_map[hy][hx];

            const int last_update_index = (heat_rect->index + GUAC_COMMON_SURFACE_HEAT_UPDATE_ARRAY_SZ - 1) % GUAC_COMMON_SURFACE_HEAT_UPDATE_ARRAY_SZ;
            const guac_timestamp last_update = heat_rect->updates[last_update_index];
            const guac_timestamp time_since_last = now - last_update;

            /* If the time between the last 2 refreshes is larger than the
             * threshold, move this rectangle back to the non-lossy
             * refresh pipe. */
            if (time_since_last > GUAC_COMMON_SURFACE_NON_LOSSY_REFRESH_THRESHOLD) {

                /* Send this lossy rectangle to the normal update queue. */
                const int x = hx * GUAC_COMMON_SURFACE_HEAT_MAP_CELL;
                const int y = hy * GUAC_COMMON_SURFACE_HEAT_MAP_CELL;
                __guac_common_surface_flush_lossy_rect_to_dirty_rect(surface,
                                                                     x, y);

                /* Clear the frequency and refresh times for this square. */
                heat_rect->frequency = 0;
                memset(heat_rect->updates, 0, sizeof(heat_rect->updates));
                continue ;
            }

            /* Only calculate frequency after N updates to this heat
             * rectangle. */
            if (heat_rect->updates[GUAC_COMMON_SURFACE_HEAT_UPDATE_ARRAY_SZ - 1] == 0) {
                continue;
            }

            /* Calculate refresh frequency. */
            const guac_timestamp first_update = heat_rect->updates[heat_rect->index];
            int elapsed_time = last_update - first_update;
            if (elapsed_time)
                heat_rect->frequency = GUAC_COMMON_SURFACE_HEAT_UPDATE_ARRAY_SZ * 1000 / elapsed_time;
            else
                heat_rect->frequency = 0;

        }
    }

}

/**
 * Touch the heat map with this update rectangle, so that the update
 * frequency can be calculated later.
 *
 * @param surface
 *     The surface containing the rectangle to be updated.
 *
 * @param rect
 *     The rectangle updated.
 *
 * @param time
 *     The time stamp of this update.
 */
static void __guac_common_surface_touch_rect(guac_common_surface* surface,
        guac_common_rect* rect, guac_timestamp time)
{

    const int w = (rect->x + rect->width) / GUAC_COMMON_SURFACE_HEAT_MAP_CELL;
    const int h = (rect->y + rect->height) / GUAC_COMMON_SURFACE_HEAT_MAP_CELL;
    int hx = rect->x / GUAC_COMMON_SURFACE_HEAT_MAP_CELL;
    int hy = rect->y / GUAC_COMMON_SURFACE_HEAT_MAP_CELL;

    for (; hy <= h; hy++) {
        for (; hx <= w; hx++) {

            guac_common_surface_heat_rect* heat_rect = &surface->heat_map[hy][hx];
            heat_rect->updates[heat_rect->index] = time;

            /* Move the heat index to the next. */
            heat_rect->index = (heat_rect->index + 1) % GUAC_COMMON_SURFACE_HEAT_UPDATE_ARRAY_SZ;

        }
    }

}

/**
 * Expands the lossy dirty rectangle of the given surface to contain the
 * rectangle described by the given coordinates.
 *
 * @param surface
 *     The surface to mark as dirty.
 *
 * @param rect
 *     The rectangle of the update which is dirtying the surface.
 */
static void __guac_common_mark_lossy_dirty(guac_common_surface* surface,
        const guac_common_rect* rect) {

    /* Ignore empty rects */
    if (rect->width <= 0 || rect->height <= 0)
        return;

    /* If already dirty, update existing rect */
    if (surface->lossy_dirty) {
        guac_common_rect_extend(&surface->lossy_dirty_rect, rect);
    }
    /* Otherwise init lossy dirty rect */
    else {
        surface->lossy_dirty_rect = *rect;
        surface->lossy_dirty = 1;
    }

}

/**
 * Flushes the bitmap update currently described by the dirty rectangle within the
 * given surface to that surface's bitmap queue. There MUST be space within the
 * queue.
 *
 * @param surface The surface to flush.
 */
static void __guac_common_surface_flush_to_queue(guac_common_surface* surface) {

    /* Do not flush if not dirty */
    if (!surface->dirty)
        return;

    /* Add new rect to queue */
    __guac_common_surface_flush_rect_to_queue(surface, &surface->dirty_rect);

    /* Surface now flushed */
    surface->dirty = 0;
}

void guac_common_surface_flush_deferred(guac_common_surface* surface) {

    /* Do not flush if not dirty */
    if (!surface->dirty)
        return;

    /* Flush if queue size has reached maximum (space is reserved for the final dirty rect,
     * as guac_common_surface_flush() MAY add an additional rect to the queue */
    if (surface->bitmap_queue_length == GUAC_COMMON_SURFACE_QUEUE_SIZE-1)
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
    surface->bitmap_queue_length = 0;

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

    /* Initialize heat map and adaptive coding bits. */
    surface->lossy_dirty = 0;
    surface->last_heat_map_update = 0;
    for (int y = 0; y < GUAC_COMMON_SURFACE_HEAT_MAP_ROWS; y++) {
        for (int x = 0; x < GUAC_COMMON_SURFACE_HEAT_MAP_COLS; x++) {

            guac_common_surface_heat_rect *rect= & surface->heat_map[y][x];
            memset(rect->updates, 0, sizeof(rect->updates));
            rect->frequency = 0;
            rect->index = 0;

            surface->lossy_rect[y][x] = 0;

        }
    }

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

    unsigned int freq = 0;

    /* Update the heat map for the update rectangle. */
    guac_timestamp time = guac_timestamp_current();
    __guac_common_surface_touch_rect(surface, &rect, time);

    /* Calculate the update frequency for this rectangle. */
    freq = __guac_common_surface_calculate_refresh_frequency(surface, x, y, w, h);

    /* If this rectangle is hot, mark lossy dirty rectangle. */
    if (freq >= GUAC_COMMON_SURFACE_LOSSY_REFRESH_FREQUENCY) {
        __guac_common_mark_lossy_dirty(surface, &rect);
    }
    /* Standard refresh path */
    else {

        /* Flush if not combining */
        if (!__guac_common_should_combine(surface, &rect, 0))
            guac_common_surface_flush_deferred(surface);

        /* Always defer draws */
        __guac_common_mark_dirty(surface, &rect);

    }

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
 * Flushes the bitmap update currently described by the dirty rectangle within the
 * given surface.
 *
 * @param surface The surface to flush.
 */
static void __guac_common_surface_flush_to_bitmap(guac_common_surface* surface) {

    if (surface->dirty) {

        guac_common_rect dirty_rect;
        guac_common_rect_init(&dirty_rect,
                              surface->dirty_rect.x,
                              surface->dirty_rect.y,
                              surface->dirty_rect.width,
                              surface->dirty_rect.height);

        /* Flush bitmap */
        __guac_common_surface_flush_to_bitmap_impl(surface, &dirty_rect, 0);

        surface->realized = 1;

        /* Surface is no longer dirty */
        surface->dirty = 0;

    }

}

/**
 * Comparator for instances of guac_common_surface_bitmap_rect, the elements
 * which make up a surface's bitmap buffer.
 *
 * @see qsort
 */
static int __guac_common_surface_bitmap_rect_compare(const void* a, const void* b) {

    guac_common_surface_bitmap_rect* ra = (guac_common_surface_bitmap_rect*) a;
    guac_common_surface_bitmap_rect* rb = (guac_common_surface_bitmap_rect*) b;

    /* Order roughly top to bottom, left to right */
    if (ra->rect.y != rb->rect.y) return ra->rect.y - rb->rect.y;
    if (ra->rect.x != rb->rect.x) return ra->rect.x - rb->rect.x;

    /* Wider updates should come first (more likely to intersect later) */
    if (ra->rect.width != rb->rect.width) return rb->rect.width - ra->rect.width;

    /* Shorter updates should come first (less likely to increase cost) */
    return ra->rect.height - rb->rect.height;

}

void guac_common_surface_flush(guac_common_surface* surface) {

    /* Update heat map. */
    guac_timestamp time = guac_timestamp_current();
    __guac_common_surface_update_heat_map(surface, time);

    /* Flush final dirty rectangle to queue. */
    __guac_common_surface_flush_to_queue(surface);

    /* Flush the lossy bitmap to client. */
    __guac_common_surface_flush_lossy_bitmap(surface);

    guac_common_surface_bitmap_rect* current = surface->bitmap_queue;
    int i, j;
    int original_queue_length;
    int flushed = 0;

    original_queue_length = surface->bitmap_queue_length;

    /* Sort updates to make combination less costly */
    qsort(surface->bitmap_queue, surface->bitmap_queue_length, sizeof(guac_common_surface_bitmap_rect),
          __guac_common_surface_bitmap_rect_compare);

    /* Flush all rects in queue */
    for (i=0; i < surface->bitmap_queue_length; i++) {

        /* Get next unflushed candidate */
        guac_common_surface_bitmap_rect* candidate = current;
        if (!candidate->flushed) {

            int combined = 0;

            /* Build up rect as much as possible */
            for (j=i; j < surface->bitmap_queue_length; j++) {

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
                    && surface->bitmap_queue_length < GUAC_COMMON_SURFACE_QUEUE_SIZE)
                __guac_common_surface_flush_to_queue(surface);

            /* Flush as bitmap otherwise */
            else {
                if (surface->dirty) flushed++;
                __guac_common_surface_flush_to_bitmap(surface);
            }

        }

        current++;

    }

    /* Flush complete */
    surface->bitmap_queue_length = 0;

}

