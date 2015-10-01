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

#include "config.h"

#include "audio.h"
#include "raw_encoder.h"

#include <guacamole/audio.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void raw_encoder_begin_handler(guac_audio_stream* audio) {

    raw_encoder_state* state;
    char mimetype[256];

    /* Produce mimetype string from format info */
    snprintf(mimetype, sizeof(mimetype), "audio/L%i;rate=%i,channels=%i",
            audio->bps, audio->rate, audio->channels);

    /* Associate stream */
    guac_protocol_send_audio(audio->client->socket, audio->stream, mimetype);

    /* Allocate and init encoder state */
    audio->data = state = malloc(sizeof(raw_encoder_state));
    state->written = 0;
    state->length = GUAC_RAW_ENCODER_BUFFER_SIZE
                    * audio->rate * audio->channels * audio->bps
                    / 8 / 1000;

    state->buffer = malloc(state->length);

    guac_client_log(audio->client, GUAC_LOG_DEBUG,
            "Using raw encoder (%s) with a %i byte buffer.",
            mimetype, state->length);

}

static void raw_encoder_end_handler(guac_audio_stream* audio) {

    raw_encoder_state* state = (raw_encoder_state*) audio->data;

    /* Send end of stream */
    guac_protocol_send_end(audio->client->socket, audio->stream);

    /* Free state information */
    free(state->buffer);
    free(state);

}

static void raw_encoder_write_handler(guac_audio_stream* audio, 
        const unsigned char* pcm_data, int length) {

    raw_encoder_state* state = (raw_encoder_state*) audio->data;

    while (length > 0) {

        /* Prefer to copy a chunk of equal size to available buffer space */
        int chunk_size = state->length - state->written;

        /* If no space remains, flush and retry */
        if (chunk_size == 0) {
            guac_audio_stream_flush(audio);
            continue;
        }

        /* Do not copy more data than is available in source PCM */
        if (chunk_size > length)
            chunk_size = length;

        /* Copy block of PCM data into buffer */
        memcpy(state->buffer + state->written, pcm_data, chunk_size);

        /* Advance to next block */
        state->written += chunk_size;
        pcm_data += chunk_size;
        length -= chunk_size;

    }

}

static void raw_encoder_flush_handler(guac_audio_stream* audio) {

    raw_encoder_state* state = (raw_encoder_state*) audio->data;
    guac_socket* socket = audio->client->socket;
    guac_stream* stream = audio->stream;

    unsigned char* current = state->buffer;
    int remaining = state->written;

    /* Flush all data in buffer as blobs */
    while (remaining > 0) {

        /* Determine size of blob to be written */
        int chunk_size = remaining;
        if (chunk_size > 6048)
            chunk_size = 6048;

        /* Send audio data */
        guac_protocol_send_blob(socket, stream, current, chunk_size);

        /* Advance to next blob */
        current += chunk_size;
        remaining -= chunk_size;

    }

    /* All data has been flushed */
    state->written = 0;

}

/* 8-bit raw encoder handlers */
guac_audio_encoder _raw8_encoder = {
    .mimetype      = "audio/L8",
    .begin_handler = raw_encoder_begin_handler,
    .write_handler = raw_encoder_write_handler,
    .flush_handler = raw_encoder_flush_handler,
    .end_handler   = raw_encoder_end_handler
};

/* 16-bit raw encoder handlers */
guac_audio_encoder _raw16_encoder = {
    .mimetype      = "audio/L16",
    .begin_handler = raw_encoder_begin_handler,
    .write_handler = raw_encoder_write_handler,
    .flush_handler = raw_encoder_flush_handler,
    .end_handler   = raw_encoder_end_handler
};

/* Actual encoder definitions */
guac_audio_encoder* raw8_encoder  = &_raw8_encoder;
guac_audio_encoder* raw16_encoder = &_raw16_encoder;

