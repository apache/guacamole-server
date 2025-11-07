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

#include "rdpecam.h"
#include "channels/rdpecam/rdpecam_sink.h"
#include "plugins/channels.h"
#include "plugins/ptr-string.h"
#include "rdp.h"

#include <freerdp/freerdp.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>
#include <guacamole/user.h>

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 * Per-stream reassembly state for RDPECAM frames. Handles fragmentation
 * across arbitrary Guacamole blob boundaries. One instance is attached to
 * the Guacamole stream via stream->data.
 */
typedef struct guac_rdp_rdpecam_stream_state {

    /* Header assembly */
    uint8_t header_buf[sizeof(guac_rdpecam_frame_header)];
    size_t header_received;

    /* Full-frame assembly (header + payload) */
    uint8_t* frame_buf;
    size_t frame_expected;   /* total bytes expected (header+payload) */
    size_t frame_received;   /* total bytes currently accumulated */

} guac_rdp_rdpecam_stream_state;

/**
 * Parses the given RDPECAM mimetype, validating that it's the expected format.
 *
 * @param mimetype
 *     The RDPECAM mimetype to parse.
 *
 * @return
 *     Zero if the given mimetype is valid, non-zero otherwise.
 */
static int guac_rdp_rdpecam_parse_mimetype(const char* mimetype) {

    /* Validate RDPECAM H.264 mimetype */
    if (strcmp(mimetype, "application/rdpecam+h264") == 0)
        return 0;

    return 1;

}

int guac_rdp_rdpecam_video_handler(guac_user* user, guac_stream* stream,
        char* mimetype) {

    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Parse mimetype, abort on parse error */
    if (guac_rdp_rdpecam_parse_mimetype(mimetype)) {
        guac_user_log(user, GUAC_LOG_WARNING, "Denying user camera stream with "
                "unsupported mimetype: \"%s\"", mimetype);
        guac_protocol_send_ack(user->socket, stream, "Unsupported camera "
                "mimetype", GUAC_PROTOCOL_STATUS_CLIENT_BAD_TYPE);
        return 0;
    }

    /* Init stream data */
    stream->blob_handler = guac_rdp_rdpecam_blob_handler;
    stream->end_handler = guac_rdp_rdpecam_end_handler;

    /* Initialize per-stream reassembly state */
    if (!stream->data) {
        guac_rdp_rdpecam_stream_state* st = guac_mem_zalloc(sizeof(guac_rdp_rdpecam_stream_state));
        stream->data = st;
    }

    /* Associate stream with RDPECAM sink */
    if (rdp_client->rdpecam_sink) {
        guac_user_log(user, GUAC_LOG_DEBUG, "User is requesting to provide camera "
                "input as H.264 video stream.");

        /* Send acknowledgment */
        guac_protocol_send_ack(user->socket, stream, "OK", GUAC_PROTOCOL_STATUS_SUCCESS);
        guac_socket_flush(user->socket);
    } else {
        guac_user_log(user, GUAC_LOG_WARNING, "RDPECAM sink not available");
        guac_protocol_send_ack(user->socket, stream, "RDPECAM not available",
                GUAC_PROTOCOL_STATUS_SERVER_ERROR);
    }

    return 0;

}

int guac_rdp_rdpecam_blob_handler(guac_user* user, guac_stream* stream,
        void* data, int length) {

    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* If sink not available, fail */
    if (!rdp_client->rdpecam_sink) {
        guac_protocol_send_ack(user->socket, stream, "RDPECAM not available",
                GUAC_PROTOCOL_STATUS_SERVER_ERROR);
        guac_socket_flush(user->socket);
        return 0;
    }

    /* Retrieve or initialize reassembly state */
    guac_rdp_rdpecam_stream_state* st = (guac_rdp_rdpecam_stream_state*) stream->data;
    if (!st) {
        st = guac_mem_zalloc(sizeof(guac_rdp_rdpecam_stream_state));
        stream->data = st;
    }

    const uint8_t* in = (const uint8_t*) data;
    size_t remaining = (length < 0) ? 0 : (size_t) length;


    /* Consume input, assembling one or more complete frames if present */
    while (remaining > 0) {

        /* Step 1: ensure header is complete */
        if (st->frame_expected == 0) {
            size_t need = sizeof(guac_rdpecam_frame_header) - st->header_received;
            size_t take = (remaining < need) ? remaining : need;
            if (take > 0) {
                memcpy(st->header_buf + st->header_received, in, take);
                st->header_received += take;
                in += take;
                remaining -= take;
            }

            if (st->header_received < sizeof(guac_rdpecam_frame_header)) {
                /* Need more data to finish header */
                break;
            }

            /* Header complete: validate and start frame assembly */
            const guac_rdpecam_frame_header* hdr = (const guac_rdpecam_frame_header*) st->header_buf;

            /* Basic sanity checks to avoid pathological allocations */
            if (hdr->version != 1 || hdr->payload_len > GUAC_RDPECAM_MAX_FRAME_SIZE) {
                guac_user_log(user, GUAC_LOG_WARNING,
                        "RDPECAM invalid header (version=%u, payload_len=%u) - discarding corrupted data (likely camera switch in progress)",
                        (unsigned) hdr->version, (unsigned) hdr->payload_len);
                /* Fast recovery: discard all accumulated data and wait for next clean frame.
                 * This typically happens when switching cameras - old camera's partial data
                 * arrives mixed with new camera's data. Discarding everything is faster
                 * than byte-by-byte scanning and reduces warning spam. */
                st->header_received = 0;
                remaining = 0;  /* Discard rest of this blob */
                break;
            }

            st->frame_expected = sizeof(guac_rdpecam_frame_header) + (size_t) hdr->payload_len;
            st->frame_buf = guac_mem_alloc(st->frame_expected);
            if (!st->frame_buf) {
                guac_user_log(user, GUAC_LOG_ERROR, "RDPECAM failed to allocate reassembly buffer: %zu bytes", st->frame_expected);
                /* Reset state and drop current partial data */
                st->header_received = 0;
                st->frame_expected = 0;
                st->frame_received = 0;
                break;
            }
            memcpy(st->frame_buf, st->header_buf, sizeof(guac_rdpecam_frame_header));
            st->frame_received = sizeof(guac_rdpecam_frame_header);
            /* Header buffer consumed for this frame */
            st->header_received = 0;
        }

        /* Step 2: append payload (and possibly following headers/payloads) */
        size_t to_copy = st->frame_expected - st->frame_received;
        size_t take = (remaining < to_copy) ? remaining : to_copy;
        if (take > 0) {
            memcpy(st->frame_buf + st->frame_received, in, take);
            st->frame_received += take;
            in += take;
            remaining -= take;
        }

        if (st->frame_received == st->frame_expected) {
            /* We have a full frame: push to sink (failures are tracked in periodic stats) */
            guac_rdpecam_push(rdp_client->rdpecam_sink, st->frame_buf, st->frame_expected);
            guac_mem_free(st->frame_buf);
            st->frame_buf = NULL;
            st->frame_expected = 0;
            st->frame_received = 0;
            /* Loop continues to see if additional frame data is present in this blob */
        } else {
            /* Partial frame */
            guac_user_log(user, GUAC_LOG_TRACE,
                    "RDPECAM partial frame: %zu/%zu bytes", st->frame_received, st->frame_expected);
        }
    }

    /* Always ACK success for accepted blob bytes to keep the stream flowing. */
    guac_protocol_send_ack(user->socket, stream, "OK", GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(user->socket);

    return 0;

}

int guac_rdp_rdpecam_end_handler(guac_user* user, guac_stream* stream) {

    /* Free any reassembly state */
    guac_rdp_rdpecam_stream_state* st = (guac_rdp_rdpecam_stream_state*) stream->data;
    if (st) {
        if (st->frame_buf) guac_mem_free(st->frame_buf);
        guac_mem_free(st);
        stream->data = NULL;
    }

    return 0;

}

void guac_rdp_rdpecam_load_plugin(rdpContext* context) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    char client_ref[GUAC_RDP_PTR_STRING_LENGTH];

    /* Add "guacrdpecam" plugin (loads libguacrdpecam-client.so) */
    guac_rdp_ptr_to_string(client, client_ref);
    guac_freerdp_dynamic_channel_collection_add(context->settings, "guacrdpecam", client_ref, NULL);

}
