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

#ifndef __GUAC_AUDIO_FNTYPES_H
#define __GUAC_AUDIO_FNTYPES_H

/**
 * Function type definitions related to simple streaming audio.
 *
 * @file audio-fntypes.h
 */

#include "audio-types.h"

/**
 * Handler which is called when the audio stream is opened.
 */
typedef void guac_audio_encoder_begin_handler(guac_audio_stream* audio);

/**
 * Handler which is called when the audio stream needs to be flushed.
 */
typedef void guac_audio_encoder_flush_handler(guac_audio_stream* audio);

/**
 * Handler which is called when the audio stream is closed.
 */
typedef void guac_audio_encoder_end_handler(guac_audio_stream* audio);

/**
 * Handler which is called when PCM data is written to the audio stream.
 */
typedef void guac_audio_encoder_write_handler(guac_audio_stream* audio,
        const unsigned char* pcm_data, int length);

#endif

