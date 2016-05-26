/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "config.h"
#include "audio_input.h"
#include "dvc.h"
#include "ptr_string.h"
#include "rdp.h"

#include <freerdp/freerdp.h>
#include <freerdp/channels/channels.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>
#include <guacamole/user.h>

#include <errno.h>
#include <stdlib.h>
#include <pthread.h>

/**
 * Parses the given raw audio mimetype, producing the corresponding rate,
 * number of channels, and bytes per sample.
 *
 * @param mimetype
 *     The raw auduio mimetype to parse.
 *
 * @param rate
 *     A pointer to an int where the sample rate for the PCM format described
 *     by the given mimetype should be stored.
 *
 * @param channels
 *     A pointer to an int where the number of channels used by the PCM format
 *     described by the given mimetype should be stored.
 *
 * @param bps
 *     A pointer to an int where the number of bytes used the PCM format for
 *     each sample (independent of number of channels) described by the given
 *     mimetype should be stored.
 *
 * @return
 *     Zero if the given mimetype is a raw audio mimetype and has been parsed
 *     successfully, non-zero otherwise.
 */
static int guac_rdp_audio_parse_mimetype(const char* mimetype,
        int* rate, int* channels, int* bps) {

    int parsed_rate = -1;
    int parsed_channels = 1;
    int parsed_bps;

    /* PCM audio with one byte per sample */
    if (strncmp(mimetype, "audio/L8;", 9) == 0) {
        mimetype += 8; /* Advance to semicolon ONLY */
        parsed_bps = 1;
    }

    /* PCM audio with two bytes per sample */
    else if (strncmp(mimetype, "audio/L16;", 10) == 0) {
        mimetype += 9; /* Advance to semicolon ONLY */
        parsed_bps = 2;
    }

    /* Unsupported mimetype */
    else
        return 1;

    /* Parse each parameter name/value pair within the mimetype */
    do {

        /* Advance to first character of parameter (current is either a
         * semicolon or a comma) */
        mimetype++;

        /* Parse number of channels */
        if (strncmp(mimetype, "channels=", 9) == 0) {

            mimetype += 9;
            parsed_channels = strtol(mimetype, (char**) &mimetype, 10);

            /* Fail if value invalid / out of range */
            if (errno == EINVAL || errno == ERANGE)
                return 1;

        }

        /* Parse number of rate */
        else if (strncmp(mimetype, "rate=", 5) == 0) {

            mimetype += 5;
            parsed_rate = strtol(mimetype, (char**) &mimetype, 10);

            /* Fail if value invalid / out of range */
            if (errno == EINVAL || errno == ERANGE)
                return 1;

        }

        /* Advance to next parameter */
        mimetype = strchr(mimetype, ',');

    } while (mimetype != NULL);

    /* Mimetype is invalid if rate was not specified */
    if (parsed_rate == -1)
        return 1;

    /* Parse success */
    *rate = parsed_rate;
    *channels = parsed_channels;
    *bps = parsed_bps;

    return 0;

}

int guac_rdp_audio_handler(guac_user* user, guac_stream* stream,
        char* mimetype) {

    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    int rate;
    int channels;
    int bps;

    /* Parse mimetype, abort on parse error */
    if (guac_rdp_audio_parse_mimetype(mimetype, &rate, &channels, &bps)) {
        guac_user_log(user, GUAC_LOG_WARNING, "Denying user audio stream with "
                "unsupported mimetype: \"%s\"", mimetype);
        guac_protocol_send_ack(user->socket, stream, "Unsupported audio "
                "mimetype", GUAC_PROTOCOL_STATUS_CLIENT_BAD_TYPE);
        return 0;
    }

    /* Init stream data */
    stream->blob_handler = guac_rdp_audio_blob_handler;
    stream->end_handler = guac_rdp_audio_end_handler;

    /* Associate stream with audio buffer */
    guac_rdp_audio_buffer_set_stream(rdp_client->audio_input, user, stream,
            rate, channels, bps);

    return 0;

}

int guac_rdp_audio_blob_handler(guac_user* user, guac_stream* stream,
        void* data, int length) {

    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Write blob to audio stream, buffering if necessary */
    guac_rdp_audio_buffer_write(rdp_client->audio_input, data, length);

    return 0;

}

int guac_rdp_audio_end_handler(guac_user* user, guac_stream* stream) {

    /* Ignore - the AUDIO_INPUT channel will simply not receive anything */
    return 0;

}

void guac_rdp_audio_load_plugin(rdpContext* context, guac_rdp_dvc_list* list) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;

    /* Add "AUDIO_INPUT" channel */
    guac_rdp_dvc_list_add(list, "guacai", guac_rdp_ptr_to_string(client), NULL);

}

guac_rdp_audio_buffer* guac_rdp_audio_buffer_alloc() {
    guac_rdp_audio_buffer* buffer = calloc(1, sizeof(guac_rdp_audio_buffer));
    pthread_mutex_init(&(buffer->lock), NULL);
    return buffer;
}

/**
 * Sends an "ack" instruction over the socket associated with the Guacamole
 * stream over which audio data is being received. The "ack" instruction will
 * only be sent if the Guacamole audio stream has been established (through
 * receipt of an "audio" instruction), is still open (has not received an "end"
 * instruction nor been associated with an "ack" having an error code), and is
 * associated with an active RDP AUDIO_INPUT channel.
 *
 * @param audio_buffer
 *     The audio buffer associated with the guac_stream for which the "ack"
 *     instruction should be sent, if any. If there is no associated
 *     guac_stream, this function has no effect.
 *
 * @param message
 *     An arbitrary human-readable message to send along with the "ack".
 *
 * @param status
 *     The Guacamole protocol status code to send with the "ack". This should
 *     be GUAC_PROTOCOL_STATUS_SUCCESS if the audio stream has been set up
 *     successfully or GUAC_PROTOCOL_STATUS_RESOURCE_CLOSED if the audio stream
 *     has been closed (but may usable again if reopened).
 */
static void guac_rdp_audio_buffer_ack(guac_rdp_audio_buffer* audio_buffer,
        const char* message, guac_protocol_status status) {

    guac_user* user = audio_buffer->user;
    guac_stream* stream = audio_buffer->stream;

    /* Do not send ack unless both sides of the audio stream are ready */
    if (user == NULL || stream == NULL || audio_buffer->packet == NULL)
        return;

    /* Send ack instruction */
    guac_protocol_send_ack(user->socket, stream, message, status);
    guac_socket_flush(user->socket);

}

void guac_rdp_audio_buffer_set_stream(guac_rdp_audio_buffer* audio_buffer,
        guac_user* user, guac_stream* stream, int rate, int channels, int bps) {

    pthread_mutex_lock(&(audio_buffer->lock));

    /* Associate received stream */
    audio_buffer->user = user;
    audio_buffer->stream = stream;
    audio_buffer->in_format.rate = rate;
    audio_buffer->in_format.channels = channels;
    audio_buffer->in_format.bps = bps;

    /* Acknowledge stream creation (if buffer is ready to receive) */
    guac_rdp_audio_buffer_ack(audio_buffer,
            "OK", GUAC_PROTOCOL_STATUS_SUCCESS);

    guac_user_log(user, GUAC_LOG_DEBUG, "User is requesting to provide audio "
            "input as %i-channel, %i Hz PCM audio at %i bytes/sample.",
            audio_buffer->in_format.channels,
            audio_buffer->in_format.rate,
            audio_buffer->in_format.bps);

    pthread_mutex_unlock(&(audio_buffer->lock));

}

void guac_rdp_audio_buffer_set_output(guac_rdp_audio_buffer* audio_buffer,
        int rate, int channels, int bps) {

    pthread_mutex_lock(&(audio_buffer->lock));

    /* Set output format */
    audio_buffer->out_format.rate = rate;
    audio_buffer->out_format.channels = channels;
    audio_buffer->out_format.bps = bps;

    pthread_mutex_unlock(&(audio_buffer->lock));

}

void guac_rdp_audio_buffer_begin(guac_rdp_audio_buffer* audio_buffer,
        int packet_frames, guac_rdp_audio_buffer_flush_handler* flush_handler,
        void* data) {

    pthread_mutex_lock(&(audio_buffer->lock));

    /* Reset buffer state to provided values */
    audio_buffer->bytes_written = 0;
    audio_buffer->flush_handler = flush_handler;
    audio_buffer->data = data;

    /* Calculate size of each packet in bytes */
    audio_buffer->packet_size = packet_frames
                              * audio_buffer->out_format.channels
                              * audio_buffer->out_format.bps;

    /* Allocate new buffer */
    free(audio_buffer->packet);
    audio_buffer->packet = malloc(audio_buffer->packet_size);

    /* Acknowledge stream creation (if stream is ready to receive) */
    guac_rdp_audio_buffer_ack(audio_buffer,
            "OK", GUAC_PROTOCOL_STATUS_SUCCESS);

    pthread_mutex_unlock(&(audio_buffer->lock));

}

void guac_rdp_audio_buffer_write(guac_rdp_audio_buffer* audio_buffer,
        char* buffer, int length) {

    pthread_mutex_lock(&(audio_buffer->lock));

    /* FIXME: Assuming mimetype of "audio/L16;rate=44100,channels=2" */

    /* Ignore packet if there is no buffer */
    if (audio_buffer->packet_size == 0 || audio_buffer->packet == NULL) {
        pthread_mutex_unlock(&(audio_buffer->lock));
        return;
    }

    /* Continuously write packets until no data remains */
    while (length > 0) {

        /* Calculate ideal size of chunk based on available space */
        int chunk_size = audio_buffer->packet_size
                       - audio_buffer->bytes_written;

        /* Shrink chunk size if insufficient bytes are provided */
        if (length < chunk_size)
            chunk_size = length;

        /* Append buffer */
        memcpy(audio_buffer->packet + audio_buffer->bytes_written,
                buffer, chunk_size);

        /* Update byte counters */
        length -= chunk_size;
        audio_buffer->bytes_written += chunk_size;

        /* Advance to next chunk */
        buffer += chunk_size;

        /* Invoke flush handler if full */
        if (audio_buffer->bytes_written == audio_buffer->packet_size) {

            /* Only actually invoke if defined */
            if (audio_buffer->flush_handler)
                audio_buffer->flush_handler(audio_buffer->packet,
                        audio_buffer->bytes_written, audio_buffer->data);

            /* Reset buffer in all cases */
            audio_buffer->bytes_written = 0;

        }

    } /* end packet write loop */

    pthread_mutex_unlock(&(audio_buffer->lock));

}

void guac_rdp_audio_buffer_end(guac_rdp_audio_buffer* audio_buffer) {

    pthread_mutex_lock(&(audio_buffer->lock));

    /* The stream is now closed */
    guac_rdp_audio_buffer_ack(audio_buffer,
            "CLOSED", GUAC_PROTOCOL_STATUS_RESOURCE_CLOSED);

    /* Unset user and stream */
    audio_buffer->user = NULL;
    audio_buffer->stream = NULL;

    /* Reset buffer state */
    audio_buffer->bytes_written = 0;
    audio_buffer->packet_size = 0;
    audio_buffer->flush_handler = NULL;

    /* Free packet (if any) */
    free(audio_buffer->packet);
    audio_buffer->packet = NULL;

    pthread_mutex_unlock(&(audio_buffer->lock));

}

void guac_rdp_audio_buffer_free(guac_rdp_audio_buffer* audio_buffer) {
    pthread_mutex_destroy(&(audio_buffer->lock));
    free(audio_buffer->packet);
    free(audio_buffer);
}

