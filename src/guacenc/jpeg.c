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
#include "jpeg.h"
#include "log.h"

#include <stdio.h>
#include <unistd.h>

#include <cairo/cairo.h>
#include <jpeglib.h>

#include <stdlib.h>

cairo_surface_t* guacenc_jpeg_decoder(unsigned char* data, int length) {

    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    /* Create decompressor with standard error handling */
    jpeg_create_decompress(&cinfo);
    cinfo.err = jpeg_std_error(&jerr);

    /* Read JPEG directly from memory buffer */
    jpeg_mem_src(&cinfo, data, length);

    /* Read and validate JPEG header */
    if (!jpeg_read_header(&cinfo, TRUE)) {
        guacenc_log(GUAC_LOG_WARNING, "Invalid JPEG data");
        jpeg_destroy_decompress(&cinfo);
        return NULL;
    }

    /* Begin decompression */
    cinfo.out_color_space = JCS_EXT_BGRX;
    jpeg_start_decompress(&cinfo);

    /* Pull JPEG dimensions from decompressor */
    int width = cinfo.output_width;
    int height = cinfo.output_height;

    /* Create blank Cairo surface (no transparency in JPEG) */
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24,
            width, height);

    /* Pull underlying buffer and its stride */
    int stride = cairo_image_surface_get_stride(surface);
    unsigned char* row = cairo_image_surface_get_data(surface);

    /* Read JPEG into surface */
    while (cinfo.output_scanline < height) {
        unsigned char* buffers[1] = { row };
        jpeg_read_scanlines(&cinfo, buffers, 1);
        row += stride;
    }

    /* End decompression */
    jpeg_finish_decompress(&cinfo);

    /* Free decompressor */
    jpeg_destroy_decompress(&cinfo);

    /* JPEG was read successfully */
    return surface;

}

