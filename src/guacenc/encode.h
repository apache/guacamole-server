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

#ifndef GUACENC_ENCODE_H
#define GUACENC_ENCODE_H

#include "config.h"

#include <stdbool.h>

/**
 * Encodes the given Guacamole protocol dump as video. A read lock will be
 * acquired on the input file to ensure that in-progress recordings are not
 * encoded. This behavior can be overridden by specifying true for the force
 * parameter.
 *
 * @param path
 *     The path to the file containing the raw Guacamole protocol dump.
 *
 * @param out_path
 *     The full path to the file in which encoded video should be written.
 *
 * @param codec
 *     The name of the codec to use for the video encoding, as defined by
 *     ffmpeg / libavcodec.
 *
 * @param width
 *     The width of the desired video, in pixels.
 *
 * @param height
 *     The height of the desired video, in pixels.
 *
 * @param bitrate
 *     The desired overall bitrate of the resulting encoded video, in bits per
 *     second.
 *
 * @param force
 *     Perform the encoding, even if the input file appears to be an
 *     in-progress recording (has an associated lock).
 *
 * @return
 *     Zero on success, non-zero if an error prevented successful encoding of
 *     the video.
 */
int guacenc_encode(const char* path, const char* out_path, const char* codec,
        int width, int height, int bitrate, bool force);

#endif

