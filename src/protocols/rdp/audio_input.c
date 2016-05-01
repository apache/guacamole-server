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
#include "ptr_string.h"
#include "rdp.h"

#include <freerdp/freerdp.h>
#include <freerdp/channels/channels.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>
#include <guacamole/user.h>

#include <stdlib.h>

int guac_rdp_audio_handler(guac_user* user, guac_stream* stream,
        char* mimetype) {

    /* FIXME: Assuming mimetype of "audio/L16;rate=44100,channels=2" */

    /* Init stream data */
    stream->blob_handler = guac_rdp_audio_blob_handler;
    stream->end_handler = guac_rdp_audio_end_handler;

    guac_protocol_send_ack(user->socket, stream,
            "OK", GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(user->socket);

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

void guac_rdp_audio_load_plugin(rdpContext* context) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;

    /* Add "AUDIO_INPUT" channel */
    ADDIN_ARGV* args = malloc(sizeof(ADDIN_ARGV));
    args->argc = 2;
    args->argv = malloc(sizeof(char**) * 2);
    args->argv[0] = strdup("guacai");
    args->argv[1] = guac_rdp_ptr_to_string(client);
    freerdp_dynamic_channel_collection_add(context->settings, args);

}

guac_rdp_audio_buffer* guac_rdp_audio_buffer_alloc() {
    return calloc(1, sizeof(guac_rdp_audio_buffer));
}

void guac_rdp_audio_buffer_begin(guac_rdp_audio_buffer* audio_buffer,
        int packet_size, guac_rdp_audio_buffer_flush_handler* flush_handler,
        void* data) {

    /* Reset buffer state to provided values */
    audio_buffer->bytes_written = 0;
    audio_buffer->packet_size = packet_size;
    audio_buffer->flush_handler = flush_handler;
    audio_buffer->data = data;

    /* Allocate new buffer */
    free(audio_buffer->packet);
    audio_buffer->packet = malloc(packet_size);

}

void guac_rdp_audio_buffer_write(guac_rdp_audio_buffer* audio_buffer,
        char* buffer, int length) {

    /* Ignore packet if there is no buffer */
    if (audio_buffer->packet_size == 0 || audio_buffer->packet == NULL)
        return;

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

}

void guac_rdp_audio_buffer_end(guac_rdp_audio_buffer* audio_buffer) {

    /* Reset buffer state */
    audio_buffer->bytes_written = 0;
    audio_buffer->packet_size = 0;
    audio_buffer->flush_handler = NULL;

    /* Free packet (if any) */
    free(audio_buffer->packet);
    audio_buffer->packet = NULL;

}

void guac_rdp_audio_buffer_free(guac_rdp_audio_buffer* audio_buffer) {
    free(audio_buffer->packet);
    free(audio_buffer);
}

