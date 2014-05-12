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

void guac_common_rect_init(guac_common_rect* rect, int x, int y, int width, int height) {
    rect->x      = x;
    rect->y      = y;
    rect->width  = width;
    rect->height = height;
}

void guac_common_rect_extend(guac_common_rect* rect, const guac_common_rect* min) {

    /* Calculate extents of existing dirty rect */
    int left   = rect->x;
    int top    = rect->y;
    int right  = left + rect->width;
    int bottom = top  + rect->height;

    /* Calculate missing extents of given new rect */
    int min_left   = min->x;
    int min_top    = min->y;
    int min_right  = min_left + min->width;
    int min_bottom = min_top  + min->height;

    /* Update minimums */
    if (min_left   < left)   left   = min_left;
    if (min_top    < top)    top    = min_top;
    if (min_right  > right)  right  = min_right;
    if (min_bottom > bottom) bottom = min_bottom;

    /* Commit rect */
    guac_common_rect_init(rect, left, top, right - left, bottom - top);

}

void guac_common_rect_constrain(guac_common_rect* rect, const guac_common_rect* max) {

    /* Calculate extents of existing dirty rect */
    int left   = rect->x;
    int top    = rect->y;
    int right  = left + rect->width;
    int bottom = top  + rect->height;

    /* Calculate missing extents of given new rect */
    int max_left   = max->x;
    int max_top    = max->y;
    int max_right  = max_left + max->width;
    int max_bottom = max_top  + max->height;

    /* Update maximums */
    if (max_left   > left)   left   = max_left;
    if (max_top    > top)    top    = max_top;
    if (max_right  < right)  right  = max_right;
    if (max_bottom < bottom) bottom = max_bottom;

    /* Commit rect */
    guac_common_rect_init(rect, left, top, right - left, bottom - top);

}

