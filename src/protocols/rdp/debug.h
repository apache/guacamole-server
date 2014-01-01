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


#ifndef __GUAC_RDP_DEBUG_H
#define __GUAC_RDP_DEBUG_H

#include "config.h"

#include <stdio.h>

/* Ensure GUAC_RDP_DEBUG_LEVEL is defined to a constant */
#ifndef GUAC_RDP_DEBUG_LEVEL
#define GUAC_RDP_DEBUG_LEVEL 0
#endif

/**
 * Prints a message to STDERR using the given printf format string and
 * arguments. This will only do anything if the GUAC_RDP_DEBUG_LEVEL
 * macro is defined and greater than the given log level.
 *
 * @param level The desired log level (an integer).
 * @param fmt The format to use when printing.
 * @param ... Arguments corresponding to conversion specifiers in the format
 *            string.
 */
#define GUAC_RDP_DEBUG(level, fmt, ...)                           \
    do {                                                          \
        if (GUAC_RDP_DEBUG_LEVEL >= level)                        \
            fprintf(stderr, "%s:%d: %s(): " fmt "\n",             \
                    __FILE__, __LINE__, __func__, __VA_ARGS__);   \
    } while (0);

#endif

