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

#ifndef _GUAC_TIMESTAMP_H
#define _GUAC_TIMESTAMP_H

/**
 * Provides functions and structures for creating timestamps.
 *
 * @file timestamp.h
 */

#include "timestamp-types.h"

/**
 * Returns an arbitrary timestamp. The difference between return values of any
 * two calls is equal to the amount of time in milliseconds between those 
 * calls. The return value from a single call will not have any useful
 * (or defined) meaning.
 *
 * @return
 *     An arbitrary millisecond timestamp.
 */
guac_timestamp guac_timestamp_current();

/**
 * Sleeps for the given number of milliseconds.
 *
 * @param duration
 *     The number of milliseconds to sleep.
 */
void guac_timestamp_msleep(int duration);

#endif

