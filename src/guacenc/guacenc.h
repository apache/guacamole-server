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

#ifndef GUACENC_H
#define GUACENC_H

#include "config.h"

/**
 * The name of the codec to use by default, if no other codec is specified on
 * the command line. This name is dictated by ffmpeg / libavcodec.
 */
#define GUACENC_DEFAULT_CODEC "mpeg4"

/**
 * The extension to append to the end of the input file to produce the output
 * file name, excluding the separating period, if no other suffix is specified
 * on the command line.
 */
#define GUACENC_DEFAULT_SUFFIX "mpg"

/**
 * The width of the output video, in pixels, if no other width is given on the
 * command line. Note that different codecs will have different restrictions
 * regarding legal widths.
 */
#define GUACENC_DEFAULT_WIDTH 640

/**
 * The height of the output video, in pixels, if no other height is given on the
 * command line. Note that different codecs will have different restrictions
 * regarding legal heights.
 */
#define GUACENC_DEFAULT_HEIGHT 480

/**
 * The desired bitrate of the output video, in bits per second, if no other
 * bitrate is given on the command line.
 */
#define GUACENC_DEFAULT_BITRATE 2000000

#endif

