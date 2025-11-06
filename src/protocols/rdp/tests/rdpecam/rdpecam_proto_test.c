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

#include "plugins/guacrdpecam/rdpecam_proto.h"

#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <winpr/stream.h>

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
 * Test which verifies that build_version_request creates a valid message.
 */
void test_rdpecam_proto__build_version_request(void) {
    wStream* s = Stream_New(NULL, 1024);
    CU_ASSERT_PTR_NOT_NULL_FATAL(s);

    BOOL result = rdpecam_build_version_request(s);
    CU_ASSERT_TRUE(result);

    Stream_Seek(s, 0);
    uint8_t version = Stream_Read_UINT8(s);
    uint8_t msg_id = Stream_Read_UINT8(s);

    CU_ASSERT_EQUAL(version, RDPECAM_PROTO_VERSION);
    CU_ASSERT_EQUAL(msg_id, RDPECAM_MSG_SELECT_VERSION_REQUEST);

    Stream_Free(s, TRUE);
}

/**
 * Test which verifies that build_version_request with NULL stream fails.
 */
void test_rdpecam_proto__build_version_request_null(void) {
    BOOL result = rdpecam_build_version_request(NULL);
    CU_ASSERT_FALSE(result);
}

/**
 * Test which verifies that build_version_response creates a valid message.
 */
void test_rdpecam_proto__build_version_response(void) {
    wStream* s = Stream_New(NULL, 1024);
    CU_ASSERT_PTR_NOT_NULL_FATAL(s);

    BOOL result = rdpecam_build_version_response(s);
    CU_ASSERT_TRUE(result);

    Stream_Seek(s, 0);
    uint8_t version = Stream_Read_UINT8(s);
    uint8_t msg_id = Stream_Read_UINT8(s);

    CU_ASSERT_EQUAL(version, RDPECAM_PROTO_VERSION);
    CU_ASSERT_EQUAL(msg_id, RDPECAM_MSG_SELECT_VERSION_RESPONSE);

    Stream_Free(s, TRUE);
}

/**
 * Test which verifies that build_success_response creates a valid message.
 */
void test_rdpecam_proto__build_success_response(void) {
    wStream* s = Stream_New(NULL, 1024);
    CU_ASSERT_PTR_NOT_NULL_FATAL(s);

    BOOL result = rdpecam_build_success_response(s);
    CU_ASSERT_TRUE(result);

    Stream_Seek(s, 0);
    uint8_t version = Stream_Read_UINT8(s);
    uint8_t msg_id = Stream_Read_UINT8(s);

    CU_ASSERT_EQUAL(version, RDPECAM_PROTO_VERSION);
    CU_ASSERT_EQUAL(msg_id, RDPECAM_MSG_SUCCESS_RESPONSE);

    Stream_Free(s, TRUE);
}

/**
 * Test which verifies that build_device_added creates a valid message.
 */
void test_rdpecam_proto__build_device_added(void) {
    wStream* s = Stream_New(NULL, 1024);
    CU_ASSERT_PTR_NOT_NULL_FATAL(s);

    const char* device_name = "Test Camera";
    const char* channel_name = "CAMERA#0";

    BOOL result = rdpecam_build_device_added(s, device_name, channel_name);
    CU_ASSERT_TRUE(result);

    Stream_Seek(s, 0);
    uint8_t version = Stream_Read_UINT8(s);
    uint8_t msg_id = Stream_Read_UINT8(s);
    CU_ASSERT_EQUAL(version, RDPECAM_PROTO_VERSION);
    CU_ASSERT_EQUAL(msg_id, RDPECAM_MSG_DEVICE_ADDED_NOTIFICATION);

    /* Verify device name (UTF-16LE) */
    size_t name_len = strlen(device_name);
    for (size_t i = 0; i < name_len; i++) {
        uint16_t ch = Stream_Read_UINT16(s);
        CU_ASSERT_EQUAL(ch, (uint16_t)(unsigned char)device_name[i]);
    }
    uint16_t nul = Stream_Read_UINT16(s);
    CU_ASSERT_EQUAL(nul, 0);

    /* Verify channel name (ASCII) */
    char read_channel[256];
    Stream_Read(s, read_channel, strlen(channel_name) + 1);
    CU_ASSERT_NSTRING_EQUAL(read_channel, channel_name, strlen(channel_name) + 1);

    Stream_Free(s, TRUE);
}

/**
 * Test which verifies that build_device_added with NULL parameters fails.
 */
void test_rdpecam_proto__build_device_added_null(void) {
    wStream* s = Stream_New(NULL, 1024);
    CU_ASSERT_PTR_NOT_NULL_FATAL(s);

    BOOL result = rdpecam_build_device_added(NULL, "device", "channel");
    CU_ASSERT_FALSE(result);

    result = rdpecam_build_device_added(s, NULL, "channel");
    CU_ASSERT_FALSE(result);

    result = rdpecam_build_device_added(s, "device", NULL);
    CU_ASSERT_FALSE(result);

    Stream_Free(s, TRUE);
}

/**
 * Test which verifies that build_device_removed creates a valid message.
 */
void test_rdpecam_proto__build_device_removed(void) {
    wStream* s = Stream_New(NULL, 1024);
    CU_ASSERT_PTR_NOT_NULL_FATAL(s);

    const char* channel_name = "CAMERA#0";

    BOOL result = rdpecam_build_device_removed(s, channel_name);
    CU_ASSERT_TRUE(result);

    Stream_Seek(s, 0);
    uint8_t version = Stream_Read_UINT8(s);
    uint8_t msg_id = Stream_Read_UINT8(s);
    CU_ASSERT_EQUAL(version, RDPECAM_PROTO_VERSION);
    CU_ASSERT_EQUAL(msg_id, RDPECAM_MSG_DEVICE_REMOVED_NOTIFICATION);

    char read_channel[256];
    Stream_Read(s, read_channel, strlen(channel_name) + 1);
    CU_ASSERT_NSTRING_EQUAL(read_channel, channel_name, strlen(channel_name) + 1);

    Stream_Free(s, TRUE);
}

/**
 * Test which verifies that build_device_removed with NULL parameters fails.
 */
void test_rdpecam_proto__build_device_removed_null(void) {
    wStream* s = Stream_New(NULL, 1024);
    CU_ASSERT_PTR_NOT_NULL_FATAL(s);

    BOOL result = rdpecam_build_device_removed(NULL, "channel");
    CU_ASSERT_FALSE(result);

    result = rdpecam_build_device_removed(s, NULL);
    CU_ASSERT_FALSE(result);

    Stream_Free(s, TRUE);
}

/**
 * Test which verifies that build_stream_list creates a valid message.
 */
void test_rdpecam_proto__build_stream_list(void) {
    wStream* s = Stream_New(NULL, 1024);
    CU_ASSERT_PTR_NOT_NULL_FATAL(s);

    rdpecam_stream_desc streams[2] = {
        { CAM_STREAM_FRAME_SOURCE_TYPE_Color, CAM_STREAM_CATEGORY_Capture, 1, 0 },
        { CAM_STREAM_FRAME_SOURCE_TYPE_Color, CAM_STREAM_CATEGORY_Capture, 0, 1 }
    };

    BOOL result = rdpecam_build_stream_list(s, streams, 2);
    CU_ASSERT_TRUE(result);

    Stream_Seek(s, 0);
    uint8_t version = Stream_Read_UINT8(s);
    uint8_t msg_id = Stream_Read_UINT8(s);
    CU_ASSERT_EQUAL(version, RDPECAM_PROTO_VERSION);
    CU_ASSERT_EQUAL(msg_id, RDPECAM_MSG_STREAM_LIST_RESPONSE);

    for (int i = 0; i < 2; i++) {
        uint16_t frame_source = Stream_Read_UINT16(s);
        uint8_t category = Stream_Read_UINT8(s);
        uint8_t selected = Stream_Read_UINT8(s);
        uint8_t can_be_shared = Stream_Read_UINT8(s);

        CU_ASSERT_EQUAL(frame_source, streams[i].FrameSourceType);
        CU_ASSERT_EQUAL(category, streams[i].Category);
        CU_ASSERT_EQUAL(selected, streams[i].Selected);
        CU_ASSERT_EQUAL(can_be_shared, streams[i].CanBeShared);
    }

    Stream_Free(s, TRUE);
}

/**
 * Test which verifies that build_stream_list with NULL parameters fails.
 */
void test_rdpecam_proto__build_stream_list_null(void) {
    wStream* s = Stream_New(NULL, 1024);
    CU_ASSERT_PTR_NOT_NULL_FATAL(s);

    rdpecam_stream_desc streams[1] = {{0}};

    BOOL result = rdpecam_build_stream_list(NULL, streams, 1);
    CU_ASSERT_FALSE(result);

    result = rdpecam_build_stream_list(s, NULL, 1);
    CU_ASSERT_FALSE(result);

    Stream_Free(s, TRUE);
}

/**
 * Test which verifies that build_media_type_list creates a valid message.
 */
void test_rdpecam_proto__build_media_type_list(void) {
    wStream* s = Stream_New(NULL, 1024);
    CU_ASSERT_PTR_NOT_NULL_FATAL(s);

    rdpecam_media_type_desc media_types[2] = {
        { CAM_MEDIA_FORMAT_H264, 640, 480, 30, 1, 1, 1, 0 },
        { CAM_MEDIA_FORMAT_H264, 1280, 720, 60, 1, 1, 1, 0 }
    };

    BOOL result = rdpecam_build_media_type_list(s, media_types, 2);
    CU_ASSERT_TRUE(result);

    Stream_Seek(s, 0);
    uint8_t version = Stream_Read_UINT8(s);
    uint8_t msg_id = Stream_Read_UINT8(s);
    CU_ASSERT_EQUAL(version, RDPECAM_PROTO_VERSION);
    CU_ASSERT_EQUAL(msg_id, RDPECAM_MSG_MEDIA_TYPE_LIST_RESPONSE);

    for (int i = 0; i < 2; i++) {
        uint8_t format = Stream_Read_UINT8(s);
        uint32_t width = Stream_Read_UINT32(s);
        uint32_t height = Stream_Read_UINT32(s);
        uint32_t fps_num = Stream_Read_UINT32(s);
        uint32_t fps_den = Stream_Read_UINT32(s);
        uint32_t par_num = Stream_Read_UINT32(s);
        uint32_t par_den = Stream_Read_UINT32(s);
        uint8_t flags = Stream_Read_UINT8(s);

        CU_ASSERT_EQUAL(format, media_types[i].Format);
        CU_ASSERT_EQUAL(width, media_types[i].Width);
        CU_ASSERT_EQUAL(height, media_types[i].Height);
        CU_ASSERT_EQUAL(fps_num, media_types[i].FrameRateNumerator);
        CU_ASSERT_EQUAL(fps_den, media_types[i].FrameRateDenominator);
        CU_ASSERT_EQUAL(par_num, media_types[i].PixelAspectRatioNumerator);
        CU_ASSERT_EQUAL(par_den, media_types[i].PixelAspectRatioDenominator);
        CU_ASSERT_EQUAL(flags, media_types[i].Flags);
    }

    Stream_Free(s, TRUE);
}

/**
 * Test which verifies that build_current_media_type creates a valid message.
 */
void test_rdpecam_proto__build_current_media_type(void) {
    wStream* s = Stream_New(NULL, 1024);
    CU_ASSERT_PTR_NOT_NULL_FATAL(s);

    rdpecam_media_type_desc media_type = {
        CAM_MEDIA_FORMAT_H264, 1920, 1080, 30, 1, 1, 1, 0
    };

    BOOL result = rdpecam_build_current_media_type(s, &media_type);
    CU_ASSERT_TRUE(result);

    Stream_Seek(s, 0);
    uint8_t version = Stream_Read_UINT8(s);
    uint8_t msg_id = Stream_Read_UINT8(s);
    CU_ASSERT_EQUAL(version, RDPECAM_PROTO_VERSION);
    CU_ASSERT_EQUAL(msg_id, RDPECAM_MSG_CURRENT_MEDIA_TYPE_RESPONSE);

    uint8_t format = Stream_Read_UINT8(s);
    uint32_t width = Stream_Read_UINT32(s);
    uint32_t height = Stream_Read_UINT32(s);
    uint32_t fps_num = Stream_Read_UINT32(s);
    uint32_t fps_den = Stream_Read_UINT32(s);
    uint32_t par_num = Stream_Read_UINT32(s);
    uint32_t par_den = Stream_Read_UINT32(s);
    uint8_t flags = Stream_Read_UINT8(s);

    CU_ASSERT_EQUAL(format, media_type.Format);
    CU_ASSERT_EQUAL(width, media_type.Width);
    CU_ASSERT_EQUAL(height, media_type.Height);
    CU_ASSERT_EQUAL(fps_num, media_type.FrameRateNumerator);
    CU_ASSERT_EQUAL(fps_den, media_type.FrameRateDenominator);
    CU_ASSERT_EQUAL(par_num, media_type.PixelAspectRatioNumerator);
    CU_ASSERT_EQUAL(par_den, media_type.PixelAspectRatioDenominator);
    CU_ASSERT_EQUAL(flags, media_type.Flags);

    Stream_Free(s, TRUE);
}

/**
 * Test which verifies that parse_sample_credits parses correctly.
 */
void test_rdpecam_proto__parse_sample_credits(void) {
    uint8_t payload[4] = { 0x34, 0x12, 0x00, 0x00 }; /* Little-endian 0x1234 */
    uint32_t credits = 0;

    BOOL result = rdpecam_parse_sample_credits(payload, sizeof(payload), &credits);
    CU_ASSERT_TRUE(result);
    CU_ASSERT_EQUAL(credits, 0x1234);
}

/**
 * Test which verifies that parse_sample_credits with NULL parameters fails.
 */
void test_rdpecam_proto__parse_sample_credits_null(void) {
    uint8_t payload[4] = {0};

    BOOL result = rdpecam_parse_sample_credits(NULL, 4, NULL);
    CU_ASSERT_FALSE(result);

    uint32_t credits = 0;
    result = rdpecam_parse_sample_credits(payload, 3, &credits); /* Too small */
    CU_ASSERT_FALSE(result);
}

/**
 * Test which verifies that parse_start_streams parses correctly.
 */
void test_rdpecam_proto__parse_start_streams(void) {
    uint8_t payload[27];
    uint8_t* p = payload;

    *p++ = 0; /* stream index */
    *p++ = CAM_MEDIA_FORMAT_H264; /* Format */
    
    /* Width = 640 (little-endian) */
    *p++ = 0x80; *p++ = 0x02; *p++ = 0x00; *p++ = 0x00;
    /* Height = 480 (little-endian) */
    *p++ = 0xE0; *p++ = 0x01; *p++ = 0x00; *p++ = 0x00;
    /* FPS numerator = 30 */
    *p++ = 0x1E; *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;
    /* FPS denominator = 1 */
    *p++ = 0x01; *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;
    /* PAR numerator = 1 */
    *p++ = 0x01; *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;
    /* PAR denominator = 1 */
    *p++ = 0x01; *p++ = 0x00; *p++ = 0x00; *p++ = 0x00;
    *p++ = 0; /* Flags */

    uint8_t stream_index = 0;
    rdpecam_media_type_desc media_type = {0};

    BOOL result = rdpecam_parse_start_streams(payload, sizeof(payload),
            &stream_index, &media_type);
    CU_ASSERT_TRUE(result);
    CU_ASSERT_EQUAL(stream_index, 0);
    CU_ASSERT_EQUAL(media_type.Format, CAM_MEDIA_FORMAT_H264);
    CU_ASSERT_EQUAL(media_type.Width, 640);
    CU_ASSERT_EQUAL(media_type.Height, 480);
    CU_ASSERT_EQUAL(media_type.FrameRateNumerator, 30);
    CU_ASSERT_EQUAL(media_type.FrameRateDenominator, 1);
}

/**
 * Test which verifies that parse_start_streams with invalid parameters fails.
 */
void test_rdpecam_proto__parse_start_streams_invalid(void) {
    uint8_t payload[27] = {0};
    uint8_t stream_index = 0;
    rdpecam_media_type_desc media_type = {0};

    BOOL result = rdpecam_parse_start_streams(NULL, 27, &stream_index, &media_type);
    CU_ASSERT_FALSE(result);

    result = rdpecam_parse_start_streams(payload, 26, &stream_index, &media_type); /* Too small */
    CU_ASSERT_FALSE(result);
}

/**
 * Test which verifies that parse_sample_request parses correctly.
 */
void test_rdpecam_proto__parse_sample_request(void) {
    uint8_t payload[1] = { 5 }; /* stream index */
    uint8_t stream_index = 0;

    BOOL result = rdpecam_parse_sample_request(payload, sizeof(payload), &stream_index);
    CU_ASSERT_TRUE(result);
    CU_ASSERT_EQUAL(stream_index, 5);
}

/**
 * Test which verifies that parse_stop_streams succeeds.
 */
void test_rdpecam_proto__parse_stop_streams(void) {
    uint8_t payload[0] = {};

    BOOL result = rdpecam_parse_stop_streams(payload, 0);
    CU_ASSERT_TRUE(result);
}

/**
 * Test which verifies that write_sample_response_header creates a valid message.
 */
void test_rdpecam_proto__write_sample_response_header(void) {
    wStream* s = Stream_New(NULL, 1024);
    CU_ASSERT_PTR_NOT_NULL_FATAL(s);

    BOOL result = rdpecam_write_sample_response_header(s, 0, 1, 100, 1000000);
    CU_ASSERT_TRUE(result);

    Stream_Seek(s, 0);
    uint8_t version = Stream_Read_UINT8(s);
    uint8_t msg_id = Stream_Read_UINT8(s);
    uint8_t stream_id = Stream_Read_UINT8(s);

    CU_ASSERT_EQUAL(version, RDPECAM_PROTO_VERSION);
    CU_ASSERT_EQUAL(msg_id, RDPECAM_MSG_SAMPLE_RESPONSE);
    CU_ASSERT_EQUAL(stream_id, 0);

    Stream_Free(s, TRUE);
}

/**
 * Test which verifies that write_sample_response_header with NULL stream fails.
 */
void test_rdpecam_proto__write_sample_response_header_null(void) {
    BOOL result = rdpecam_write_sample_response_header(NULL, 0, 1, 100, 1000000);
    CU_ASSERT_FALSE(result);
}

