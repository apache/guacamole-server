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

#include "svc.h"

#include <stdlib.h>
#include <string.h>

#include <freerdp/constants.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/string.h>
#include <winpr/stream.h>

/**
 * Processes data received along an SVC via a CHANNEL_EVENT_DATA_RECEIVED
 * event, forwarding the data along an established, outbound pipe stream to the
 * Guacamole client.
 *
 * @param svc
 *     The guac_rdp_svc structure representing the SVC that received the data.
 *
 * @param data
 *     The data that was received.
 *
 * @param length
 *     The number of bytes received.
 */
static void guac_rdp_svc_process_receive(guac_rdp_svc* svc,
        void* data, int length) {

    /* Fail if output not created */
    if (svc->output_pipe == NULL) {
        guac_client_log(svc->client, GUAC_LOG_WARNING, "%i bytes of data "
                "received from within the remote desktop session for SVC "
                "\"%s\" are being dropped because the outbound pipe stream "
                "for that SVC is not yet open. This should NOT happen.",
                length, svc->channel_def.name);
        return;
    }

    /* Send blob */
    guac_protocol_send_blob(svc->client->socket, svc->output_pipe, data, length);
    guac_socket_flush(svc->client->socket);

}

/**
 * Event handler for events which deal with data transmitted over an open SVC.
 * This specific implementation of the event handler currently handles only the
 * CHANNEL_EVENT_DATA_RECEIVED event, delegating actual handling of that event
 * to guac_rdp_svc_process_receive().
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
static VOID guac_rdp_svc_handle_open_event(LPVOID user_param,
        DWORD open_handle, UINT event, LPVOID data, UINT32 data_length,
        UINT32 total_length, UINT32 data_flags) {

    /* Ignore all events except for received data */
    if (event != CHANNEL_EVENT_DATA_RECEIVED)
        return;

    guac_rdp_svc* svc = (guac_rdp_svc*) user_param;

    /* Validate relevant handle matches that of SVC */
    if (open_handle != svc->open_handle) {
        guac_client_log(svc->client, GUAC_LOG_WARNING, "%i bytes of data "
                "received from within the remote desktop session for SVC "
                "\"%s\" are being dropped because the relevant open handle "
                "(0x%X) does not match the open handle of the SVC (0x%X).",
                data_length, svc->channel_def.name, open_handle,
                svc->open_handle);
        return;
    }

    guac_rdp_svc_process_receive(svc, data, data_length);

}

/**
 * Processes a CHANNEL_EVENT_CONNECTED event, completing the
 * connection/initialization process of the SVC.
 *
 * @param svc
 *     The guac_rdp_svc structure representing the SVC that is now connected.
 */
static void guac_rdp_svc_process_connect(guac_rdp_svc* svc) {

    /* Open FreeRDP side of connected channel */
    UINT32 open_status =
        svc->entry_points.pVirtualChannelOpenEx(svc->init_handle,
                &svc->open_handle, svc->channel_def.name,
                guac_rdp_svc_handle_open_event);

    /* Warn if the channel cannot be opened after all */
    if (open_status != CHANNEL_RC_OK) {
        guac_client_log(svc->client, GUAC_LOG_WARNING, "SVC \"%s\" could not "
                "be opened: %s (error %i)", svc->channel_def.name,
                WTSErrorToString(open_status), open_status);
        return;
    }

    /* SVC may now receive data from client */
    guac_rdp_svc_add(svc->client, svc);

    /* Create pipe */
    svc->output_pipe = guac_client_alloc_stream(svc->client);

    /* Notify of pipe's existence */
    guac_rdp_svc_send_pipe(svc->client->socket, svc);

    /* Log connection to static channel */
    guac_client_log(svc->client, GUAC_LOG_INFO,
            "Static channel \"%s\" connected.", svc->channel_def.name);

}

/**
 * Processes a CHANNEL_EVENT_TERMINATED event, freeing all resources associated
 * with the SVC.
 *
 * @param svc
 *     The guac_rdp_svc structure representing the SVC that has been
 *     terminated.
 */
static void guac_rdp_svc_process_terminate(guac_rdp_svc* svc) {

    /* Remove and free SVC */
    guac_client_log(svc->client, GUAC_LOG_INFO, "Closing channel \"%s\"...", svc->channel_def.name);
    guac_rdp_svc_remove(svc->client, svc->channel_def.name);
    free(svc);

}

/**
 * Event handler for events which deal with the overall lifecycle of an SVC.
 * This specific implementation of the event handler currently handles only
 * CHANNEL_EVENT_CONNECTED and CHANNEL_EVENT_TERMINATED events, delegating
 * actual handling of those events to guac_rdp_svc_process_connect() and
 * guac_rdp_svc_process_terminate() respectively.
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
static VOID guac_rdp_svc_handle_init_event(LPVOID user_param,
        LPVOID init_handle, UINT event, LPVOID data, UINT data_length) {

    guac_rdp_svc* svc = (guac_rdp_svc*) user_param;

    /* Validate relevant handle matches that of SVC */
    if (init_handle != svc->init_handle) {
        guac_client_log(svc->client, GUAC_LOG_WARNING, "An init event (#%i) "
                "for SVC \"%s\" has been dropped because the relevant init "
                "handle (0x%X) does not match the init handle of the SVC "
                "(0x%X).", event, svc->channel_def.name, init_handle,
                svc->init_handle);
        return;
    }

    switch (event) {

        /* The remote desktop side of the SVC has been connected */
        case CHANNEL_EVENT_CONNECTED:
            guac_rdp_svc_process_connect(svc);
            break;

        /* The channel has disconnected and now must be cleaned up */
        case CHANNEL_EVENT_TERMINATED:
            guac_rdp_svc_process_terminate(svc);
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

    /* Get structure representing the Guacamole side of the SVC from plugin
     * parameters */
    guac_rdp_svc* svc = (guac_rdp_svc*) entry_points_ex->pExtendedData;

    /* Copy FreeRDP data into SVC structure for future reference */
    svc->entry_points = *entry_points_ex;
    svc->init_handle = init_handle;

    /* Complete initialization */
    if (svc->entry_points.pVirtualChannelInitEx(svc, svc, init_handle,
                &svc->channel_def, 1, VIRTUAL_CHANNEL_VERSION_WIN2000,
                guac_rdp_svc_handle_init_event) != CHANNEL_RC_OK) {
        return FALSE;
    }

    return TRUE;

}

