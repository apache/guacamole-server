/*
 * Copyright (C) 2013 Glyptodon LLC
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


#ifndef _GUAC_HASH_H
#define _GUAC_HASH_H

/**
 * Provides functions and structures for producing likely-to-be-unique hash
 * values for images.
 *
 * @file hash.h
 */

#include <cairo/cairo.h>

/**
 * Produces a 24-bit hash value from all pixels of the given surface. The
 * surface provided must be RGB or ARGB with each pixel stored in 32 bits.
 * The hashing algorithm used is a variant of the cyclic polynomial rolling
 * hash.
 *
 * @param surface The Cairo surface to hash.
 * @return An arbitrary 24-bit unsigned integer value intended to be well
 *         distributed across different images.
 */
unsigned int guac_hash_surface(cairo_surface_t* surface);

/**
 * Given two Cairo surfaces, returns zero if the data contained within each
 * is identical, and a positive or negative value if the value of the first
 * is found to be lexically greater or less than the second respectively.
 *
 * @param a The first Cairo surface to compare.
 * @param b The Cairo surface to compare the first surface against.
 * @return Zero if the data contained within each is identical, and a positive
 *         or negative value if the value of the first is found to be lexically
 *         greater or less than the second respectively.
 */
int guac_surface_cmp(cairo_surface_t* a, cairo_surface_t* b);

#endif

