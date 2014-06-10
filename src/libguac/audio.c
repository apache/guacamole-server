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

#ifdef ENABLE_OGG
#include "ogg_encoder.h"
#endif

#include "wav_encoder.h"

#include <guacamole/audio.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/stream.h>

#include <stdlib.h>
#include <string.h>

guac_audio_stream* guac_audio_stream_alloc(guac_client* client, guac_audio_encoder* encoder) {

    guac_audio_stream* audio;

    /* Choose an encoding if not specified */
    if (encoder == NULL) {

        int i;

        /* For each supported mimetype, check for an associated encoder */
        for (i=0; client->info.audio_mimetypes[i] != NULL; i++) {

            const char* mimetype = client->info.audio_mimetypes[i];

#ifdef ENABLE_OGG
            /* If Ogg is supported, done. */
            if (strcmp(mimetype, ogg_encoder->mimetype) == 0) {
                encoder = ogg_encoder;
                break;
            }
#endif

            /* If wav is supported, done. */
            if (strcmp(mimetype, wav_encoder->mimetype) == 0) {
                encoder = wav_encoder;
                break;
            }

        } /* end for each mimetype */

        /* If still no encoder could be found, fail */
        if (encoder == NULL)
            return NULL;

    }

    /* Allocate stream */
    audio = (guac_audio_stream*) malloc(sizeof(guac_audio_stream));
    audio->client = client;

    /* Reset buffer stats */
    audio->used = 0;
    audio->length = 0x40000;

    audio->encoded_data_used = 0;
    audio->encoded_data_length = 0x40000;

    /* Allocate buffers */
    audio->pcm_data = malloc(audio->length);
    audio->encoded_data = malloc(audio->encoded_data_length);

    /* Assign encoder */
    audio->encoder = encoder;
    audio->stream = guac_client_alloc_stream(client);

    return audio;
}

void guac_audio_stream_begin(guac_audio_stream* audio, int rate, int channels, int bps) {

    /* Load PCM properties */
    audio->rate = rate;
    audio->channels = channels;
    audio->bps = bps;

    /* Reset write counter */
    audio->pcm_bytes_written = 0;

    /* Call handler */
    audio->encoder->begin_handler(audio);

}

void guac_audio_stream_end(guac_audio_stream* audio) {

    double duration;

    /* Flush stream and finish encoding */
    guac_audio_stream_flush(audio);
    audio->encoder->end_handler(audio);

    /* Calculate duration of PCM data */
    duration = ((double) (audio->pcm_bytes_written * 1000 * 8))
                / audio->rate / audio->channels / audio->bps;

    /* Send audio */
    guac_protocol_send_audio(audio->client->socket, audio->stream,
            audio->stream->index, audio->encoder->mimetype, duration);

    guac_protocol_send_blob(audio->client->socket, audio->stream,
            audio->encoded_data, audio->encoded_data_used);

    guac_protocol_send_end(audio->client->socket, audio->stream);

    /* Clear data */
    audio->encoded_data_used = 0;

}

void guac_audio_stream_free(guac_audio_stream* audio) {
    free(audio->pcm_data);
    free(audio);
}

void guac_audio_stream_write_pcm(guac_audio_stream* audio, 
        const unsigned char* data, int length) {

    /* Update counter */
    audio->pcm_bytes_written += length;

    /* Resize audio buffer if necessary */
    if (length > audio->length) {

        /* Resize to double provided length */
        audio->length = length*2;
        audio->pcm_data = realloc(audio->pcm_data, audio->length);

    }

    /* Flush if necessary */
    if (audio->used + length > audio->length)
        guac_audio_stream_flush(audio);

    /* Append to buffer */
    memcpy(&(audio->pcm_data[audio->used]), data, length);
    audio->used += length;

}

void guac_audio_stream_flush(guac_audio_stream* audio) {

    /* If data in buffer */
    if (audio->used != 0) {

        /* Write data */
        audio->encoder->write_handler(audio,
                audio->pcm_data, audio->used);

        /* Reset buffer */
        audio->used = 0;

    }

}

void guac_audio_stream_write_encoded(guac_audio_stream* audio,
        const unsigned char* data, int length) {

    /* Resize audio buffer if necessary */
    if (audio->encoded_data_used + length > audio->encoded_data_length) {

        /* Increase to double concatenated size to accomodate */
        audio->encoded_data_length = (audio->encoded_data_length + length)*2;
        audio->encoded_data = realloc(audio->encoded_data,
                audio->encoded_data_length);

    }

    /* Append to buffer */
    memcpy(&(audio->encoded_data[audio->encoded_data_used]), data, length);
    audio->encoded_data_used += length;

}

