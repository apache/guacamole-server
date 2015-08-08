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

#ifndef __GUAC_COMMON_RECT_H
#define __GUAC_COMMON_RECT_H

#include "config.h"

/**
 * Simple representation of a rectangle, having a defined corner and dimensions.
 */
typedef struct guac_common_rect {

    /**
     * The X coordinate of the upper-left corner of this rectangle.
     */
    int x;

    /**
     * The Y coordinate of the upper-left corner of this rectangle.
     */
    int y;

    /**
     * The width of this rectangle.
     */
    int width;

    /**
     * The height of this rectangle.
     */
    int height;

} guac_common_rect;

/**
 * Initialize the given rect with the given coordinates and dimensions.
 *
 * @param rect The rect to initialize.
 * @param x The X coordinate of the upper-left corner of the rect.
 * @param y The Y coordinate of the upper-left corner of the rect.
 * @param width The width of the rect.
 * @param height The height of the rect.
 */
void guac_common_rect_init(guac_common_rect* rect, int x, int y, int width, int height);

/**
 * Expand the rectangle to fit an NxN grid.
 *
 * The rectangle will be shifted to the left and up, expanded and adjusted to 
 * fit within the max bounding rect.
 *
 * @param cell_size
 *     The (NxN) grid cell size.
 *
 * @param rect
 *     The rectangle to adjust.
 *
 * @param max_rect
 *     The bounding area in which the given rect can exist.
 *
 * @return
 *     Zero on success, non-zero on error.
 */
int guac_common_rect_expand_to_grid(int cell_size, guac_common_rect* rect,
                                    const guac_common_rect* max_rect);

/**
 * Extend the given rect such that it contains at least the specified minimum
 * rect.
 *
 * @param rect The rect to extend.
 * @param min The minimum area which must be contained within the given rect.
 */
void guac_common_rect_extend(guac_common_rect* rect, const guac_common_rect* min);

/**
 * Collapse the given rect such that it exists only within the given maximum
 * rect.
 *
 * @param rect The rect to extend.
 * @param max The maximum area in which the given rect can exist.
 */
void guac_common_rect_constrain(guac_common_rect* rect, const guac_common_rect* max);

/**
 * Check whether a rectangle intersects another.
 *
 * @param rect
 *     Rectangle to check for intersection.
 *
 * @param other
 *     The other rectangle.
 *
 * @return
 *     Zero if no intersection, 1 if partial intersection,
 *     2 if first rect is completely inside the other.
 */
int guac_common_rect_intersects(const guac_common_rect* rect,
                                const guac_common_rect* other);

/**
 * Clip and split a rectangle into rectangles which are not covered by the
 * hole rectangle.
 *
 * This function will clip and split single edges when executed and must be
 * invoked until it returns zero. The edges are handled counter-clockwise
 * starting at the top edge.
 *
 * @param rect
 *     The rectangle to be split. This rectangle will be clipped by the
 *     split_rect.
 *
 * @param hole
 *     The rectangle which represents the hole.
 *
 * @param split_rect
 *     Resulting split rectangle.
 *
 * @return
 *     Zero when no splits were done, non-zero when the rectangle was split.
 */
int guac_common_rect_clip_and_split(guac_common_rect* rect,
        const guac_common_rect* hole, guac_common_rect* split_rect);

#endif

