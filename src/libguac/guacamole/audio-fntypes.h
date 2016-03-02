/*
 * Copyright (C) 2016 Glyptodon LLC
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

#ifndef GUAC_AUDIO_FNTYPES_H
#define GUAC_AUDIO_FNTYPES_H

/**
 * Function type definitions related to simple streaming audio.
 *
 * @file audio-fntypes.h
 */

#include "audio-types.h"
#include "user-types.h"

/**
 * Handler which is called when the audio stream is opened.
 *
 * @param audio
 *     The audio stream being opened.
 */
typedef void guac_audio_encoder_begin_handler(guac_audio_stream* audio);

/**
 * Handler which is called when the audio stream needs to be flushed.
 *
 * @param audio
 *     The audio stream being flushed.
 */
typedef void guac_audio_encoder_flush_handler(guac_audio_stream* audio);

/**
 * Handler which is called when the audio stream is closed.
 *
 * @param audio
 *     The audio stream being closed.
 */
typedef void guac_audio_encoder_end_handler(guac_audio_stream* audio);

/**
 * Handler which is called when a new user has joined the Guacamole
 * connection associated with the audio stream.
 *
 * @param audio
 *     The audio stream associated with the Guacamole connection being
 *     joined.
 *
 * @param user
 *     The user that joined the connection.
 */
typedef void guac_audio_encoder_join_handler(guac_audio_stream* audio,
        guac_user* user);

/**
 * Handler which is called when PCM data is written to the audio stream. The
 * format of the PCM data is dictated by the properties of the audio stream.
 *
 * @param audio
 *     The audio stream to which data is being written.
 *
 * @param pcm_data
 *     A buffer containing the raw PCM data to be written.
 *
 * @param length
 *     The number of bytes within the buffer that should be written to the
 *     audio stream.
 */
typedef void guac_audio_encoder_write_handler(guac_audio_stream* audio,
        const unsigned char* pcm_data, int length);

#endif

