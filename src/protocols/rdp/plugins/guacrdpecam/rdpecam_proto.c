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

#include "rdpecam_proto.h"

#include <guacamole/client.h>
#include <winpr/stream.h>

#include <inttypes.h>
#include <string.h>

BOOL rdpecam_write_sample_response_header(wStream* s,
        uint32_t streamId, uint32_t sampleSequence,
        uint32_t payloadLength, uint64_t ptsHundredsOfNs) {
    WINPR_UNUSED(sampleSequence);
    WINPR_UNUSED(payloadLength);
    WINPR_UNUSED(ptsHundredsOfNs);

    if (!s)
        return FALSE;

    /* FreeRDP/MS-RDPECAM SampleResponse header:
     * [Version (1)][MsgId (1)==SampleResponse][StreamIndex (1)]
     * The sample payload follows immediately.
     */
    if (!Stream_EnsureRemainingCapacity(s, 3))
        return FALSE;

    Stream_Write_UINT8(s, RDPECAM_PROTO_VERSION);
    Stream_Write_UINT8(s, RDPECAM_MSG_SAMPLE_RESPONSE);
    Stream_Write_UINT8(s, (uint8_t)(streamId & 0xFF));

    return TRUE;
}

BOOL rdpecam_build_version_request(wStream* s) {
    if (!s) return FALSE;
    if (!Stream_EnsureRemainingCapacity(s, 2)) return FALSE;
    Stream_Write_UINT8(s, RDPECAM_PROTO_VERSION);
    Stream_Write_UINT8(s, RDPECAM_MSG_SELECT_VERSION_REQUEST);
    return TRUE;
}

BOOL rdpecam_build_version_response(wStream* s) {
    if (!s) return FALSE;
    if (!Stream_EnsureRemainingCapacity(s, 2)) return FALSE;
    Stream_Write_UINT8(s, RDPECAM_PROTO_VERSION);
    Stream_Write_UINT8(s, RDPECAM_MSG_SELECT_VERSION_RESPONSE);
    return TRUE;
}

BOOL rdpecam_build_device_added(wStream* s, const char* device_name,
        const char* channel_name) {
    if (!s || !device_name || !channel_name) return FALSE;
    
    /* Calculate UTF-16 length for device name (each char becomes 2 bytes + 2 for NUL) */
    size_t name_len = strlen(device_name);
    size_t utf16_bytes = (name_len + 1) * 2; /* +1 for NUL */
    size_t ch_len = strlen(channel_name) + 1; /* include NUL */
    
    if (!Stream_EnsureRemainingCapacity(s, 2 + utf16_bytes + ch_len)) return FALSE;
    
    Stream_Write_UINT8(s, RDPECAM_PROTO_VERSION);
    Stream_Write_UINT8(s, RDPECAM_MSG_DEVICE_ADDED_NOTIFICATION);
    
    /* Write device name as UTF-16LE with NUL terminator */
    for (size_t i = 0; i < name_len; i++) {
        Stream_Write_UINT16(s, (uint16_t)(unsigned char)device_name[i]);
    }
    Stream_Write_UINT16(s, 0); /* NUL terminator */
    
    /* Write channel name as ASCII with NUL terminator */
    Stream_Write(s, channel_name, ch_len);
    
    return TRUE;
}

BOOL rdpecam_build_success_response(wStream* s) {
    if (!s) return FALSE;
    if (!Stream_EnsureRemainingCapacity(s, 2)) return FALSE;
    Stream_Write_UINT8(s, RDPECAM_PROTO_VERSION);
    Stream_Write_UINT8(s, RDPECAM_MSG_SUCCESS_RESPONSE);
    return TRUE;
}

BOOL rdpecam_build_stream_list(wStream* s, const rdpecam_stream_desc* streams, size_t count) {
    if (!s || !streams) return FALSE;
    
    /* Header (2) + count * 5 bytes per stream descriptor (no explicit count field) */
    if (!Stream_EnsureRemainingCapacity(s, 2 + count * 5)) return FALSE;
    
    Stream_Write_UINT8(s, RDPECAM_PROTO_VERSION);
    Stream_Write_UINT8(s, RDPECAM_MSG_STREAM_LIST_RESPONSE);
    /* No count field - server calculates it from message length / 5 */
    
    for (size_t i = 0; i < count; i++) {
        Stream_Write_UINT16(s, streams[i].FrameSourceType);
        Stream_Write_UINT8(s, streams[i].Category);
        Stream_Write_UINT8(s, streams[i].Selected);
        Stream_Write_UINT8(s, streams[i].CanBeShared);
    }
    
    return TRUE;
}

BOOL rdpecam_build_media_type_list(wStream* s, const rdpecam_media_type_desc* media_types,
        size_t count) {
    if (!s || !media_types) return FALSE;
    
    /* Header (2) + count * 26 bytes per media type descriptor */
    if (!Stream_EnsureRemainingCapacity(s, 2 + count * 26)) return FALSE;
    
    Stream_Write_UINT8(s, RDPECAM_PROTO_VERSION);
    Stream_Write_UINT8(s, RDPECAM_MSG_MEDIA_TYPE_LIST_RESPONSE);
    
    for (size_t i = 0; i < count; i++) {
        Stream_Write_UINT8(s, media_types[i].Format);
        Stream_Write_UINT32(s, media_types[i].Width);
        Stream_Write_UINT32(s, media_types[i].Height);
        Stream_Write_UINT32(s, media_types[i].FrameRateNumerator);
        Stream_Write_UINT32(s, media_types[i].FrameRateDenominator);
        Stream_Write_UINT32(s, media_types[i].PixelAspectRatioNumerator);
        Stream_Write_UINT32(s, media_types[i].PixelAspectRatioDenominator);
        Stream_Write_UINT8(s, media_types[i].Flags);
    }
    
    return TRUE;
}

BOOL rdpecam_build_current_media_type(wStream* s, const rdpecam_media_type_desc* media_type) {
    if (!s || !media_type) return FALSE;
    
    /* Header (2) + 26 bytes for media type descriptor */
    if (!Stream_EnsureRemainingCapacity(s, 2 + 26)) return FALSE;
    
    Stream_Write_UINT8(s, RDPECAM_PROTO_VERSION);
    Stream_Write_UINT8(s, RDPECAM_MSG_CURRENT_MEDIA_TYPE_RESPONSE);
    
    Stream_Write_UINT8(s, media_type->Format);
    Stream_Write_UINT32(s, media_type->Width);
    Stream_Write_UINT32(s, media_type->Height);
    Stream_Write_UINT32(s, media_type->FrameRateNumerator);
    Stream_Write_UINT32(s, media_type->FrameRateDenominator);
    Stream_Write_UINT32(s, media_type->PixelAspectRatioNumerator);
    Stream_Write_UINT32(s, media_type->PixelAspectRatioDenominator);
    Stream_Write_UINT8(s, media_type->Flags);
    
    return TRUE;
}

void rdpecam_log_hex_dump(guac_client* client, const char* prefix,
        const void* data, size_t len, size_t max_len) {

    if (!client || !data)
        return;

    const uint8_t* bytes = (const uint8_t*) data;
    size_t dump_len = (len < max_len) ? len : max_len;

    char line[3 * 16 + 1];
    size_t pos = 0;
    for (size_t i = 0; i < dump_len; i++) {
        int n = snprintf(&line[pos], sizeof(line) - pos, "%02X ", bytes[i]);
        if (n <= 0) break;
        pos += (size_t) n;
        if ((i % 16) == 15 || i + 1 == dump_len) {
            line[pos] = '\0';
            guac_client_log(client, GUAC_LOG_DEBUG, "%s: %s", prefix, line);
            pos = 0;
        }
    }
}

BOOL rdpecam_parse_sample_credits(const uint8_t* payload, size_t payload_len,
        uint32_t* out_credits) {

    if (!payload || !out_credits || payload_len < 4)
        return FALSE;

    uint32_t value =
        ((uint32_t) payload[0]) |
        ((uint32_t) payload[1] << 8) |
        ((uint32_t) payload[2] << 16) |
        ((uint32_t) payload[3] << 24);

    *out_credits = value;
    return TRUE;
}

BOOL rdpecam_parse_start_streams(const uint8_t* payload, size_t payload_len,
        uint8_t* out_stream_index, rdpecam_media_type_desc* out_media_type) {
    if (!payload || !out_stream_index || !out_media_type)
        return FALSE;
    
    /* Expect: [streamIndex (1)][MediaTypeDesc (26)] = 27 bytes */
    if (payload_len < 27)
        return FALSE;
    
    const uint8_t* p = payload;
    
    /* Read stream index */
    *out_stream_index = *p++;
    
    /* Read media type descriptor (26 bytes) */
    out_media_type->Format = *p++;
    
    out_media_type->Width = 
        ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) | 
        ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
    p += 4;
    
    out_media_type->Height = 
        ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) | 
        ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
    p += 4;
    
    out_media_type->FrameRateNumerator = 
        ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) | 
        ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
    p += 4;
    
    out_media_type->FrameRateDenominator = 
        ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) | 
        ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
    p += 4;
    
    out_media_type->PixelAspectRatioNumerator = 
        ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) | 
        ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
    p += 4;
    
    out_media_type->PixelAspectRatioDenominator = 
        ((uint32_t)p[0]) | ((uint32_t)p[1] << 8) | 
        ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
    p += 4;
    
    out_media_type->Flags = *p++;
    
    return TRUE;
}

BOOL rdpecam_parse_current_media_type_request(const uint8_t* payload, size_t payload_len,
        uint8_t* out_stream_index) {
    if (!payload || !out_stream_index || payload_len < 1)
        return FALSE;
    *out_stream_index = payload[0];
    return TRUE;
}

BOOL rdpecam_parse_media_type_list_request(const uint8_t* payload, size_t payload_len,
        uint8_t* out_stream_index) {
    if (!payload || !out_stream_index || payload_len < 1)
        return FALSE;
    *out_stream_index = payload[0];
    return TRUE;
}

BOOL rdpecam_parse_sample_request(const uint8_t* payload, size_t payload_len,
        uint8_t* out_stream_index) {
    if (!payload || !out_stream_index || payload_len < 1)
        return FALSE;
    *out_stream_index = payload[0];
    return TRUE;
}

BOOL rdpecam_parse_stop_streams(const uint8_t* payload, size_t payload_len) {
    /* StopStreamsRequest has no payload in single-stream implementations */
    (void)payload;
    (void)payload_len;
    return TRUE;
}

BOOL rdpecam_build_start_streams_response(wStream* s, uint32_t status) {
    (void) status; /* SuccessResponse has no status payload */
    if (!s) return FALSE;
    if (!Stream_EnsureRemainingCapacity(s, 2)) return FALSE;
    Stream_Write_UINT8(s, RDPECAM_PROTO_VERSION);
    Stream_Write_UINT8(s, RDPECAM_MSG_SUCCESS_RESPONSE);
    return TRUE;
}

BOOL rdpecam_build_stop_streams_response(wStream* s, uint32_t status) {
    (void) status; /* SuccessResponse has no status payload */
    if (!s) return FALSE;
    if (!Stream_EnsureRemainingCapacity(s, 2)) return FALSE;
    Stream_Write_UINT8(s, RDPECAM_PROTO_VERSION);
    Stream_Write_UINT8(s, RDPECAM_MSG_SUCCESS_RESPONSE);
    return TRUE;
}

BOOL rdpecam_build_sample_error_response(wStream* s, uint8_t streamIndex) {
    if (!s) return FALSE;
    if (!Stream_EnsureRemainingCapacity(s, 3)) return FALSE;
    Stream_Write_UINT8(s, RDPECAM_PROTO_VERSION);
    Stream_Write_UINT8(s, RDPECAM_MSG_SAMPLE_ERROR_RESPONSE);
    Stream_Write_UINT8(s, streamIndex);
    return TRUE;
}

BOOL rdpecam_build_device_removed(wStream* s, const char* channel_name) {
    if (!s || !channel_name) return FALSE;
    size_t ch_len = strlen(channel_name) + 1; /* include NUL */
    if (!Stream_EnsureRemainingCapacity(s, 2 + ch_len)) return FALSE;
    Stream_Write_UINT8(s, RDPECAM_PROTO_VERSION);
    Stream_Write_UINT8(s, RDPECAM_MSG_DEVICE_REMOVED_NOTIFICATION);
    Stream_Write(s, channel_name, ch_len);
    return TRUE;
}

