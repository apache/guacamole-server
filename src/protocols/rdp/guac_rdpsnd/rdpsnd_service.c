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

#include "rdpsnd_service.h"
#include "rdpsnd_messages.h"

#include <stdlib.h>
#include <string.h>

#include <freerdp/constants.h>
#include <guacamole/client.h>
#include <winpr/stream.h>

/**
 * Processes data received along the RDPSND channel via a
 * CHANNEL_EVENT_DATA_RECEIVED event, forwarding the data along an established,
 * outbound pipe stream to the Guacamole client.
 *
 * @param rdpsnd
 *     The guac_rdpsnd structure representing the RDPSND channel.
 *
 * @param input_stream
 *     The data that was received.
 */
static void guac_rdpsnd_process_receive(guac_rdpsnd* rdpsnd,
        wStream* input_stream) {

    guac_rdpsnd_pdu_header header;

    /* Read RDPSND PDU header */
    Stream_Read_UINT8(input_stream, header.message_type);
    Stream_Seek_UINT8(input_stream);
    Stream_Read_UINT16(input_stream, header.body_size);

    /* 
     * If next PDU is SNDWAVE (due to receiving WaveInfo PDU previously),
     * ignore the header and parse as a Wave PDU.
     */
    if (rdpsnd->next_pdu_is_wave) {
        guac_rdpsnd_wave_handler(rdpsnd, input_stream, &header);
        return;
    }

    /* Dispatch message to standard handlers */
    switch (header.message_type) {

        /* Server Audio Formats and Version PDU */
        case SNDC_FORMATS:
            guac_rdpsnd_formats_handler(rdpsnd, input_stream, &header);
            break;

        /* Training PDU */
        case SNDC_TRAINING:
            guac_rdpsnd_training_handler(rdpsnd, input_stream, &header);
            break;

        /* WaveInfo PDU */
        case SNDC_WAVE:
            guac_rdpsnd_wave_info_handler(rdpsnd, input_stream, &header);
            break;

        /* Close PDU */
        case SNDC_CLOSE:
            guac_rdpsnd_close_handler(rdpsnd, input_stream, &header);
            break;

    }

}

/**
 * Event handler for events which deal with data transmitted over the RDPSND
 * channel.  This specific implementation of the event handler currently
 * handles only the CHANNEL_EVENT_DATA_RECEIVED event, delegating actual
 * handling of that event to guac_rdpsnd_process_receive().
 *
 * The FreeRDP requirements for this function follow those of the
 * VirtualChannelOpenEventEx callback defined within Microsoft's RDP API:
 *
 * https://docs.microsoft.com/en-us/previous-versions/windows/embedded/aa514754%28v%3dmsdn.10%29
 *
 * @param user_param
 *     The pointer to arbitrary data originally passed via the first parameter
 *     of the pVirtualChannelInitEx() function call when the associated channel
 *     was initialized. The pVirtualChannelInitEx() function is exposed within
 *     the channel entry points structure.
 *
 * @param open_handle
 *     The handle which identifies the channel itself, typically referred to
 *     within the FreeRDP source as OpenHandle.
 *
 * @param event
 *     An integer representing the event that should be handled. This will be
 *     either CHANNEL_EVENT_DATA_RECEIVED, CHANNEL_EVENT_WRITE_CANCELLED, or
 *     CHANNEL_EVENT_WRITE_COMPLETE.
 *
 * @param data
 *     The data received, for CHANNEL_EVENT_DATA_RECEIVED events, and the value
 *     passed as user data to pVirtualChannelWriteEx() for
 *     CHANNEL_EVENT_WRITE_* events (note that user data for
 *     pVirtualChannelWriteEx() as implemented by FreeRDP MUST either be NULL
 *     or a wStream containing the data written).
 *
 * @param data_length
 *     The number of bytes of event-specific data.
 *
 * @param total_length
 *     The total number of bytes written to the RDP server in a single write
 *     operation.
 *
 *     NOTE: The meaning of total_length is unclear. The above description was
 *     written mainly through referencing the documentation in MSDN. Real-world
 *     use will need to be consulted, likely within the FreeRDP source, before
 *     this value can be reliably used. The current implementation of this
 *     handler ignores this parameter.
 *
 * @param data_flags
 *     The result of a bitwise OR of the CHANNEL_FLAG_* flags which apply to
 *     the data received. This value is relevant only to
 *     CHANNEL_EVENT_DATA_RECEIVED events. Valid flags are CHANNEL_FLAG_FIRST,
 *     CHANNEL_FLAG_LAST, and CHANNEL_FLAG_ONLY. The flag CHANNEL_FLAG_MIDDLE
 *     is not itself a flag, but the absence of both CHANNEL_FLAG_FIRST and
 *     CHANNEL_FLAG_LAST.
 */
static VOID guac_rdpsnd_handle_open_event(LPVOID user_param,
        DWORD open_handle, UINT event, LPVOID data, UINT32 data_length,
        UINT32 total_length, UINT32 data_flags) {

    /* Ignore all events except for received data */
    if (event != CHANNEL_EVENT_DATA_RECEIVED)
        return;

    guac_rdpsnd* rdpsnd = (guac_rdpsnd*) user_param;

    /* Validate relevant handle matches that of the RDPSND channel */
    if (open_handle != rdpsnd->open_handle) {
        guac_client_log(rdpsnd->client, GUAC_LOG_WARNING, "%i bytes of data "
                "received from within the remote desktop session for the "
                "RDPSND channel are being dropped because the relevant open "
                "handle (0x%X) does not match the open handle of RDPSND "
                "(0x%X).", data_length, rdpsnd->channel_def.name, open_handle,
                rdpsnd->open_handle);
        return;
    }

    wStream* input_stream = Stream_New(data, data_length);
    guac_rdpsnd_process_receive(rdpsnd, input_stream);
    Stream_Free(input_stream, FALSE);

}

/**
 * Processes a CHANNEL_EVENT_CONNECTED event, completing the
 * connection/initialization process of the RDPSND channel.
 *
 * @param rdpsnd
 *     The guac_rdpsnd structure representing the RDPSND channel.
 */
static void guac_rdpsnd_process_connect(guac_rdpsnd* rdpsnd) {

    /* Open FreeRDP side of connected channel */
    UINT32 open_status =
        rdpsnd->entry_points.pVirtualChannelOpenEx(rdpsnd->init_handle,
                &rdpsnd->open_handle, rdpsnd->channel_def.name,
                guac_rdpsnd_handle_open_event);

    /* Warn if the channel cannot be opened after all */
    if (open_status != CHANNEL_RC_OK) {
        guac_client_log(rdpsnd->client, GUAC_LOG_WARNING, "RDPSND channel "
                "could not be opened: %s (error %i)",
                WTSErrorToString(open_status), open_status);
        return;
    }

    /* Log that sound has been loaded */
    guac_client_log(rdpsnd->client, GUAC_LOG_INFO, "RDPSND channel "
            "connected.");

}

/**
 * Processes a CHANNEL_EVENT_TERMINATED event, freeing all resources associated
 * with the RDPSND channel.
 *
 * @param rdpsnd
 *     The guac_rdpsnd structure representing the RDPSND channel.
 */
static void guac_rdpsnd_process_terminate(guac_rdpsnd* rdpsnd) {
    guac_client_log(rdpsnd->client, GUAC_LOG_INFO, "RDPSND channel disconnected.");
    free(rdpsnd);
}

/**
 * Event handler for events which deal with the overall lifecycle of the RDPSND
 * channel.  This specific implementation of the event handler currently
 * handles only CHANNEL_EVENT_CONNECTED and CHANNEL_EVENT_TERMINATED events,
 * delegating actual handling of those events to guac_rdpsnd_process_connect()
 * and guac_rdpsnd_process_terminate() respectively.
 *
 * The FreeRDP requirements for this function follow those of the
 * VirtualChannelInitEventEx callback defined within Microsoft's RDP API:
 *
 * https://docs.microsoft.com/en-us/previous-versions/windows/embedded/aa514727%28v%3dmsdn.10%29
 *
 * @param user_param
 *     The pointer to arbitrary data originally passed via the first parameter
 *     of the pVirtualChannelInitEx() function call when the associated channel
 *     was initialized. The pVirtualChannelInitEx() function is exposed within
 *     the channel entry points structure.
 *
 * @param init_handle
 *     The handle which identifies the client connection, typically referred to
 *     within the FreeRDP source as pInitHandle.
 *
 * @param event
 *     An integer representing the event that should be handled. This will be
 *     either CHANNEL_EVENT_CONNECTED, CHANNEL_EVENT_DISCONNECTED,
 *     CHANNEL_EVENT_INITIALIZED, CHANNEL_EVENT_TERMINATED, or
 *     CHANNEL_EVENT_V1_CONNECTED.
 *
 * @param data
 *     NULL in all cases except the CHANNEL_EVENT_CONNECTED event, in which
 *     case this is a null-terminated string containing the name of the server.
 *
 * @param data_length
 *     The number of bytes of data, if any.
 */
static VOID guac_rdpsnd_handle_init_event(LPVOID user_param,
        LPVOID init_handle, UINT event, LPVOID data, UINT data_length) {

    guac_rdpsnd* rdpsnd = (guac_rdpsnd*) user_param;

    /* Validate relevant handle matches that of the RDPSND channel */
    if (init_handle != rdpsnd->init_handle) {
        guac_client_log(rdpsnd->client, GUAC_LOG_WARNING, "An init event "
                "(#%i) for the RDPSND channel has been dropped because the "
                "relevant init handle (0x%X) does not match the init handle "
                "of the RDPSND channel (0x%X).", event, init_handle,
                rdpsnd->init_handle);
        return;
    }

    switch (event) {

        /* The RDPSND channel has been connected */
        case CHANNEL_EVENT_CONNECTED:
            guac_rdpsnd_process_connect(rdpsnd);
            break;

        /* The RDPSND channel has disconnected and now must be cleaned up */
        case CHANNEL_EVENT_TERMINATED:
            guac_rdpsnd_process_terminate(rdpsnd);
            break;

    }

}

/**
 * Entry point for FreeRDP plugins. This function is automatically invoked when
 * the plugin is loaded.
 *
 * @param entry_points
 *     Functions and data specific to the FreeRDP side of the virtual channel
 *     and plugin. This structure must be copied within implementation-specific
 *     storage such that the functions it references can be invoked when
 *     needed.
 *
 * @param init_handle
 *     The handle which identifies the client connection, typically referred to
 *     within the FreeRDP source as pInitHandle. This handle is also provided
 *     to the channel init event handler. The handle must eventually be used
 *     within the channel open event handler to obtain a handle to the channel
 *     itself.
 *
 * @return
 *     TRUE if the plugin has initialized successfully, FALSE otherwise.
 */
BOOL VirtualChannelEntryEx(PCHANNEL_ENTRY_POINTS entry_points,
        PVOID init_handle) {

    CHANNEL_ENTRY_POINTS_FREERDP_EX* entry_points_ex =
        (CHANNEL_ENTRY_POINTS_FREERDP_EX*) entry_points;

    /* Allocate plugin */
    guac_rdpsnd* rdpsnd = (guac_rdpsnd*) calloc(1, sizeof(guac_rdpsnd));

    /* Init channel def */
    strcpy(rdpsnd->channel_def.name, "rdpsnd");
    rdpsnd->channel_def.options = CHANNEL_OPTION_INITIALIZED
        | CHANNEL_OPTION_ENCRYPT_RDP;

    /* Maintain reference to associated guac_client */
    rdpsnd->client = (guac_client*) entry_points_ex->pExtendedData;

    /* Copy FreeRDP data into RDPSND structure for future reference */
    rdpsnd->entry_points = *entry_points_ex;
    rdpsnd->init_handle = init_handle;

    /* Complete initialization */
    if (rdpsnd->entry_points.pVirtualChannelInitEx(rdpsnd, rdpsnd, init_handle,
                &rdpsnd->channel_def, 1, VIRTUAL_CHANNEL_VERSION_WIN2000,
                guac_rdpsnd_handle_init_event) != CHANNEL_RC_OK) {
        return FALSE;
    }

    return TRUE;

}

