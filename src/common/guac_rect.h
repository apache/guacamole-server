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

#endif

