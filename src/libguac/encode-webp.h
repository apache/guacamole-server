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

#ifndef GUAC_ENCODE_WEBP_H
#define GUAC_ENCODE_WEBP_H

#include "config.h"

#include "socket.h"
#include "stream.h"

#include <cairo/cairo.h>

/**
 * Encodes the given surface as a WebP, and sends the resulting data over the
 * given stream and socket as blobs.
 *
 * @param socket
 *     The socket to send WebP blobs over.
 *
 * @param stream
 *     The stream to associate with each blob.
 *
 * @param surface
 *     The Cairo surface to write to the given stream and socket as PNG blobs.
 *
 * @param quality
 *     The WebP image quality to use. For lossy images, larger values indicate
 *     improving quality at the expense of larger file size. For lossless
 *     images, this dictates the quality of compression, with larger values
 *     producing smaller files at the expense of speed.
 *
 * @param lossless
 *     Zero for a lossy image, non-zero for lossless.
 *
 * @return
 *     Zero if the encoding operation is successful, non-zero otherwise.
 */
int guac_webp_write(guac_socket* socket, guac_stream* stream,
        cairo_surface_t* surface, int quality, int lossless);

#endif
