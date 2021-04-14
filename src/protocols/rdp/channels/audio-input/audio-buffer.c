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

#include "channels/audio-input/audio-buffer.h"
#include "rdp.h"

#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>
#include <guacamole/user.h>

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

guac_rdp_audio_buffer* guac_rdp_audio_buffer_alloc(guac_client* client) {
    guac_rdp_audio_buffer* buffer = calloc(1, sizeof(guac_rdp_audio_buffer));
    pthread_mutex_init(&(buffer->lock), NULL);
    buffer->client = client;
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

/**
 * Reads a single sample from the given buffer of data, using the input
 * format defined within the given audio buffer. Each read sample is
 * translated to a signed 16-bit value, even if the input format is 8-bit.
 * The offset into the given buffer will be determined according to the
 * input and output formats, the number of bytes sent thus far, and the
 * number of bytes received (excluding the contents of the buffer).
 *
 * @param audio_buffer
 *     The audio buffer dictating the format of the given data buffer, as
 *     well as the offset from which the sample should be read.
 *
 * @param buffer
 *     The buffer of raw PCM audio data from which the sample should be read.
 *     This buffer MUST NOT contain data already taken into account by the
 *     audio buffer's total_bytes_received counter.
 *
 * @param length
 *     The number of bytes within the given buffer of PCM data.
 *
 * @param sample
 *     A pointer to the int16_t in which the read sample should be stored. If
 *     the input format is 8-bit, the sample will be shifted left by 8 bits
 *     to produce a 16-bit sample.
 *
 * @return
 *     Non-zero if a sample was successfully read, zero if no data remains
 *     within the given buffer that has not already been mapped to an
 *     output sample.
 */
static int guac_rdp_audio_buffer_read_sample(
        guac_rdp_audio_buffer* audio_buffer, const char* buffer, int length,
        int16_t* sample) {

    int in_bps = audio_buffer->in_format.bps;
    int in_rate = audio_buffer->in_format.rate;
    int in_channels = audio_buffer->in_format.channels;

    int out_bps = audio_buffer->out_format.bps;
    int out_rate = audio_buffer->out_format.rate;
    int out_channels = audio_buffer->out_format.channels;

    /* Calculate position within audio output */
    int current_sample  = audio_buffer->total_bytes_sent / out_bps;
    int current_frame   = current_sample / out_channels;
    int current_channel = current_sample % out_channels;

    /* Map output channel to input channel */
    if (current_channel >= in_channels)
        current_channel = in_channels - 1;

    /* Transform output position to input position */
    current_frame = (int) current_frame * ((double) in_rate / out_rate);
    current_sample = current_frame * in_channels + current_channel;

    /* Calculate offset within given buffer from absolute input position */
    int offset = current_sample * in_bps
               - audio_buffer->total_bytes_received;

    /* It should be impossible for the offset to ever go negative */
    assert(offset >= 0);

    /* Apply offset to buffer */
    buffer += offset;
    length -= offset;

    /* Read only if sufficient data is present in the given buffer */
    if (length < in_bps)
        return 0;

    /* Simply read sample directly if input is 16-bit */
    if (in_bps == 2) {
        *sample = *((int16_t*) buffer);
        return 1;
    }

    /* Translate to 16-bit if input is 8-bit */
    if (in_bps == 1) {
        *sample = *buffer << 8;
        return 1;
    }

    /* Accepted audio formats are required to be 8- or 16-bit */
    return 0;

}

void guac_rdp_audio_buffer_write(guac_rdp_audio_buffer* audio_buffer,
        char* buffer, int length) {

    int16_t sample;

    pthread_mutex_lock(&(audio_buffer->lock));

    /* Ignore packet if there is no buffer */
    if (audio_buffer->packet_size == 0 || audio_buffer->packet == NULL) {
        pthread_mutex_unlock(&(audio_buffer->lock));
        return;
    }

    int out_bps = audio_buffer->out_format.bps;

    /* Continuously write packets until no data remains */
    while (guac_rdp_audio_buffer_read_sample(audio_buffer,
                buffer, length, &sample) > 0) {

        char* current = audio_buffer->packet + audio_buffer->bytes_written;

        /* Store as 16-bit or 8-bit, depending on output format */
        if (out_bps == 2)
            *((int16_t*) current) = sample;
        else if (out_bps == 1)
            *current = sample >> 8;

        /* Accepted audio formats are required to be 8- or 16-bit */
        else
            assert(0);

        /* Update byte counters */
        audio_buffer->bytes_written += out_bps;
        audio_buffer->total_bytes_sent += out_bps;

        /* Invoke flush handler if full */
        if (audio_buffer->bytes_written == audio_buffer->packet_size) {

            /* Only actually invoke if defined */
            if (audio_buffer->flush_handler)
                audio_buffer->flush_handler(audio_buffer,
                        audio_buffer->bytes_written);

            /* Reset buffer in all cases */
            audio_buffer->bytes_written = 0;

        }

    } /* end packet write loop */

    /* Track current position in audio stream */
    audio_buffer->total_bytes_received += length;

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

    /* Reset I/O counters */
    audio_buffer->total_bytes_sent = 0;
    audio_buffer->total_bytes_received = 0;

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

