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

#ifndef _GUAC_POOL_H
#define _GUAC_POOL_H

/**
 * Provides functions and structures for maintaining dynamically allocated and
 * freed pools of integers.
 *
 * @file pool.h
 */

#include "pool-types.h"

struct guac_pool {

    /**
     * The minimum number of integers which must have been returned by
     * guac_pool_next_int before previously-used and freed integers are
     * allowed to be returned.
     */
    int min_size;

    /**
     * The number of integers currently in use.
     */
    int active;

    /**
     * The next integer to be released (after no more integers remain in the
     * pool.
     */
    int __next_value;

    /**
     * The first integer in the pool, if any.
     */
    guac_pool_int* __head;

    /**
     * The last integer in the pool, if any.
     */
    guac_pool_int* __tail;

};

struct guac_pool_int {

    /**
     * The integer value of this pool entry.
     */
    int value;

    /**
     * The next available (unused) guac_pool_int in the list of
     * allocated but free'd ints.
     */
    guac_pool_int* __next;

};

/**
 * Allocates a new guac_pool having the given minimum size.
 *
 * @param size The minimum number of integers which must have been returned by
 *             guac_pool_next_int before freed integers (previously used
 *             integers) are allowed to be returned.
 * @return A new, empty guac_pool, having the given minimum size.
 */
guac_pool* guac_pool_alloc(int size);

/**
 * Frees the given guac_pool.
 *
 * @param pool The guac_pool to free.
 */
void guac_pool_free(guac_pool* pool);

/**
 * Returns the next available integer from the given guac_pool. All integers
 * returned are non-negative, and are returned in sequences, starting from 0.
 *
 * @param pool The guac_pool to retrieve an integer from.
 * @return The next available integer, which may be either an integer not yet
 *         returned by a call to guac_pool_next_int, or an integer which was
 *         previosly returned, but has since been freed.
 */
int guac_pool_next_int(guac_pool* pool);

/**
 * Frees the given integer back into the given guac_pool. The integer given
 * will be available for future calls to guac_pool_next_int.
 *
 * @param pool The guac_pool to free the given integer into.
 * @param value The integer which should be returned to the given pool, such
 *              that it can be received by a future call to guac_pool_next_int.
 */
void guac_pool_free_int(guac_pool* pool, int value);

#endif

