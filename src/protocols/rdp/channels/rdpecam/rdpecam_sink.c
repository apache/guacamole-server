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

#include "rdpecam_sink.h"

#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>
#include <guacamole/user.h>

#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

guac_rdpecam_sink* guac_rdpecam_create(guac_client* client) {

    guac_rdpecam_sink* sink = guac_mem_zalloc(sizeof(guac_rdpecam_sink));

    if (!sink) {
        guac_client_log(client, GUAC_LOG_ERROR, "Failed to allocate RDPECAM sink");
        return NULL;
    }

    if (pthread_mutex_init(&sink->lock, NULL) != 0) {
        guac_client_log(client, GUAC_LOG_ERROR, "Failed to initialize RDPECAM sink mutex");
        guac_mem_free(sink);
        return NULL;
    }

    if (pthread_cond_init(&sink->frame_available, NULL) != 0) {
        guac_client_log(client, GUAC_LOG_ERROR, "Failed to initialize RDPECAM sink condition variable");
        pthread_mutex_destroy(&sink->lock);
        guac_mem_free(sink);
        return NULL;
    }

    sink->client = client;
    sink->queue_head = NULL;
    sink->queue_tail = NULL;
    sink->queue_size = 0;
    sink->stopping = false;
    sink->streaming = false;
    sink->credits = 0;
    sink->stream_index = 0;
    sink->has_active_sender = false;
    sink->active_sender_channel = NULL;

    guac_client_log(client, GUAC_LOG_DEBUG, "RDPECAM sink created");

    return sink;

}

void guac_rdpecam_destroy(guac_rdpecam_sink* sink) {

    if (!sink)
        return;
    pthread_mutex_lock(&sink->lock);

    sink->stopping = true;

    /* Drain any queued frames before releasing the sink. */
    guac_rdpecam_frame* frame = sink->queue_head;
    while (frame) {
        guac_rdpecam_frame* next = frame->next;
        guac_mem_free(frame->data);
        guac_mem_free(frame);
        frame = next;
    }

    sink->queue_head = NULL;
    sink->queue_tail = NULL;
    sink->queue_size = 0;

    pthread_cond_broadcast(&sink->frame_available);

    pthread_mutex_unlock(&sink->lock);

    pthread_cond_destroy(&sink->frame_available);
    pthread_mutex_destroy(&sink->lock);

    guac_mem_free(sink);

}


void guac_rdpecam_signal_stop(guac_rdpecam_sink* sink) {

    if (!sink)
        return;

    pthread_mutex_lock(&sink->lock);
    if (!sink->stopping)
        sink->stopping = true;

    pthread_cond_broadcast(&sink->frame_available);
    pthread_mutex_unlock(&sink->lock);
}


bool guac_rdpecam_push(guac_rdpecam_sink* sink, const void* data, size_t len) {

    guac_client* client = sink ? sink->client : NULL;

    if (!sink || !data || len == 0) {
        if (client)
            guac_client_log(client, GUAC_LOG_WARNING, "RDPECAM push called with invalid parameters: sink=%p, data=%p, len=%zu", sink, data, len);
        return false;
    }

    pthread_mutex_lock(&sink->lock);

    /* Reject new frames once destruction has begun. */
    if (sink->stopping) {
        guac_client_log(sink->client, GUAC_LOG_DEBUG, "RDPECAM sink is stopping, rejecting frame");
        pthread_mutex_unlock(&sink->lock);
        return false;
    }

    /* Prevent unbounded growth when the consumer is back-pressured. */
    if (sink->queue_size >= GUAC_RDPECAM_MAX_FRAMES) {
        pthread_mutex_unlock(&sink->lock);
        return false;
    }

    if (len < sizeof(guac_rdpecam_frame_header)) {
        guac_client_log(sink->client, GUAC_LOG_WARNING, "RDPECAM frame too small: %zu bytes (expected at least %zu)", 
                       len, sizeof(guac_rdpecam_frame_header));
        pthread_mutex_unlock(&sink->lock);
        return false;
    }

    const guac_rdpecam_frame_header* header = (const guac_rdpecam_frame_header*) data;
    
    if (header->version != 1) {
        guac_client_log(sink->client, GUAC_LOG_WARNING, "RDPECAM frame has invalid version: %d", header->version);
        pthread_mutex_unlock(&sink->lock);
        return false;
    }

    if (header->payload_len > GUAC_RDPECAM_MAX_FRAME_SIZE) {
        guac_client_log(sink->client, GUAC_LOG_WARNING, "RDPECAM frame payload too large: %u bytes", header->payload_len);
        pthread_mutex_unlock(&sink->lock);
        return false;
    }

    size_t expected_total_len = sizeof(guac_rdpecam_frame_header) + header->payload_len;
    if (len != expected_total_len) {
        guac_client_log(sink->client, GUAC_LOG_WARNING, "RDPECAM frame length mismatch: got %zu bytes, expected %zu (header: %zu + payload: %u)", 
                       len, expected_total_len, sizeof(guac_rdpecam_frame_header), header->payload_len);
        pthread_mutex_unlock(&sink->lock);
        return false;
    }

    guac_rdpecam_frame* frame = guac_mem_zalloc(sizeof(guac_rdpecam_frame));
    if (!frame) {
        guac_client_log(sink->client, GUAC_LOG_ERROR, "Failed to allocate RDPECAM frame");
        pthread_mutex_unlock(&sink->lock);
        return false;
    }

    frame->data = guac_mem_alloc(header->payload_len);
    if (!frame->data) {
        guac_client_log(sink->client, GUAC_LOG_ERROR, "Failed to allocate RDPECAM frame data");
        guac_mem_free(frame);
        pthread_mutex_unlock(&sink->lock);
        return false;
    }

    const uint8_t* payload_start = (const uint8_t*) data + sizeof(guac_rdpecam_frame_header);
    memcpy(frame->data, payload_start, header->payload_len);
    frame->length = header->payload_len;
    frame->pts_ms = header->pts_ms;
    frame->keyframe = (header->flags & 0x01) != 0;
    frame->next = NULL;
    
    if (sink->queue_tail) {
        sink->queue_tail->next = frame;
        sink->queue_tail = frame;
    } else {
        sink->queue_head = frame;
        sink->queue_tail = frame;
    }
    sink->queue_size++;

    guac_client_log(sink->client, GUAC_LOG_TRACE, "RDPECAM frame queued: %zu bytes, keyframe=%s, pts=%u ms, queue_size=%d/%d",
                  frame->length, frame->keyframe ? "yes" : "no", frame->pts_ms, sink->queue_size, GUAC_RDPECAM_MAX_FRAMES);

    int utilization = (sink->queue_size * 100) / GUAC_RDPECAM_MAX_FRAMES;
    if (utilization >= 80) {
        guac_client_log(sink->client, GUAC_LOG_DEBUG, "RDPECAM queue utilization: %d%% (%d/%d frames)", 
                       utilization, sink->queue_size, GUAC_RDPECAM_MAX_FRAMES);
    }

    pthread_cond_signal(&sink->frame_available);

    pthread_mutex_unlock(&sink->lock);

    return true;

}

bool guac_rdpecam_pop(guac_rdpecam_sink* sink, uint8_t** out_buf, size_t* out_len,
                      bool* out_keyframe, uint32_t* out_pts_ms) {

    if (!sink || !out_buf || !out_len || !out_keyframe || !out_pts_ms)
        return false;

    pthread_mutex_lock(&sink->lock);

    /* Sleep until a frame arrives or destruction is requested. */
    while (sink->queue_size == 0 && !sink->stopping) {
        pthread_cond_wait(&sink->frame_available, &sink->lock);
    }

    if (sink->stopping || sink->queue_size == 0) {
        pthread_mutex_unlock(&sink->lock);
        return false;
    }

    guac_rdpecam_frame* frame = sink->queue_head;
    sink->queue_head = frame->next;
    if (!sink->queue_head) {
        sink->queue_tail = NULL;
    }
    sink->queue_size--;

    *out_buf = frame->data;
    *out_len = frame->length;
    *out_keyframe = frame->keyframe;
    *out_pts_ms = frame->pts_ms;

    guac_client_log(sink->client, GUAC_LOG_TRACE, "RDPECAM frame popped: %zu bytes, keyframe=%s, pts=%u ms, queue_size=%d/%d",
                    frame->length, frame->keyframe ? "yes" : "no", frame->pts_ms, sink->queue_size, GUAC_RDPECAM_MAX_FRAMES);

    if (sink->queue_size == 0) {
        guac_client_log(sink->client, GUAC_LOG_DEBUG, "RDPECAM queue is now empty");
    } else if (sink->queue_size <= 3) {
        guac_client_log(sink->client, GUAC_LOG_DEBUG, "RDPECAM queue is low: %d/%d frames remaining", sink->queue_size, GUAC_RDPECAM_MAX_FRAMES);
    }

    guac_mem_free(frame);

    pthread_mutex_unlock(&sink->lock);

    return true;

}

int guac_rdpecam_get_queue_size(guac_rdpecam_sink* sink) {

    if (!sink)
        return 0;

    pthread_mutex_lock(&sink->lock);
    int size = sink->queue_size;
    pthread_mutex_unlock(&sink->lock);

    return size;

}
