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

#ifndef GUACENC_BUFFER_H
#define GUACENC_BUFFER_H

#include "config.h"

#include <cairo/cairo.h>

#include <stdbool.h>

/**
 * The image and size storage for either a buffer (a Guacamole layer with a
 * negative index) or a layer (a Guacamole layer with a non-negative index).
 */
typedef struct guacenc_buffer {

    /**
     * Whether this buffer should be automatically resized to fit any draw
     * operation.
     */
    bool autosize;

    /**
     * The width of this buffer or layer, in pixels.
     */
    int width;

    /**
     * The height of this buffer or layer, in pixels.
     */
    int height;

    /**
     * The number of bytes in each row of image data.
     */
    int stride;

    /**
     * The underlying image data of this surface. If the width or height of
     * this surface are 0, this will be NULL.
     */
    unsigned char* image;

    /**
     * The Cairo surface wrapping the underlying image data of this surface. If
     * the width or height of this surface are 0, this will be NULL.
     */
    cairo_surface_t* surface;

    /**
     * The current graphics context of the Cairo surface. If the width or
     * height of this surface are 0, this will be NULL.
     */
    cairo_t* cairo;

} guacenc_buffer;

/**
 * Allocates and initializes a new buffer object. This allocation is
 * independent of the Guacamole video encoder display; the allocated
 * guacenc_buffer will not automatically be associated with the active display.
 *
 * @return
 *     A newly-allocated and initialized guacenc_buffer, or NULL if allocation
 *     fails.
 */
guacenc_buffer* guacenc_buffer_alloc();

/**
 * Frees all memory associated with the given buffer object. If the buffer
 * provided is NULL, this function has no effect.
 *
 * @param buffer
 *     The buffer to free, which may be NULL.
 */
void guacenc_buffer_free(guacenc_buffer* buffer);

/**
 * Resizes the given buffer to the given dimensions, allocating or freeing
 * memory as necessary, and updating the buffer's width, height, and stride
 * properties.
 *
 * @param buffer
 *     The buffer to resize.
 *
 * @param width
 *     The new width of the buffer, in pixels.
 *
 * @param height
 *     The new height of the buffer, in pixels.
 *
 * @return
 *     Zero if the resize operation is successful, non-zero on error.
 */
int guacenc_buffer_resize(guacenc_buffer* buffer, int width, int height);

/**
 * Resizes the given buffer as necessary to contain at the given X/Y
 * coordinate, allocating or freeing memory as necessary, and updating the
 * buffer's width, height, and stride properties. If the buffer already
 * contains the given coordinate, this function has no effect.
 *
 * @param buffer
 *     The buffer to resize.
 *
 * @param x
 *     The X coordinate to ensure is within the buffer.
 *
 * @param y
 *     The Y coordinate to ensure is within the buffer.
 *
 * @return
 *     Zero if the resize operation is successful or no resize was performed,
 *     non-zero if the resize operation failed.
 */
int guacenc_buffer_fit(guacenc_buffer* buffer, int x, int y);

/**
 * Copies the entire contents of the given source buffer to the destination
 * buffer, ignoring the current contents of the destination. The destination
 * buffer's contents are entirely replaced.
 *
 * @param dst
 *     The destination buffer whose contents should be replaced.
 *
 * @param src
 *     The source buffer whose contents should replace those of the destination
 *     buffer.
 *
 * @return
 *     Zero if the copy operation was successful, non-zero on failure.
 */
int guacenc_buffer_copy(guacenc_buffer* dst, guacenc_buffer* src);

#endif

