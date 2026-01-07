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

#include "channels/rdpecam/rdpecam_sink.h"

#include <guacamole/client.h>
#include <guacamole/mem.h>

#include <CUnit/CUnit.h>
#include <stdlib.h>
#include <string.h>

/**
 * Creates a minimal mock guac_client for testing.
 */
static guac_client* create_mock_client(void) {
    guac_client* client = guac_mem_zalloc(sizeof(guac_client));
    if (client) {
        client->log_level = GUAC_LOG_DEBUG;
    }
    return client;
}

/**
 * Frees a mock guac_client created by create_mock_client().
 */
static void free_mock_client(guac_client* client) {
    if (client)
        guac_mem_free(client);
}

/**
 * Creates a valid RDPECAM frame header with the given parameters.
 */
static void create_frame_header(guac_rdpecam_frame_header* header,
        uint32_t payload_len, uint32_t pts_ms, bool keyframe) {
    header->version = 1;
    header->flags = keyframe ? 0x01 : 0x00;
    header->reserved = 0;
    header->pts_ms = pts_ms;
    header->payload_len = payload_len;
}

/**
 * Test which verifies that a sink can be created and destroyed.
 */
void test_rdpecam_sink__create_destroy(void) {
    guac_client* client = create_mock_client();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_rdpecam_sink* sink = guac_rdpecam_create(client);
    CU_ASSERT_PTR_NOT_NULL(sink);
    CU_ASSERT_EQUAL(guac_rdpecam_get_queue_size(sink), 0);

    guac_rdpecam_destroy(sink);
    free_mock_client(client);
}

/**
 * Test which verifies that creating a sink with NULL client returns NULL.
 */
void test_rdpecam_sink__create_null_client(void) {
    guac_rdpecam_sink* sink = guac_rdpecam_create(NULL);
    CU_ASSERT_PTR_NULL(sink);
}

/**
 * Test which verifies that destroying a NULL sink is safe.
 */
void test_rdpecam_sink__destroy_null(void) {
    guac_rdpecam_destroy(NULL);
}

/**
 * Test which verifies that pushing a valid frame succeeds.
 */
void test_rdpecam_sink__push_valid_frame(void) {
    guac_client* client = create_mock_client();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_rdpecam_sink* sink = guac_rdpecam_create(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(sink);

    uint8_t payload[100] = {0};
    guac_rdpecam_frame_header header;
    create_frame_header(&header, sizeof(payload), 1000, false);

    uint8_t frame_data[sizeof(header) + sizeof(payload)];
    memcpy(frame_data, &header, sizeof(header));
    memcpy(frame_data + sizeof(header), payload, sizeof(payload));

    bool result = guac_rdpecam_push(sink, frame_data, sizeof(frame_data));
    CU_ASSERT_TRUE(result);
    CU_ASSERT_EQUAL(guac_rdpecam_get_queue_size(sink), 1);

    guac_rdpecam_destroy(sink);
    free_mock_client(client);
}

/**
 * Test which verifies that pushing a keyframe succeeds.
 */
void test_rdpecam_sink__push_keyframe(void) {
    guac_client* client = create_mock_client();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_rdpecam_sink* sink = guac_rdpecam_create(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(sink);

    uint8_t payload[200] = {0};
    guac_rdpecam_frame_header header;
    create_frame_header(&header, sizeof(payload), 2000, true);

    uint8_t frame_data[sizeof(header) + sizeof(payload)];
    memcpy(frame_data, &header, sizeof(header));
    memcpy(frame_data + sizeof(header), payload, sizeof(payload));

    bool result = guac_rdpecam_push(sink, frame_data, sizeof(frame_data));
    CU_ASSERT_TRUE(result);
    CU_ASSERT_EQUAL(guac_rdpecam_get_queue_size(sink), 1);

    guac_rdpecam_destroy(sink);
    free_mock_client(client);
}

/**
 * Test which verifies that pushing with NULL sink fails.
 */
void test_rdpecam_sink__push_null_sink(void) {
    uint8_t data[100] = {0};
    bool result = guac_rdpecam_push(NULL, data, sizeof(data));
    CU_ASSERT_FALSE(result);
}

/**
 * Test which verifies that pushing with NULL data fails.
 */
void test_rdpecam_sink__push_null_data(void) {
    guac_client* client = create_mock_client();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_rdpecam_sink* sink = guac_rdpecam_create(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(sink);

    bool result = guac_rdpecam_push(sink, NULL, 100);
    CU_ASSERT_FALSE(result);

    guac_rdpecam_destroy(sink);
    free_mock_client(client);
}

/**
 * Test which verifies that pushing with zero length fails.
 */
void test_rdpecam_sink__push_zero_length(void) {
    guac_client* client = create_mock_client();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_rdpecam_sink* sink = guac_rdpecam_create(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(sink);

    uint8_t data[100] = {0};
    bool result = guac_rdpecam_push(sink, data, 0);
    CU_ASSERT_FALSE(result);

    guac_rdpecam_destroy(sink);
    free_mock_client(client);
}

/**
 * Test which verifies that pushing a frame that's too small fails.
 */
void test_rdpecam_sink__push_too_small(void) {
    guac_client* client = create_mock_client();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_rdpecam_sink* sink = guac_rdpecam_create(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(sink);

    uint8_t data[1] = {0};
    bool result = guac_rdpecam_push(sink, data, sizeof(data));
    CU_ASSERT_FALSE(result);

    guac_rdpecam_destroy(sink);
    free_mock_client(client);
}

/**
 * Test which verifies that pushing a frame with invalid version fails.
 */
void test_rdpecam_sink__push_invalid_version(void) {
    guac_client* client = create_mock_client();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_rdpecam_sink* sink = guac_rdpecam_create(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(sink);

    uint8_t payload[100] = {0};
    guac_rdpecam_frame_header header;
    create_frame_header(&header, sizeof(payload), 1000, false);
    header.version = 2; /* Invalid version */

    uint8_t frame_data[sizeof(header) + sizeof(payload)];
    memcpy(frame_data, &header, sizeof(header));
    memcpy(frame_data + sizeof(header), payload, sizeof(payload));

    bool result = guac_rdpecam_push(sink, frame_data, sizeof(frame_data));
    CU_ASSERT_FALSE(result);

    guac_rdpecam_destroy(sink);
    free_mock_client(client);
}

/**
 * Test which verifies that pushing a frame with payload too large fails.
 */
void test_rdpecam_sink__push_payload_too_large(void) {
    guac_client* client = create_mock_client();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_rdpecam_sink* sink = guac_rdpecam_create(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(sink);

    uint32_t large_payload_len = GUAC_RDPECAM_MAX_FRAME_SIZE + 1;
    guac_rdpecam_frame_header header;
    create_frame_header(&header, large_payload_len, 1000, false);

    uint8_t frame_data[sizeof(header) + 100];
    memcpy(frame_data, &header, sizeof(header));

    bool result = guac_rdpecam_push(sink, frame_data, sizeof(frame_data));
    CU_ASSERT_FALSE(result);

    guac_rdpecam_destroy(sink);
    free_mock_client(client);
}

/**
 * Test which verifies that pushing frames up to the maximum queue size succeeds.
 */
void test_rdpecam_sink__push_max_frames(void) {
    guac_client* client = create_mock_client();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_rdpecam_sink* sink = guac_rdpecam_create(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(sink);

    uint8_t payload[100] = {0};
    guac_rdpecam_frame_header header;
    create_frame_header(&header, sizeof(payload), 1000, false);

    uint8_t frame_data[sizeof(header) + sizeof(payload)];
    memcpy(frame_data, &header, sizeof(header));
    memcpy(frame_data + sizeof(header), payload, sizeof(payload));

    /* Push up to maximum */
    for (int i = 0; i < GUAC_RDPECAM_MAX_FRAMES; i++) {
        header.pts_ms = 1000 + i;
        memcpy(frame_data, &header, sizeof(header));
        bool result = guac_rdpecam_push(sink, frame_data, sizeof(frame_data));
        CU_ASSERT_TRUE(result);
    }

    CU_ASSERT_EQUAL(guac_rdpecam_get_queue_size(sink), GUAC_RDPECAM_MAX_FRAMES);

    /* Next push should fail */
    bool result = guac_rdpecam_push(sink, frame_data, sizeof(frame_data));
    CU_ASSERT_FALSE(result);
    CU_ASSERT_EQUAL(guac_rdpecam_get_queue_size(sink), GUAC_RDPECAM_MAX_FRAMES);

    guac_rdpecam_destroy(sink);
    free_mock_client(client);
}

/**
 * Test which verifies that popping from an empty sink fails.
 */
void test_rdpecam_sink__pop_empty(void) {
    guac_client* client = create_mock_client();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_rdpecam_sink* sink = guac_rdpecam_create(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(sink);

    uint8_t* out_buf = NULL;
    size_t out_len = 0;
    bool out_keyframe = false;
    uint32_t out_pts_ms = 0;

    bool result = guac_rdpecam_pop(sink, &out_buf, &out_len, &out_keyframe, &out_pts_ms);
    CU_ASSERT_FALSE(result);

    guac_rdpecam_destroy(sink);
    free_mock_client(client);
}

/**
 * Test which verifies that popping with NULL parameters fails.
 */
void test_rdpecam_sink__pop_null_params(void) {
    guac_client* client = create_mock_client();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_rdpecam_sink* sink = guac_rdpecam_create(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(sink);

    bool result = guac_rdpecam_pop(sink, NULL, NULL, NULL, NULL);
    CU_ASSERT_FALSE(result);

    guac_rdpecam_destroy(sink);
    free_mock_client(client);
}

/**
 * Test which verifies that push and pop operations work correctly.
 */
void test_rdpecam_sink__push_pop(void) {
    guac_client* client = create_mock_client();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_rdpecam_sink* sink = guac_rdpecam_create(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(sink);

    uint8_t payload[100];
    memset(payload, 0xAA, sizeof(payload));
    guac_rdpecam_frame_header header;
    create_frame_header(&header, sizeof(payload), 5000, true);

    uint8_t frame_data[sizeof(header) + sizeof(payload)];
    memcpy(frame_data, &header, sizeof(header));
    memcpy(frame_data + sizeof(header), payload, sizeof(payload));

    /* Push frame */
    bool push_result = guac_rdpecam_push(sink, frame_data, sizeof(frame_data));
    CU_ASSERT_TRUE(push_result);
    CU_ASSERT_EQUAL(guac_rdpecam_get_queue_size(sink), 1);

    /* Pop frame */
    uint8_t* out_buf = NULL;
    size_t out_len = 0;
    bool out_keyframe = false;
    uint32_t out_pts_ms = 0;

    bool pop_result = guac_rdpecam_pop(sink, &out_buf, &out_len, &out_keyframe, &out_pts_ms);
    CU_ASSERT_TRUE(pop_result);
    CU_ASSERT_PTR_NOT_NULL(out_buf);
    CU_ASSERT_EQUAL(out_len, sizeof(payload));
    CU_ASSERT_TRUE(out_keyframe);
    CU_ASSERT_EQUAL(out_pts_ms, 5000);
    CU_ASSERT_EQUAL(guac_rdpecam_get_queue_size(sink), 0);

    /* Verify payload content */
    CU_ASSERT_EQUAL(memcmp(out_buf, payload, sizeof(payload)), 0);

    guac_mem_free(out_buf);
    guac_rdpecam_destroy(sink);
    free_mock_client(client);
}

/**
 * Test which verifies that multiple frames can be pushed and popped in order.
 */
void test_rdpecam_sink__push_pop_multiple(void) {
    guac_client* client = create_mock_client();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_rdpecam_sink* sink = guac_rdpecam_create(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(sink);

    uint8_t payload[50] = {0};
    guac_rdpecam_frame_header header;
    create_frame_header(&header, sizeof(payload), 0, false);

    uint8_t frame_data[sizeof(header) + sizeof(payload)];
    memcpy(frame_data, &header, sizeof(header));
    memcpy(frame_data + sizeof(header), payload, sizeof(payload));

    /* Push multiple frames */
    for (int i = 0; i < 5; i++) {
        header.pts_ms = 1000 * i;
        header.flags = (i == 0) ? 0x01 : 0x00; /* First frame is keyframe */
        memcpy(frame_data, &header, sizeof(header));
        memset(payload, (uint8_t)i, sizeof(payload));
        memcpy(frame_data + sizeof(header), payload, sizeof(payload));

        bool result = guac_rdpecam_push(sink, frame_data, sizeof(frame_data));
        CU_ASSERT_TRUE(result);
    }

    CU_ASSERT_EQUAL(guac_rdpecam_get_queue_size(sink), 5);

    /* Pop frames and verify order */
    for (int i = 0; i < 5; i++) {
        uint8_t* out_buf = NULL;
        size_t out_len = 0;
        bool out_keyframe = false;
        uint32_t out_pts_ms = 0;

        bool result = guac_rdpecam_pop(sink, &out_buf, &out_len, &out_keyframe, &out_pts_ms);
        CU_ASSERT_TRUE(result);
        CU_ASSERT_EQUAL(out_pts_ms, (uint32_t)(1000 * i));
        CU_ASSERT_EQUAL(out_keyframe, (i == 0));
        CU_ASSERT_EQUAL(out_len, sizeof(payload));

        guac_mem_free(out_buf);
    }

    CU_ASSERT_EQUAL(guac_rdpecam_get_queue_size(sink), 0);

    guac_rdpecam_destroy(sink);
    free_mock_client(client);
}

/**
 * Test which verifies that signal_stop wakes up waiting threads.
 */
void test_rdpecam_sink__signal_stop(void) {
    guac_client* client = create_mock_client();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_rdpecam_sink* sink = guac_rdpecam_create(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(sink);

    guac_rdpecam_signal_stop(sink);

    /* After signal_stop, pop should fail */
    uint8_t* out_buf = NULL;
    size_t out_len = 0;
    bool out_keyframe = false;
    uint32_t out_pts_ms = 0;

    bool result = guac_rdpecam_pop(sink, &out_buf, &out_len, &out_keyframe, &out_pts_ms);
    CU_ASSERT_FALSE(result);

    guac_rdpecam_destroy(sink);
    free_mock_client(client);
}

/**
 * Test which verifies that pushing to a stopped sink fails.
 */
void test_rdpecam_sink__push_after_stop(void) {
    guac_client* client = create_mock_client();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_rdpecam_sink* sink = guac_rdpecam_create(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(sink);

    guac_rdpecam_signal_stop(sink);

    uint8_t payload[100] = {0};
    guac_rdpecam_frame_header header;
    create_frame_header(&header, sizeof(payload), 1000, false);

    uint8_t frame_data[sizeof(header) + sizeof(payload)];
    memcpy(frame_data, &header, sizeof(header));
    memcpy(frame_data + sizeof(header), payload, sizeof(payload));

    bool result = guac_rdpecam_push(sink, frame_data, sizeof(frame_data));
    CU_ASSERT_FALSE(result);

    guac_rdpecam_destroy(sink);
    free_mock_client(client);
}

/**
 * Test which verifies that get_queue_size returns correct values.
 */
void test_rdpecam_sink__get_queue_size(void) {
    guac_client* client = create_mock_client();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_rdpecam_sink* sink = guac_rdpecam_create(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(sink);

    CU_ASSERT_EQUAL(guac_rdpecam_get_queue_size(sink), 0);

    uint8_t payload[100] = {0};
    guac_rdpecam_frame_header header;
    create_frame_header(&header, sizeof(payload), 1000, false);

    uint8_t frame_data[sizeof(header) + sizeof(payload)];
    memcpy(frame_data, &header, sizeof(header));
    memcpy(frame_data + sizeof(header), payload, sizeof(payload));

    for (int i = 1; i <= 3; i++) {
        bool result = guac_rdpecam_push(sink, frame_data, sizeof(frame_data));
        CU_ASSERT_TRUE(result);
        CU_ASSERT_EQUAL(guac_rdpecam_get_queue_size(sink), i);
    }

    guac_rdpecam_destroy(sink);
    free_mock_client(client);
}

/**
 * Test which verifies that get_queue_size with NULL sink returns 0.
 */
void test_rdpecam_sink__get_queue_size_null(void) {
    int size = guac_rdpecam_get_queue_size(NULL);
    CU_ASSERT_EQUAL(size, 0);
}


