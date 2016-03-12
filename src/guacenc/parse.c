/*
 * Copyright (C) 2016 Glyptodon, Inc.
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

#include <errno.h>
#include <limits.h>
#include <string.h>
#include <stdlib.h>

int guacenc_parse_int(char* arg, int* i) {

    char* end;

    /* Parse string as an integer */
    errno = 0;
    long int value = strtol(arg, &end, 10);

    /* Ignore number if invalid / non-positive */
    if (errno != 0 || value <= 0 || value > INT_MAX || *end != '\0')
        return 1;

    /* Store value */
    *i = value;

    /* Parsing successful */
    return 0;

}

int guacenc_parse_dimensions(char* arg, int* width, int* height) {

    /* Locate the 'x' within the dimensions string */
    char* x = strchr(arg, 'x');
    if (x == NULL)
        return 1;

    /* Replace 'x' with a null terminator */
    *x = '\0';

    /* Parse width and height */
    int w, h;
    if (guacenc_parse_int(arg, &w) || guacenc_parse_int(x+1, &h))
        return 1;

    /* Width and height are both valid */
    *width = w;
    *height = h;

    return 0;

}

