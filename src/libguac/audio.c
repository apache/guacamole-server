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
#include <guacamole/user.h>

#include <stdlib.h>
#include <string.h>

/**
 * Assigns a new audio encoder to the given guac_audio_stream based on the
 * audio mimetypes declared as supported by the given user. If no audio encoder
 * can be found, no new audio encoder is assigned, and the existing encoder is
 * left untouched (if any).
 *
 * @param owner
 *     The user whose supported audio mimetypes should determine the audio
 *     encoder selected. It is expected that this user will be the owner of
 *     the connection.
 *
 * @param data
 *     The guac_audio_stream to which the new encoder should be assigned.
 *     Existing properties set on this audio stream, such as the bits per
 *     sample, may affect the encoder chosen.
 *
 * @return
 *     The assigned audio encoder. If no new audio encoder can be assigned,
 *     this will be the currently-assigned audio encoder (which may be NULL).
 */
static void* guac_audio_assign_encoder(guac_user* owner, void* data) {

    int i;

    guac_audio_stream* audio = (guac_audio_stream*) data;
    int bps = audio->bps;

    /* If there is no owner, do not attempt to assign a new encoder */
    if (owner == NULL)
        return audio->encoder;

    /* For each supported mimetype, check for an associated encoder */
    for (i=0; owner->info.audio_mimetypes[i] != NULL; i++) {

        const char* mimetype = owner->info.audio_mimetypes[i];

        /* If 16-bit raw audio is supported, done. */
        if (bps == 16 && strcmp(mimetype, raw16_encoder->mimetype) == 0) {
            audio->encoder = raw16_encoder;
            break;
        }

        /* If 8-bit raw audio is supported, done. */
        if (bps == 8 && strcmp(mimetype, raw8_encoder->mimetype) == 0) {
            audio->encoder = raw8_encoder;
            break;
        }

    } /* end for each mimetype */

    /* Return assigned encoder, if any */
    return audio->encoder;

}

guac_audio_stream* guac_audio_stream_alloc(guac_client* client,
        guac_audio_encoder* encoder, int rate, int channels, int bps) {

    guac_audio_stream* audio;

    /* Allocate stream */
    audio = (guac_audio_stream*) calloc(1, sizeof(guac_audio_stream));
    audio->client = client;
    audio->stream = guac_client_alloc_stream(client);

    /* Load PCM properties */
    audio->rate = rate;
    audio->channels = channels;
    audio->bps = bps;

    /* Assign encoder for owner, abort if no encoder can be found */
    if (!guac_client_for_owner(client, guac_audio_assign_encoder, audio)) {
        guac_client_free_stream(client, audio->stream);
        free(audio);
        return NULL;
    }

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

void guac_audio_stream_add_user(guac_audio_stream* audio, guac_user* user) {

    guac_audio_encoder* encoder = audio->encoder;

    /* Notify encoder that a new user is present */
    if (encoder->join_handler)
        encoder->join_handler(audio, user);

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

