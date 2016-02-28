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
#include "log.h"
#include "png.h"

#include <cairo/cairo.h>

#include <stdlib.h>
#include <string.h>

/**
 * The current state of the PNG decoder.
 */
typedef struct guacenc_png_read_state {

    /**
     * The buffer of unread image data. This pointer will be updated to point
     * to the next unread byte when data is read.
     */
    unsigned char* data;

    /**
     * The number of bytes remaining to be read within the buffer.
     */
    unsigned int length;

} guacenc_png_read_state;

/**
 * Attempts to fill the given buffer with read image data. The behavior of
 * this function is dictated by cairo_read_t.
 *
 * @param closure
 *     The current state of the PNG decoding process (an instance of
 *     guacenc_png_read_state).
 *
 * @param data
 *     The data buffer to fill.
 *
 * @param length
 *     The number of bytes to fill within the data buffer.
 *
 * @return
 *     CAIRO_STATUS_SUCCESS if all data was read successfully (the entire
 *     buffer was filled), CAIRO_STATUS_READ_ERROR otherwise.
 */
static cairo_status_t guacenc_png_read(void* closure, unsigned char* data,
        unsigned int length) {

    guacenc_png_read_state* state = (guacenc_png_read_state*) closure;

    /* If more data is requested than is available in buffer, fail */
    if (length > state->length)
        return CAIRO_STATUS_READ_ERROR;

    /* Read chunk into buffer */
    memcpy(data, state->data, length);

    /* Advance to next chunk */
    state->length -= length;
    state->data += length;

    /* Read was successful */
    return CAIRO_STATUS_SUCCESS;

}

cairo_surface_t* guacenc_png_decoder(unsigned char* data, int length) {

    guacenc_png_read_state state = {
        .data = data,
        .length = length
    };

    /* Read PNG from data */
    cairo_surface_t* surface =
        cairo_image_surface_create_from_png_stream(guacenc_png_read, &state);

    /* If surface returned with an error, just return NULL */
    if (surface != NULL &&
            cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        guacenc_log(GUAC_LOG_WARNING, "Invalid PNG data");
        cairo_surface_destroy(surface);
        return NULL;
    }

    /* PNG was read successfully */
    return surface;

}

