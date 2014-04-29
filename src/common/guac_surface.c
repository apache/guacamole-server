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
#include "guac_surface.h"

#include <guacamole/layer.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>

guac_common_surface* guac_common_surface_alloc(guac_socket* socket, const guac_layer* layer, int w, int h) {
    /* STUB */
    return NULL;
}

void guac_common_surface_free(guac_common_surface* surface) {
    /* STUB */
}

void guac_common_surface_draw(guac_common_surface* surface, cairo_surface_t* src, int x, int y) {
    /* STUB */
}

void guac_common_surface_copy(guac_common_surface* src, int sx, int sy, int w, int h,
                              guac_common_surface* dst, int dx, int dy) {
    /* STUB */
}

void guac_common_surface_transfer(guac_transfer_function op,
                                  guac_common_surface* src, int sx, int sy, int w, int h,
                                  guac_common_surface* dst, int dx, int dy) {
    /* STUB */
}

void guac_common_surface_resize(guac_common_surface* surface, int w, int h) {
    /* STUB */
}

void guac_common_surface_rect(guac_common_surface* surface,
                              int x, int y, int w, int h,
                              int red, int green, int blue) {
    /* STUB */
}

void guac_common_surface_flush(guac_common_surface* surface) {
    /* STUB */
}

