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

#ifndef GUAC_RDP_PLUGINS_GUACRDPECAM_PROTO_H
#define GUAC_RDP_PLUGINS_GUACRDPECAM_PROTO_H

#include <guacamole/client.h>
#include <winpr/stream.h>
#include <winpr/wtypes.h>

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/*
 * RDPECAM protocol helpers.
 *
 * NOTE: These helpers provide a thin serialization layer for MS-RDPECAM
 * messages. As of now, they intentionally avoid hard-coding GUID values.
 * Where GUIDs/structures are required by the spec, call sites should provide
 * the exact values (typically mirrored from FreeRDP) until we integrate the
 * full set of constants.
 */

#ifdef __cplusplus
extern "C" {
#endif

/** Major version we support (spec-compliant implementation target). */
#define RDPECAM_VERSION_MAJOR 1u

/** Minor version we support (spec-compliant implementation target). */
#define RDPECAM_VERSION_MINOR 0u

/** Protocol version byte written in message headers (matches FreeRDP). */
#define RDPECAM_PROTO_VERSION 0x02u

/**
 * Temporary message type identifiers (to be replaced with MS-RDPECAM values
 * in a later step). Using named constants centralizes usage.
 */
/* Official MS-RDPECAM message IDs (mirroring FreeRDP's CAM_MSG_ID) */
typedef enum rdpecam_msg_type {
    RDPECAM_MSG_SUCCESS_RESPONSE            = 0x01,
    RDPECAM_MSG_ERROR_RESPONSE              = 0x02,
    RDPECAM_MSG_SELECT_VERSION_REQUEST      = 0x03,
    RDPECAM_MSG_SELECT_VERSION_RESPONSE     = 0x04,
    RDPECAM_MSG_DEVICE_ADDED_NOTIFICATION   = 0x05,
    RDPECAM_MSG_DEVICE_REMOVED_NOTIFICATION = 0x06,
    RDPECAM_MSG_ACTIVATE_DEVICE_REQUEST     = 0x07,
    RDPECAM_MSG_DEACTIVATE_DEVICE_REQUEST   = 0x08,
    RDPECAM_MSG_STREAM_LIST_REQUEST         = 0x09,
    RDPECAM_MSG_STREAM_LIST_RESPONSE        = 0x0A,
    RDPECAM_MSG_MEDIA_TYPE_LIST_REQUEST     = 0x0B,
    RDPECAM_MSG_MEDIA_TYPE_LIST_RESPONSE    = 0x0C,
    RDPECAM_MSG_CURRENT_MEDIA_TYPE_REQUEST  = 0x0D,
    RDPECAM_MSG_CURRENT_MEDIA_TYPE_RESPONSE = 0x0E,
    RDPECAM_MSG_START_STREAMS_REQUEST       = 0x0F,
    RDPECAM_MSG_STOP_STREAMS_REQUEST        = 0x10,
    RDPECAM_MSG_SAMPLE_REQUEST              = 0x11,
    RDPECAM_MSG_SAMPLE_RESPONSE             = 0x12,
    RDPECAM_MSG_SAMPLE_ERROR_RESPONSE       = 0x13,
    RDPECAM_MSG_PROPERTY_LIST_REQUEST       = 0x14,
    RDPECAM_MSG_PROPERTY_LIST_RESPONSE      = 0x15,
    RDPECAM_MSG_PROPERTY_VALUE_REQUEST      = 0x16,
    RDPECAM_MSG_PROPERTY_VALUE_RESPONSE     = 0x17,
    RDPECAM_MSG_SET_PROPERTY_VALUE_REQUEST  = 0x18
} rdpecam_msg_type;

/**
 * H.264 media subtype GUID used by MS-RDPECAM. This is the standard
 * KSDATAFORMAT_SUBTYPE_H264 GUID: {34363248-0000-0010-8000-00AA00389B71}.
 * Note: The first DWORD is the little-endian FOURCC for 'H264'.
 */
static const GUID RDPECAM_SUBTYPE_H264 =
    { 0x34363248, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71 } };

/**
 * Writes a SampleResponse header compliant with FreeRDP/MS-RDPECAM.
 * Header layout: [Version (1)][MsgId (1)==SampleResponse][StreamIndex (1)].
 *
 * @param s
 *     The output stream to write to.
 *
 * @param streamId
 *     Identifier of the capture stream.
 *
 * @param sampleSequence
 *     Monotonic sequence number of the sample for the stream.
 *
 * @param payloadLength
 *     Length in bytes of the following Annex-B payload.
 *
 * @param ptsHundredsOfNs
 *     Presentation timestamp in 100-ns units (HNS), per MS-RDPECAM.
 *
 * @return
 *     TRUE on success, FALSE on failure.
 */
BOOL rdpecam_write_sample_response_header(wStream* s,
        uint32_t streamId, uint32_t sampleSequence,
        uint32_t payloadLength, uint64_t ptsHundredsOfNs);

/**
 * Media type descriptor matching FreeRDP's CAM_MEDIA_TYPE_DESCRIPTION (26 bytes).
 */
typedef struct rdpecam_media_type_desc {
    uint8_t  Format;                          /* 1 byte - media format (1=H264) */
    uint32_t Width;                           /* 4 bytes */
    uint32_t Height;                          /* 4 bytes */
    uint32_t FrameRateNumerator;             /* 4 bytes */
    uint32_t FrameRateDenominator;           /* 4 bytes */
    uint32_t PixelAspectRatioNumerator;      /* 4 bytes */
    uint32_t PixelAspectRatioDenominator;    /* 4 bytes */
    uint8_t  Flags;                           /* 1 byte - flags */
} rdpecam_media_type_desc;

/**
 * Stream descriptor matching FreeRDP's CAM_STREAM_DESCRIPTION (5 bytes).
 */
typedef struct rdpecam_stream_desc {
    uint16_t FrameSourceType;  /* 0 = Color */
    uint8_t  Category;         /* 1 = Capture */
    uint8_t  Selected;         /* bool */
    uint8_t  CanBeShared;      /* bool */
} rdpecam_stream_desc;

/* Media format constants */
#define CAM_MEDIA_FORMAT_H264  1

/* Stream constants */
#define CAM_STREAM_FRAME_SOURCE_TYPE_Color  0x0001
#define CAM_STREAM_CATEGORY_Capture  0x01

/* Media type flags */
#define CAM_MEDIA_TYPE_DESCRIPTION_FLAG_DecodingRequired  1

/**
 * Builds SelectVersionRequest: [Version][MsgId].
 * Sent by client to initiate version negotiation.
 *
 * @param s
 *     The output stream to write to.
 *
 * @return
 *     TRUE on success, FALSE on failure.
 */
BOOL rdpecam_build_version_request(wStream* s);

/**
 * Builds SelectVersionResponse: [Version][MsgId].
 * Sent by server in response to version request.
 *
 * @param s
 *     The output stream to write to.
 *
 * @return
 *     TRUE on success, FALSE on failure.
 */
BOOL rdpecam_build_version_response(wStream* s);

/**
 * Builds SuccessResponse: [Version][MsgId].
 * Generic success response for various requests.
 *
 * @param s
 *     The output stream to write to.
 *
 * @return
 *     TRUE on success, FALSE on failure.
 */
BOOL rdpecam_build_success_response(wStream* s);

/**
 * Builds DeviceAddedNotification: [Version][MsgId][DeviceName_UTF16][ChannelName_ASCII].
 * Device name is UTF-16 encoded with NUL terminator.
 * Channel name is ASCII with NUL terminator.
 *
 * @param s
 *     The output stream to write to.
 *
 * @param device_name
 *     The device name to encode in the notification.
 *
 * @param channel_name
 *     The channel name to encode in the notification.
 *
 * @return
 *     TRUE on success, FALSE on failure.
 */
BOOL rdpecam_build_device_added(wStream* s, const char* device_name,
        const char* channel_name);

/**
 * Builds StreamListResponse: [Version][MsgId][StreamDesc...].
 * Contains one or more stream descriptors (6 bytes each).
 *
 * @param s
 *     The output stream to write to.
 *
 * @param streams
 *     Array of stream descriptors to encode.
 *
 * @param count
 *     Number of stream descriptors in the array.
 *
 * @return
 *     TRUE on success, FALSE on failure.
 */
BOOL rdpecam_build_stream_list(wStream* s, const rdpecam_stream_desc* streams, size_t count);

/**
 * Builds MediaTypeListResponse: [Version][MsgId][MediaTypeDesc...].
 * Contains media type descriptors (26 bytes each).
 *
 * @param s
 *     The output stream to write to.
 *
 * @param media_types
 *     Array of media type descriptors to encode.
 *
 * @param count
 *     Number of media type descriptors in the array.
 *
 * @return
 *     TRUE on success, FALSE on failure.
 */
BOOL rdpecam_build_media_type_list(wStream* s, const rdpecam_media_type_desc* media_types,
        size_t count);

/**
 * Builds CurrentMediaTypeResponse: [Version][MsgId][MediaTypeDesc].
 * Contains single media type descriptor (26 bytes).
 *
 * @param s
 *     The output stream to write to.
 *
 * @param media_type
 *     The media type descriptor to encode.
 *
 * @return
 *     TRUE on success, FALSE on failure.
 */
BOOL rdpecam_build_current_media_type(wStream* s, const rdpecam_media_type_desc* media_type);

/**
 * Hex-dumps at most max_len bytes to the Guacamole log at DEBUG level with a
 * given prefix. Intended for temporary wire debugging.
 *
 * @param client
 *     The Guacamole client for logging.
 *
 * @param prefix
 *     Message prefix to precede the hex dump.
 *
 * @param data
 *     Buffer to dump.
 *
 * @param len
 *     Length of the buffer.
 *
 * @param max_len
 *     Maximum number of bytes to dump.
 */
void rdpecam_log_hex_dump(guac_client* client, const char* prefix,
        const void* data, size_t len, size_t max_len);

/**
 * Parses a placeholder SampleRequest-style message payload that conveys the
 * number of credits to grant. This is a shim-compatible parser expecting a
 * 4-byte little-endian unsigned integer (no header), matching current wire.
 *
 * @param payload
 *     Pointer to the payload bytes, excluding any message type byte.
 *
 * @param payload_len
 *     Length of the payload in bytes.
 *
 * @param out_credits
 *     Output pointer to receive the credits value on success.
 *
 * @return
 *     TRUE on success, FALSE on parse error.
 */
BOOL rdpecam_parse_sample_credits(const uint8_t* payload, size_t payload_len,
        uint32_t* out_credits);

/**
 * Parses StartStreamsRequest: [streamIndex (1)][MediaTypeDesc (26)].
 * Returns stream index and full media type descriptor.
 *
 * @param payload
 *     Pointer to the payload bytes.
 *
 * @param payload_len
 *     Length of the payload in bytes.
 *
 * @param out_stream_index
 *     Output pointer to receive the stream index.
 *
 * @param out_media_type
 *     Output pointer to receive the parsed media type descriptor.
 *
 * @return
 *     TRUE on success, FALSE on parse error.
 */
BOOL rdpecam_parse_start_streams(const uint8_t* payload, size_t payload_len,
        uint8_t* out_stream_index, rdpecam_media_type_desc* out_media_type);

/**
 * Parses CurrentMediaTypeRequest: [streamIndex (1)].
 *
 * @param payload
 *     Pointer to the payload bytes.
 *
 * @param payload_len
 *     Length of the payload in bytes.
 *
 * @param out_stream_index
 *     Output pointer to receive the stream index.
 *
 * @return
 *     TRUE on success, FALSE on parse error.
 */
BOOL rdpecam_parse_current_media_type_request(const uint8_t* payload, size_t payload_len,
        uint8_t* out_stream_index);

/**
 * Parses MediaTypeListRequest: [streamIndex (1)].
 *
 * @param payload
 *     Pointer to the payload bytes.
 *
 * @param payload_len
 *     Length of the payload in bytes.
 *
 * @param out_stream_index
 *     Output pointer to receive the stream index.
 *
 * @return
 *     TRUE on success, FALSE on parse error.
 */
BOOL rdpecam_parse_media_type_list_request(const uint8_t* payload, size_t payload_len,
        uint8_t* out_stream_index);

/**
 * Parses SampleRequest: [streamIndex (1)].
 *
 * @param payload
 *     Pointer to the payload bytes.
 *
 * @param payload_len
 *     Length of the payload in bytes.
 *
 * @param out_stream_index
 *     Output pointer to receive the stream index.
 *
 * @return
 *     TRUE on success, FALSE on parse error.
 */
BOOL rdpecam_parse_sample_request(const uint8_t* payload, size_t payload_len,
        uint8_t* out_stream_index);

/**
 * Validates StopStreamsRequest payload (empty for single-stream).
 *
 * @param payload
 *     Pointer to the payload bytes.
 *
 * @param payload_len
 *     Length of the payload in bytes.
 *
 * @return
 *     TRUE if the payload is valid, FALSE otherwise.
 */
BOOL rdpecam_parse_stop_streams(const uint8_t* payload, size_t payload_len);

/**
 * Builds StartStreams response with a 32-bit status code.
 *
 * @param s
 *     The output stream to write to.
 *
 * @param status
 *     Status code to include in the response (currently unused, always 0).
 *
 * @return
 *     TRUE on success, FALSE on failure.
 */
BOOL rdpecam_build_start_streams_response(wStream* s, uint32_t status);

/**
 * Builds StopStreams response with a 32-bit status code.
 *
 * @param s
 *     The output stream to write to.
 *
 * @param status
 *     Status code to include in the response (currently unused, always 0).
 *
 * @return
 *     TRUE on success, FALSE on failure.
 */
BOOL rdpecam_build_stop_streams_response(wStream* s, uint32_t status);

/**
 * Builds SampleErrorResponse: [Version][MsgId][StreamIndex].
 *
 * @param s
 *     The output stream to write to.
 *
 * @param streamIndex
 *     The stream index for which the error is being reported.
 *
 * @return
 *     TRUE on success, FALSE on failure.
 */
BOOL rdpecam_build_sample_error_response(wStream* s, uint8_t streamIndex);

/**
 * Builds DeviceRemovedNotification: [Version][MsgId][ChannelName_ASCII_NUL].
 *
 * @param s
 *     The output stream to write to.
 *
 * @param channel_name
 *     The channel name to encode in the notification.
 *
 * @return
 *     TRUE on success, FALSE on failure.
 */
BOOL rdpecam_build_device_removed(wStream* s, const char* channel_name);

#ifdef __cplusplus
}
#endif

#endif


