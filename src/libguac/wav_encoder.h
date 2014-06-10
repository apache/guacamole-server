/*
 * Copyright (C) 2013 Glyptodon LLC
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


#ifndef __GUAC_WAV_ENCODER_H
#define __GUAC_WAV_ENCODER_H

#include "config.h"

#include "audio.h"

typedef struct wav_encoder_riff_header {

    /**
     * The RIFF chunk header, normally the string "RIFF".
     */
    unsigned char chunk_id[4];

    /**
     * Size of the entire file, not including chunk_id or chunk_size.
     */
    unsigned char chunk_size[4];

    /**
     * The format of this file, normally the string "WAVE".
     */
    unsigned char chunk_format[4];

} wav_encoder_riff_header;

typedef struct wav_encoder_fmt_header {

    /**
     * ID of this subchunk. For the fmt subchunk, this should be "fmt ".
     */
    unsigned char subchunk_id[4];

    /**
     * The size of the rest of this subchunk. For PCM, this will be 16.
     */
    unsigned char subchunk_size[4];

    /**
     * Format of this subchunk. For PCM, this will be 1.
     */
    unsigned char subchunk_format[2];

    /**
     * The number of channels in the PCM data.
     */
    unsigned char subchunk_channels[2];

    /**
     * The sample rate of the PCM data.
     */
    unsigned char subchunk_sample_rate[4];

    /**
     * The sample rate of the PCM data in bytes per second.
     */
    unsigned char subchunk_byte_rate[4];

    /**
     * The number of bytes per sample.
     */
    unsigned char subchunk_block_align[2];

    /**
     * The number of bits per sample.
     */
    unsigned char subchunk_bps[2];

} wav_encoder_fmt_header;

typedef struct wav_encoder_state {

    /**
     * Arbitrary PCM data available for writing when the overall WAV is
     * flushed.
     */
    unsigned char* data_buffer;

    /**
     * The number of bytes currently present in the data buffer.
     */
    int used;

    /**
     * The total number of bytes that can be written into the data buffer
     * without requiring resizing.
     */
    int length;

} wav_encoder_state;

typedef struct wav_encoder_data_header {

    /**
     * ID of this subchunk. For the data subchunk, this should be "data".
     */
    unsigned char subchunk_id[4];

    /**
     * The number of bytes in the PCM data.
     */
    unsigned char subchunk_size[4];

} wav_encoder_data_header;

extern guac_audio_encoder* wav_encoder;

#endif

