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

#include <cairo/cairo.h>

#include <stdlib.h>

guacenc_buffer* guacenc_buffer_alloc() {
    return calloc(1, sizeof(guacenc_buffer));
}

/**
 * Frees the underlying image data, surface, and graphics context of the given
 * buffer, marking each as unallocated.
 *
 * @param buffer
 *     The guacenc_buffer whose image data, surface, and graphics context
 *     should be freed.
 */
static void guacenc_buffer_free_image(guacenc_buffer* buffer) {

    /* Free graphics context */
    if (buffer->cairo != NULL) {
        cairo_destroy(buffer->cairo);
        buffer->cairo = NULL;
    }

    /* Free Cairo surface */
    if (buffer->surface != NULL) {
        cairo_surface_destroy(buffer->surface);
        buffer->surface = NULL;
    }

    /* Free image data (previously wrapped by Cairo surface */
    free(buffer->image);
    buffer->image = NULL;

}

void guacenc_buffer_free(guacenc_buffer* buffer) {

    /* Ignore NULL buffer */
    if (buffer == NULL)
        return;

    /* Free buffer and underlying image */
    guacenc_buffer_free_image(buffer);
    free(buffer);

}

int guacenc_buffer_resize(guacenc_buffer* buffer, int width, int height) {

    /* Simply deallocate if new image has absolutely no pixels */
    if (width == 0 || height == 0) {
        guacenc_buffer_free_image(buffer);
        buffer->width = width;
        buffer->height = height;
        buffer->stride = 0;
        return 0;
    }

    /* Allocate data for new image */
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
    unsigned char* image = calloc(1, stride*height);

    /* Wrap data in surface */
    cairo_surface_t* surface = cairo_image_surface_create_for_data(image,
            CAIRO_FORMAT_ARGB32, width, height, stride);

    /* Obtain graphics context of new surface */
    cairo_t* cairo = cairo_create(surface);

    /* Copy old surface, if defined */
    if (buffer->surface != NULL) {
        cairo_set_source_surface(cairo, buffer->surface, 0, 0);
        cairo_rectangle(cairo, 0, 0, width, height);
        cairo_paint(cairo);
    }

    /* Update properties */
    buffer->width = width;
    buffer->height = height;
    buffer->stride = stride;

    /* Replace old image */
    guacenc_buffer_free_image(buffer);
    buffer->image = image;
    buffer->surface = surface;
    buffer->cairo = cairo;

    return 0;

}

