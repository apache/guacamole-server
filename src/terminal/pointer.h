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


#ifndef GUAC_TERMINAL_POINTER_H
#define GUAC_TERMINAL_POINTER_H

#include "config.h"

#include <cairo/cairo.h>
#include <guacamole/client.h>

/**
 * Width of the embedded mouse cursor graphic.
 */
extern const int guac_terminal_pointer_width;

/**
 * Height of the embedded mouse cursor graphic.
 */
extern const int guac_terminal_pointer_height;

/**
 * Number of bytes in each row of the embedded mouse cursor graphic.
 */
extern const int guac_terminal_pointer_stride;

/**
 * The Cairo grapic format of the mouse cursor graphic.
 */
extern const cairo_format_t guac_terminal_pointer_format;

/**
 * Embedded mouse cursor graphic.
 */
extern unsigned char guac_terminal_pointer[];

/**
 * Creates a new pointer cursor, returning the corresponding cursor object.
 *
 * @param client
 *     The guac_client to send the cursor to.
 *
 * @return
 *     A new cursor which must be free'd via guac_terminal_cursor_free().
 */
guac_terminal_cursor* guac_terminal_create_pointer(guac_client* client);

#endif
