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

#include "config.h"

#include "audio.h"
#include "wav_encoder.h"

#include <stdlib.h>
#include <string.h>

#define WAV_BUFFER_SIZE 0x4000

void wav_encoder_begin_handler(guac_audio_stream* audio) {

    /* Allocate stream state */
    wav_encoder_state* state = (wav_encoder_state*)
        malloc(sizeof(wav_encoder_state));

    /* Initialize buffer */
    state->length = WAV_BUFFER_SIZE;
    state->used = 0;
    state->data_buffer = (unsigned char*) malloc(state->length);

    audio->data = state;

}

void _wav_encoder_write_le(unsigned char* buffer, int value, int length) {

    int offset;

    /* Write all bytes in the given value in little-endian byte order */
    for (offset=0; offset<length; offset++) {

        /* Store byte */
        *buffer = value & 0xFF;

        /* Move to next byte */
        value >>= 8;
        buffer++;

    }

}

void wav_encoder_end_handler(guac_audio_stream* audio) {

    /*
     * Static header init
     */

    wav_encoder_riff_header riff_header = {
        .chunk_id     = "RIFF",
        .chunk_format = "WAVE"
    };

    wav_encoder_fmt_header fmt_header = {
        .subchunk_id     = "fmt ",
        .subchunk_size   = {0x10, 0x00, 0x00, 0x00}, /* 16 */
        .subchunk_format = {0x01, 0x00}              /* 1 = PCM */
    };

    wav_encoder_data_header data_header = {
        .subchunk_id = "data"
    };

    /* Get state */
    wav_encoder_state* state = (wav_encoder_state*) audio->data;

    /*
     * RIFF HEADER
     */

    /* Chunk size */
    _wav_encoder_write_le(riff_header.chunk_size,
            4 + sizeof(fmt_header) + sizeof(data_header) + state->used,
            sizeof(riff_header.chunk_size));

    guac_audio_stream_write_encoded(audio,
            (unsigned char*) &riff_header,
            sizeof(riff_header));

    /*
     * FMT HEADER
     */

    /* Channels */
    _wav_encoder_write_le(fmt_header.subchunk_channels,
            audio->channels, sizeof(fmt_header.subchunk_channels));

    /* Sample rate */
    _wav_encoder_write_le(fmt_header.subchunk_sample_rate,
            audio->rate, sizeof(fmt_header.subchunk_sample_rate));

    /* Byte rate */
    _wav_encoder_write_le(fmt_header.subchunk_byte_rate,
            audio->rate * audio->channels * audio->bps / 8,
            sizeof(fmt_header.subchunk_byte_rate));

    /* Block align */
    _wav_encoder_write_le(fmt_header.subchunk_block_align,
            audio->channels * audio->bps / 8,
            sizeof(fmt_header.subchunk_block_align));

    /* Bits per second */
    _wav_encoder_write_le(fmt_header.subchunk_bps,
            audio->bps, sizeof(fmt_header.subchunk_bps));

    guac_audio_stream_write_encoded(audio,
            (unsigned char*) &fmt_header,
            sizeof(fmt_header));

    /*
     * DATA HEADER
     */

    /* PCM data size */
    _wav_encoder_write_le(data_header.subchunk_size,
            state->used, sizeof(data_header.subchunk_size));

    guac_audio_stream_write_encoded(audio,
            (unsigned char*) &data_header,
            sizeof(data_header));

    /* Write .wav data */
    guac_audio_stream_write_encoded(audio, state->data_buffer, state->used);

    /* Free stream state */
    free(state);

}

void wav_encoder_write_handler(guac_audio_stream* audio, 
        const unsigned char* pcm_data, int length) {

    /* Get state */
    wav_encoder_state* state = (wav_encoder_state*) audio->data;

    /* Increase size of buffer if necessary */
    if (state->used + length > state->length) {

        /* Increase to double concatenated size to accomodate */
        state->length = (state->length + length)*2;
        state->data_buffer = realloc(state->data_buffer,
                state->length);

    }

    /* Append to buffer */
    memcpy(&(state->data_buffer[state->used]), pcm_data, length);
    state->used += length;

}

/* Encoder handlers */
guac_audio_encoder _wav_encoder = {
    .mimetype      = "audio/wav",
    .begin_handler = wav_encoder_begin_handler,
    .write_handler = wav_encoder_write_handler,
    .end_handler   = wav_encoder_end_handler
};

/* Actual encoder */
guac_audio_encoder* wav_encoder = &_wav_encoder;

