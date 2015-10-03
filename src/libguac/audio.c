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

#include "raw_encoder.h"

#include <guacamole/audio.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/stream.h>

#include <stdlib.h>
#include <string.h>

guac_audio_stream* guac_audio_stream_alloc(guac_client* client,
        guac_audio_encoder* encoder, int rate, int channels, int bps) {

    guac_audio_stream* audio;

    /* Choose an encoding if not specified */
    if (encoder == NULL) {

        int i;

        /* For each supported mimetype, check for an associated encoder */
        for (i=0; client->info.audio_mimetypes[i] != NULL; i++) {

            const char* mimetype = client->info.audio_mimetypes[i];

            /* If 16-bit raw audio is supported, done. */
            if (bps == 16 && strcmp(mimetype, raw16_encoder->mimetype) == 0) {
                encoder = raw16_encoder;
                break;
            }

            /* If 8-bit raw audio is supported, done. */
            if (bps == 8 && strcmp(mimetype, raw8_encoder->mimetype) == 0) {
                encoder = raw8_encoder;
                break;
            }

        } /* end for each mimetype */

        /* If still no encoder could be found, fail */
        if (encoder == NULL)
            return NULL;

    }

    /* Allocate stream */
    audio = (guac_audio_stream*) calloc(1, sizeof(guac_audio_stream));
    audio->client = client;

    /* Assign encoder */
    audio->encoder = encoder;
    audio->stream = guac_client_alloc_stream(client);

    /* Load PCM properties */
    audio->rate = rate;
    audio->channels = channels;
    audio->bps = bps;

    /* Call handler, if defined */
    if (audio->encoder->begin_handler)
        audio->encoder->begin_handler(audio);

    return audio;

}

void guac_audio_stream_reset(guac_audio_stream* audio,
        guac_audio_encoder* encoder, int rate, int channels, int bps) {

    /* Do nothing if nothing is changing */
    if ((encoder == NULL || encoder == audio->encoder)
            && rate     == audio->rate
            && channels == audio->channels
            && bps      == audio->bps) {
        return;
    }

    /* Free old encoder data */
    if (audio->encoder->end_handler)
        audio->encoder->end_handler(audio);

    /* Assign new encoder, if changed */
    if (encoder != NULL)
        audio->encoder = encoder;

    /* Set PCM properties */
    audio->rate = rate;
    audio->channels = channels;
    audio->bps = bps;

    /* Init encoder with new data */
    if (audio->encoder->begin_handler)
        audio->encoder->begin_handler(audio);

}

void guac_audio_stream_free(guac_audio_stream* audio) {

    /* Flush stream encoding */
    guac_audio_stream_flush(audio);

    /* Clean up encoder */
    if (audio->encoder->end_handler)
        audio->encoder->end_handler(audio);

    /* Free associated data */
    free(audio);

}

void guac_audio_stream_write_pcm(guac_audio_stream* audio, 
        const unsigned char* data, int length) {

    /* Write data */
    if (audio->encoder->write_handler)
        audio->encoder->write_handler(audio, data, length);

}

void guac_audio_stream_flush(guac_audio_stream* audio) {

    /* Flush any buffered data */
    if (audio->encoder->flush_handler)
        audio->encoder->flush_handler(audio);

}

