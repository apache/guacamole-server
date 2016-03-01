/*
 * Copyright (C) 2015 Glyptodon LLC
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

#ifndef GUAC_COMMON_BLANK_CURSOR_H
#define GUAC_COMMON_BLANK_CURSOR_H

#include "config.h"

#include <cairo/cairo.h>
#include <guacamole/user.h>

/**
 * Width of the embedded transparent (blank) mouse cursor graphic.
 */
extern const int guac_common_blank_cursor_width;

/**
 * Height of the embedded transparent (blank) mouse cursor graphic.
 */
extern const int guac_common_blank_cursor_height;

/**
 * Number of bytes in each row of the embedded transparent (blank) mouse cursor
 * graphic.
 */
extern const int guac_common_blank_cursor_stride;

/**
 * The Cairo grapic format of the transparent (blank) mouse cursor graphic.
 */
extern const cairo_format_t guac_common_blank_cursor_format;

/**
 * Embedded transparent (blank) mouse cursor graphic.
 */
extern unsigned char guac_common_blank_cursor[];

/**
 * Sets the cursor of the remote display to the embedded transparent (blank)
 * cursor graphic.
 *
 * @param user
 *     The guac_user to send the cursor to.
 */
void guac_common_set_blank_cursor(guac_user* user);

#endif

