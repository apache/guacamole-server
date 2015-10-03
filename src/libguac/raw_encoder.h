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


#ifndef GUAC_RAW_ENCODER_H
#define GUAC_RAW_ENCODER_H

#include "config.h"

#include "audio.h"

/**
 * The number of bytes to send in each audio blob.
 */
#define GUAC_RAW_ENCODER_BLOB_SIZE 6048

/**
 * The size of the raw encoder output PCM buffer, in milliseconds. The
 * equivalent size in bytes will vary by PCM rate, number of channels, and bits
 * per sample.
 */
#define GUAC_RAW_ENCODER_BUFFER_SIZE 250

/**
 * The current state of the raw encoder. The raw encoder performs very minimal
 * processing, buffering provided PCM data only as necessary to ensure audio
 * packet sizes are reasonable.
 */
typedef struct raw_encoder_state {

    /**
     * Buffer of not-yet-written raw PCM data.
     */
    unsigned char* buffer;

    /**
     * Size of the PCM buffer, in bytes.
     */
    int length;

    /**
     * The current number of bytes stored within the PCM buffer.
     */
    int written;

} raw_encoder_state;

/**
 * Audio encoder which writes 8-bit raw PCM (one byte per sample).
 */
extern guac_audio_encoder* raw8_encoder;

/**
 * Audio encoder which writes 16-bit raw PCM (two bytes per sample).
 */
extern guac_audio_encoder* raw16_encoder;

#endif

