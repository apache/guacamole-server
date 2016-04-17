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

#include "ai_messages.h"
#include "rdp.h"

#include <stdlib.h>

#include <freerdp/freerdp.h>
#include <freerdp/constants.h>
#include <freerdp/dvc.h>
#include <guacamole/client.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

/**
 * Reads AUDIO_FORMAT data from the given stream into the given struct.
 *
 * @param stream
 *     The stream to read AUDIO_FORMAT data from.
 *
 * @param format
 *     The structure to populate with data from the stream.
 */
static void guac_rdp_ai_read_format(wStream* stream,
        guac_rdp_ai_format* format) {

    /* Read audio format into structure */
    Stream_Read_UINT16(stream, format->tag); /* wFormatTag */
    Stream_Read_UINT16(stream, format->channels); /* nChannels */
    Stream_Read_UINT32(stream, format->rate); /* nSamplesPerSec */
    Stream_Read_UINT32(stream, format->bytes_per_sec); /* nAvgBytesPerSec */
    Stream_Read_UINT16(stream, format->block_align); /* nBlockAlign */
    Stream_Read_UINT16(stream, format->bps); /* wBitsPerSample */
    Stream_Read_UINT16(stream, format->data_size); /* cbSize */

    /* Read arbitrary data block (if applicable) */
    if (format->data_size != 0) {
        format->data = Stream_Pointer(stream); /* data */
        Stream_Seek(stream, format->data_size);
    }

}

/**
 * Writes AUDIO_FORMAT data to the given stream from the given struct.
 *
 * @param stream
 *     The stream to write AUDIO_FORMAT data to.
 *
 * @param format
 *     The structure containing the data that should be written to the stream.
 */
static void guac_rdp_ai_write_format(wStream* stream,
        guac_rdp_ai_format* format) {

    /* Write audio format into structure */
    Stream_Write_UINT16(stream, format->tag); /* wFormatTag */
    Stream_Write_UINT16(stream, format->channels); /* nChannels */
    Stream_Write_UINT32(stream, format->rate); /* nSamplesPerSec */
    Stream_Write_UINT32(stream, format->bytes_per_sec); /* nAvgBytesPerSec */
    Stream_Write_UINT16(stream, format->block_align); /* nBlockAlign */
    Stream_Write_UINT16(stream, format->bps); /* wBitsPerSample */
    Stream_Write_UINT16(stream, format->data_size); /* cbSize */

    /* Write arbitrary data block (if applicable) */
    if (format->data_size != 0)
        Stream_Write(stream, format->data, format->data_size);

}

/**
 * Sends a Data Incoming PDU along the given channel. A Data Incoming PDU is
 * used by the client to indicate to the server that format or audio data is
 * about to be sent.
 *
 * @param channel
 *     The channel along which the PDU should be sent.
 */
static void guac_rdp_ai_send_incoming_data(IWTSVirtualChannel* channel) {

    /* Build response version PDU */
    wStream* response = Stream_New(NULL, 1);
    Stream_Write_UINT8(response, GUAC_RDP_MSG_SNDIN_DATA_INCOMING); /* MessageId */

    /* Send response */
    channel->Write(channel, (UINT32) Stream_GetPosition(response),
            Stream_Buffer(response), NULL);
    Stream_Free(response, TRUE);

}

/**
 * Sends a Sound Formats PDU along the given channel. A Sound Formats PDU is
 * used by the client to indicate to the server which formats of audio it
 * supports (in response to the server sending exactly the same type of PDU).
 * This PDU MUST be preceded by the Data Incoming PDU.
 *
 * @param channel
 *     The channel along which the PDU should be sent.
 *
 * @param formats
 *     An array of all supported formats.
 *
 * @param num_formats
 *     The number of entries in the formats array.
 */
static void guac_rdp_ai_send_formats(IWTSVirtualChannel* channel,
        guac_rdp_ai_format* formats, int num_formats) {

    int index;
    int packet_size = 9;

    /* Calculate packet size */
    for (index = 0; index < num_formats; index++)
        packet_size += 18 + formats[index].data_size;

    wStream* stream = Stream_New(NULL, packet_size);

    /* Write header */
    Stream_Write_UINT8(stream, GUAC_RDP_MSG_SNDIN_FORMATS); /* MessageId */
    Stream_Write_UINT32(stream, num_formats); /* NumFormats */
    Stream_Write_UINT32(stream, packet_size); /* cbSizeFormatsPacket  */

    /* Write all formats */
    for (index = 0; index < num_formats; index++)
        guac_rdp_ai_write_format(stream, &(formats[index]));

    /* Send PDU */
    channel->Write(channel, (UINT32) Stream_GetPosition(stream),
            Stream_Buffer(stream), NULL);
    Stream_Free(stream, TRUE);

}

void guac_rdp_ai_process_version(guac_client* client,
        IWTSVirtualChannel* channel, wStream* stream) {

    UINT32 version;
    Stream_Read_UINT32(stream, version);

    /* Warn if server's version number is incorrect */
    if (version != 1)
        guac_client_log(client, GUAC_LOG_WARNING,
                "Server reports AUDIO_INPUT version %i, not 1", version);

    /* Build response version PDU */
    wStream* response = Stream_New(NULL, 5);
    Stream_Write_UINT8(response,  GUAC_RDP_MSG_SNDIN_VERSION); /* MessageId */
    Stream_Write_UINT32(response, 1); /* Version */

    /* Send response */
    channel->Write(channel, (UINT32) Stream_GetPosition(response),
            Stream_Buffer(response), NULL);
    Stream_Free(response, TRUE);

}

void guac_rdp_ai_process_formats(guac_client* client,
        IWTSVirtualChannel* channel, wStream* stream) {

    UINT32 num_formats;
    Stream_Read_UINT32(stream, num_formats); /* NumFormats */
    Stream_Seek_UINT32(stream); /* cbSizeFormatsPacket (MUST BE IGNORED) */

    UINT32 index;
    for (index = 0; index < num_formats; index++) {

        guac_rdp_ai_format format;
        guac_rdp_ai_read_format(stream, &format);

        /* Ignore anything but WAVE_FORMAT_PCM */
        if (format.tag != GUAC_RDP_WAVE_FORMAT_PCM)
            continue;

        /* Accept single format */
        guac_rdp_ai_send_incoming_data(channel);
        guac_rdp_ai_send_formats(channel, &format, 1);
        return;

    }

    /* No formats available */
    guac_client_log(client, GUAC_LOG_WARNING, "AUDIO_INPUT: No WAVE format.");
    guac_rdp_ai_send_incoming_data(channel);
    guac_rdp_ai_send_formats(channel, NULL, 0);

}

void guac_rdp_ai_process_open(guac_client* client,
        IWTSVirtualChannel* channel, wStream* stream) {

    /* STUB */
    guac_client_log(client, GUAC_LOG_DEBUG, "AUDIO_INPUT: open");

}

void guac_rdp_ai_process_formatchange(guac_client* client,
        IWTSVirtualChannel* channel, wStream* stream) {

    /* STUB */
    guac_client_log(client, GUAC_LOG_DEBUG, "AUDIO_INPUT: formatchange");

}

