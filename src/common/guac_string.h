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

#ifndef __GUAC_COMMON_STRING_H
#define __GUAC_COMMON_STRING_H

#include "config.h"

/**
 * Counts the number of occurrences of a given character in a string.
 *
 * @param string The string to count occurrences within.
 * @param c The character to count occurrences of.
 * @return The number of occurrences.
 */
int guac_count_occurrences(const char* string, char c);

/**
 * Splits a string into a newly-allocated array of strings. The array itself
 * and each string within the array will eventually need to be freed. The array
 * is NULL-terminated.
 *
 * @param string The string to split.
 * @param delim The character which separates individual substrings within the
 *              given string.
 * @return A newly-allocated, NULL-terminated array of strings.
 */
char** guac_split(const char* string, char delim);

#endif

