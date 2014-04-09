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
     * Handler which will be called when the audio stream is opened.
     */
    guac_audio_encoder_begin_handler* begin_handler;

    /**
     * Handler which will be called when the audio stream is flushed.
     */
    guac_audio_encoder_write_handler* write_handler;

    /**
     * Handler which will be called when the audio stream is closed.
     */
    guac_audio_encoder_end_handler* end_handler;

};

struct guac_audio_stream {

    /**
     * PCM data buffer, 16-bit samples, 2-channel, 44100 Hz.
     */
    unsigned char* pcm_data;

    /**
     * Number of bytes in buffer.
     */
    int used;

    /**
     * Maximum number of bytes in buffer.
     */
    int length;

    /**
     * Encoded audio data buffer, as written by the encoder.
     */
    unsigned char* encoded_data;

    /**
     * Number of bytes in the encoded data buffer.
     */
    int encoded_data_used;

    /**
     * Maximum number of bytes in the encoded data buffer.
     */
    int encoded_data_length;

    /**
     * Arbitrary codec encoder. When the PCM buffer is flushed, PCM data will
     * be sent to this encoder.
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
     * The number of PCM bytes written since the audio chunk began.
     */
    int pcm_bytes_written;

    /**
     * Encoder-specific state data.
     */
    void* data;

};

/**
 * Allocates a new audio stream which encodes audio data using the given
 * encoder. If NULL is specified for the encoder, an appropriate encoder
 * will be selected based on the encoders built into libguac and the level
 * of client support.
 *
 * @param client The guac_client for which this audio stream is being
 *               allocated.
 * @param encoder The guac_audio_encoder to use when encoding audio, or
 *                NULL if libguac should select an appropriate built-in
 *                encoder on its own.
 * @return The newly allocated guac_audio_stream, or NULL if no audio
 *         stream could be allocated due to lack of client support.
 */
guac_audio_stream* guac_audio_stream_alloc(guac_client* client,
        guac_audio_encoder* encoder);

/**
 * Frees the given audio stream.
 *
 * @param stream The guac_audio_stream to free.
 */
void guac_audio_stream_free(guac_audio_stream* stream);

/**
 * Begins a new audio packet within the given audio stream. This packet will be
 * built up with repeated writes of PCM data, finally being sent when complete
 * via guac_audio_stream_end().
 *
 * @param stream The guac_audio_stream which should start a new audio packet.
 * @param rate The audio rate of the packet, in Hz.
 * @param channels The number of audio channels.
 * @param bps The number of bits per audio sample.
 */
void guac_audio_stream_begin(guac_audio_stream* stream, int rate, int channels, int bps);

/**
 * Ends the current audio packet, writing the finished packet as an audio
 * instruction.
 *
 * @param stream The guac_audio_stream whose current audio packet should be
 *               completed and sent.
 */
void guac_audio_stream_end(guac_audio_stream* stream);

/**
 * Writes PCM data to the given audio stream. This PCM data will be
 * automatically encoded by the audio encoder associated with this stream. This
 * function must only be called after an audio packet has been started with
 * guac_audio_stream_begin().
 *
 * @param stream The guac_audio_stream to write PCM data through.
 * @param data The PCM data to write.
 * @param length The number of bytes of PCM data provided.
 */
void guac_audio_stream_write_pcm(guac_audio_stream* stream,
        const unsigned char* data, int length);

/**
 * Flushes the given audio stream.
 *
 * @param stream The guac_audio_stream to flush.
 */
void guac_audio_stream_flush(guac_audio_stream* stream);

/**
 * Appends arbitrarily-encoded data to the encoded_data buffer within the given
 * audio stream. This data must be encoded in the output format of the encoder
 * used by the stream. This function is mainly for use by encoder
 * implementations.
 *
 * @param audio The guac_audio_stream to write data through.
 * @param data Arbitrary encoded data to write through the audio stream.
 * @param length The number of bytes of data provided.
 */
void guac_audio_stream_write_encoded(guac_audio_stream* audio,
        const unsigned char* data, int length);

#endif

