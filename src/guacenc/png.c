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
#include "buffer.h"
#include "image-stream.h"
#include "png.h"

#include <stdlib.h>

int guacenc_png_init(guacenc_image_stream* stream) {
    /* STUB */
    return 0;
}

int guacenc_png_data(guacenc_image_stream* stream, unsigned char* data,
        int length) {
    /* STUB */
    return 0;
}

int guacenc_png_end(guacenc_image_stream* stream, guacenc_buffer* buffer) {
    /* STUB */
    return 0;
}

int guacenc_png_free(guacenc_image_stream* stream) {
    /* STUB */
    return 0;
}

guacenc_decoder guacenc_png_decoder = {
    .init_handler = guacenc_png_init,
    .data_handler = guacenc_png_data,
    .end_handler  = guacenc_png_end,
    .free_handler = guacenc_png_free
};

