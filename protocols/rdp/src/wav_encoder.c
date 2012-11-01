
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is libguac-client-rdp.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#define WAV_BUFFER_SIZE 0x4000

#include <stdlib.h>
#include <string.h>

#include <guacamole/client.h>
#include <guacamole/protocol.h>

#include "audio.h"
#include "wav_encoder.h"

void wav_encoder_begin_handler(audio_stream* audio) {

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

void wav_encoder_end_handler(audio_stream* audio) {

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

    audio_stream_write_encoded(audio,
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

    audio_stream_write_encoded(audio,
            (unsigned char*) &fmt_header,
            sizeof(fmt_header));

    /*
     * DATA HEADER
     */

    /* PCM data size */
    _wav_encoder_write_le(data_header.subchunk_size,
            state->used, sizeof(data_header.subchunk_size));

    audio_stream_write_encoded(audio,
            (unsigned char*) &data_header,
            sizeof(data_header));

    /* Write .wav data */
    audio_stream_write_encoded(audio, state->data_buffer, state->used);

    /* Free stream state */
    free(state);

}

void wav_encoder_write_handler(audio_stream* audio, 
        unsigned char* pcm_data, int length) {

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
audio_encoder _wav_encoder = {
    .mimetype      = "audio/wav",
    .begin_handler = wav_encoder_begin_handler,
    .write_handler = wav_encoder_write_handler,
    .end_handler   = wav_encoder_end_handler
};

/* Actual encoder */
audio_encoder* wav_encoder = &_wav_encoder;

