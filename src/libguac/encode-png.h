/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef GUAC_ENCODE_PNG_H
#define GUAC_ENCODE_PNG_H

#include "guacamole/socket.h"
#include "guacamole/stream.h"

#include <cairo/cairo.h>

/**
 * Encodes the given surface as a PNG, and sends the resulting data over the
 * given stream and socket as blobs.
 *
 * This legacy entry point always uses the original PNG encoding policy.
 *
 * @param socket
 *     The socket to send PNG blobs over.
 *
 * @param stream
 *     The stream to associate with each blob.
 *
 * @param surface
 *     The Cairo surface to write to the given stream and socket as PNG blobs.
 *
 * @return
 *     Zero if the encoding operation is successful, non-zero otherwise.
 */
int guac_png_write(guac_socket* socket, guac_stream* stream,
        cairo_surface_t* surface);

/**
 * Writes PNG blobs, optionally using lossless fast RGB24 encoding when the
 * supplied connection option is exactly 1. Non-RGB24 images and all disabled
 * options retain the existing fallback. Codec selection is unchanged.
 *
 * @param socket
 *     The output socket.
 *
 * @param stream
 *     The output stream.
 *
 * @param surface
 *     The Cairo surface to encode.
 *
 * @param fast_png_encoding
 *     Exactly 1 to select the faster opaque RGB24 encoder, zero otherwise.
 *
 * @return
 *     Zero on success, non-zero on error.
 */
int guac_png_write_with_options(guac_socket* socket, guac_stream* stream,
        cairo_surface_t* surface, int fast_png_encoding);

#endif

