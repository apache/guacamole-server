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


#ifndef __GUAC_AUDIO_H
#define __GUAC_AUDIO_H

/**
 * Provides functions and structures used for providing simple streaming audio.
 *
 * @file audio.h
 */

#include "audio-fntypes.h"
#include "audio-types.h"
#include "client-types.h"
#include "stream-types.h"

struct guac_audio_encoder {

    /**
     * The mimetype of the audio data encoded by this audio
     * encoder.
     */
    const char* mimetype;

    /**
     * Handler which will be called when the audio stream is first created.
     */
    guac_audio_encoder_begin_handler* begin_handler;

    /**
     * Handler which will be called when PCM data is written to the audio
     * stream for encoding.
     */
    guac_audio_encoder_write_handler* write_handler;

    /**
     * Handler which will be called when the audio stream is flushed.
     */
    guac_audio_encoder_flush_handler* flush_handler;

    /**
     * Handler which will be called when the audio stream is closed.
     */
    guac_audio_encoder_end_handler* end_handler;

};

struct guac_audio_stream {

    /**
     * Arbitrary codec encoder which will receive raw PCM data.
     */
    guac_audio_encoder* encoder;

    /**
     * The client associated with this audio stream.
     */
    guac_client* client;

    /**
     * The actual stream associated with this audio stream.
     */
    guac_stream* stream;

    /**
     * The number of samples per second of PCM data sent to this stream.
     */
    int rate;

    /**
     * The number of audio channels per sample of PCM data. Legal values are
     * 1 or 2.
     */
    int channels;

    /**
     * The number of bits per sample per channel for PCM data. Legal values are
     * 8 or 16.
     */
    int bps;

    /**
     * Encoder-specific state data.
     */
    void* data;

};

/**
 * Allocates a new audio stream which encodes audio data using the given
 * encoder. If NULL is specified for the encoder, an appropriate encoder
 * will be selected based on the encoders built into libguac and the level
 * of client support. The PCM format specified here (via rate, channels, and
 * bps) must be the format used for all PCM data provided to the audio stream.
 * The format may only be changed using guac_audio_stream_reset().
 *
 * @param client
 *     The guac_client for which this audio stream is being allocated.
 *
 * @param encoder
 *     The guac_audio_encoder to use when encoding audio, or NULL if libguac
 *     should select an appropriate built-in encoder on its own.
 *
 * @param rate
 *     The number of samples per second of PCM data sent to this stream.
 *
 * @param channels
 *     The number of audio channels per sample of PCM data. Legal values are
 *     1 or 2.
 *
 * @param bps
 *     The number of bits per sample per channel for PCM data. Legal values are
 *     8 or 16.
 *
 * @return
 *     The newly allocated guac_audio_stream, or NULL if no audio stream could
 *     be allocated due to lack of client support.
 */
guac_audio_stream* guac_audio_stream_alloc(guac_client* client,
        guac_audio_encoder* encoder, int rate, int channels, int bps);

/**
 * Resets the given audio stream, switching to the given encoder, rate,
 * channels, and bits per sample. If NULL is specified for the encoder, the
 * encoder is left unchanged. If the encoder, rate, channels, and bits per
 * sample are all identical to the current settings, this function has no
 * effect.
 *
 * @param encoder
 *     The guac_audio_encoder to use when encoding audio, or NULL to leave this
 *     unchanged.
 */
void guac_audio_stream_reset(guac_audio_stream* audio,
        guac_audio_encoder* encoder, int rate, int channels, int bps);

/**
 * Closes and frees the given audio stream.
 *
 * @param stream
 *     The guac_audio_stream to free.
 */
void guac_audio_stream_free(guac_audio_stream* stream);

/**
 * Writes PCM data to the given audio stream. This PCM data will be
 * automatically encoded by the audio encoder associated with this stream. The
 * PCM data must be 2-channel, 44100 Hz, with signed 16-bit samples.
 *
 * @param stream
 *     The guac_audio_stream to write PCM data through.
 *
 * @param data
 *     The PCM data to write.
 *
 * @param length
 *     The number of bytes of PCM data provided.
 */
void guac_audio_stream_write_pcm(guac_audio_stream* stream,
        const unsigned char* data, int length);

/**
 * Flushes the underlying audio buffer, if any, ensuring that all audio
 * previously written via guac_audio_stream_write_pcm() has been encoded and
 * sent to the client.
 *
 * @param stream
 *     The guac_audio_stream whose audio buffers should be flushed.
 */
void guac_audio_stream_flush(guac_audio_stream* stream);

#endif

