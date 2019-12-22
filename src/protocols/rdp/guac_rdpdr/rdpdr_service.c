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

#include "rdp.h"
#include "rdp_fs.h"
#include "rdp_settings.h"
#include "rdp_stream.h"
#include "rdpdr_fs_service.h"
#include "rdpdr_messages.h"
#include "rdpdr_printer.h"
#include "rdpdr_service.h"

#include <stdlib.h>
#include <string.h>

#include <freerdp/constants.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>
#include <winpr/stream.h>

/**
 * Processes data received along the RDPDR channel via a
 * CHANNEL_EVENT_DATA_RECEIVED event, forwarding the data along an established,
 * outbound pipe stream to the Guacamole client.
 *
 * @param rdpdr
 *     The guac_rdpdr structure representing the RDPDR channel.
 *
 * @param input_stream
 *     The data that was received.
 */
static void guac_rdpdr_process_receive(guac_rdpdr* rdpdr,
        wStream* input_stream) {

    int component;
    int packet_id;

    /* Read header */
    Stream_Read_UINT16(input_stream, component);
    Stream_Read_UINT16(input_stream, packet_id);

    /* Core component */
    if (component == RDPDR_CTYP_CORE) {

        /* Dispatch handlers based on packet ID */
        switch (packet_id) {

            case PAKID_CORE_SERVER_ANNOUNCE:
                guac_rdpdr_process_server_announce(rdpdr, input_stream);
                break;

            case PAKID_CORE_CLIENTID_CONFIRM:
                guac_rdpdr_process_clientid_confirm(rdpdr, input_stream);
                break;

            case PAKID_CORE_DEVICE_REPLY:
                guac_rdpdr_process_device_reply(rdpdr, input_stream);
                break;

            case PAKID_CORE_DEVICE_IOREQUEST:
                guac_rdpdr_process_device_iorequest(rdpdr, input_stream);
                break;

            case PAKID_CORE_SERVER_CAPABILITY:
                guac_rdpdr_process_server_capability(rdpdr, input_stream);
                break;

            case PAKID_CORE_USER_LOGGEDON:
                guac_rdpdr_process_user_loggedon(rdpdr, input_stream);
                break;

            default:
                guac_client_log(rdpdr->client, GUAC_LOG_INFO, "Ignoring RDPDR core packet with unexpected ID: 0x%04x", packet_id);

        }

    } /* end if core */

    /* Printer component */
    else if (component == RDPDR_CTYP_PRN) {

        /* Dispatch handlers based on packet ID */
        switch (packet_id) {

            case PAKID_PRN_CACHE_DATA:
                guac_rdpdr_process_prn_cache_data(rdpdr, input_stream);
                break;

            case PAKID_PRN_USING_XPS:
                guac_rdpdr_process_prn_using_xps(rdpdr, input_stream);
                break;

            default:
                guac_client_log(rdpdr->client, GUAC_LOG_INFO, "Ignoring RDPDR printer packet with unexpected ID: 0x%04x", packet_id);

        }

    } /* end if printer */

    else
        guac_client_log(rdpdr->client, GUAC_LOG_INFO, "Ignoring packet for unknown RDPDR component: 0x%04x", component);

}

wStream* guac_rdpdr_new_io_completion(guac_rdpdr_device* device,
        int completion_id, int status, int size) {

    wStream* output_stream = Stream_New(NULL, 16+size);

    /* Write header */
    Stream_Write_UINT16(output_stream, RDPDR_CTYP_CORE);
    Stream_Write_UINT16(output_stream, PAKID_CORE_DEVICE_IOCOMPLETION);

    /* Write content */
    Stream_Write_UINT32(output_stream, device->device_id);
    Stream_Write_UINT32(output_stream, completion_id);
    Stream_Write_UINT32(output_stream, status);

    return output_stream;

}

/**
 * Callback invoked on the current connection owner (if any) when a file
 * download is being initiated using the magic "Download" folder.
 *
 * @param owner
 *     The guac_user that is the owner of the connection, or NULL if the
 *     connection owner has left.
 *
 * @param data
 *     The full absolute path to the file that should be downloaded.
 *
 * @return
 *     The stream allocated for the file download, or NULL if the download has
 *     failed to start.
 */
static void* guac_rdpdr_download_to_owner(guac_user* owner, void* data) {

    /* Do not bother attempting the download if the owner has left */
    if (owner == NULL)
        return NULL;

    guac_client* client = owner->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_rdp_fs* filesystem = rdp_client->filesystem;

    /* Ignore download if filesystem has been unloaded */
    if (filesystem == NULL)
        return NULL;

    /* Attempt to open requested file */
    char* path = (char*) data;
    int file_id = guac_rdp_fs_open(filesystem, path,
            ACCESS_FILE_READ_DATA, 0, DISP_FILE_OPEN, 0);

    /* If file opened successfully, start stream */
    if (file_id >= 0) {

        guac_rdp_stream* rdp_stream;
        const char* basename;

        int i;
        char c;

        /* Associate stream with transfer status */
        guac_stream* stream = guac_user_alloc_stream(owner);
        stream->data = rdp_stream = malloc(sizeof(guac_rdp_stream));
        stream->ack_handler = guac_rdp_download_ack_handler;
        rdp_stream->type = GUAC_RDP_DOWNLOAD_STREAM;
        rdp_stream->download_status.file_id = file_id;
        rdp_stream->download_status.offset = 0;

        /* Get basename from absolute path */
        i=0;
        basename = path;
        do {

            c = path[i];
            if (c == '/' || c == '\\')
                basename = &(path[i+1]);

            i++;

        } while (c != '\0');

        guac_user_log(owner, GUAC_LOG_DEBUG, "%s: Initiating download "
                "of \"%s\"", __func__, path);

        /* Begin stream */
        guac_protocol_send_file(owner->socket, stream,
                "application/octet-stream", basename);
        guac_socket_flush(owner->socket);

        /* Download started successfully */
        return stream;

    }

    /* Download failed */
    guac_user_log(owner, GUAC_LOG_ERROR, "Unable to download \"%s\"", path);
    return NULL;

}

void guac_rdpdr_start_download(guac_rdpdr_device* device, char* path) {

    guac_client* client = device->rdpdr->client;

    /* Initiate download to the owner of the connection */
    guac_client_for_owner(client, guac_rdpdr_download_to_owner, path);

}

/**
 * Event handler for events which deal with data transmitted over the RDPDR
 * channel.  This specific implementation of the event handler currently
 * handles only the CHANNEL_EVENT_DATA_RECEIVED event, delegating actual
 * handling of that event to guac_rdpdr_process_receive().
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
static VOID guac_rdpdr_handle_open_event(LPVOID user_param,
        DWORD open_handle, UINT event, LPVOID data, UINT32 data_length,
        UINT32 total_length, UINT32 data_flags) {

    /* Ignore all events except for received data */
    if (event != CHANNEL_EVENT_DATA_RECEIVED)
        return;

    guac_rdpdr* rdpdr = (guac_rdpdr*) user_param;

    /* Validate relevant handle matches that of the RDPDR channel */
    if (open_handle != rdpdr->open_handle) {
        guac_client_log(rdpdr->client, GUAC_LOG_WARNING, "%i bytes of data "
                "received from within the remote desktop session for the "
                "RDPDR channel are being dropped because the relevant open "
                "handle (0x%X) does not match the open handle of RDPDR "
                "(0x%X).", data_length, rdpdr->channel_def.name, open_handle,
                rdpdr->open_handle);
        return;
    }

    wStream* input_stream = Stream_New(data, data_length);
    guac_rdpdr_process_receive(rdpdr, input_stream);
    Stream_Free(input_stream, FALSE);

}

/**
 * Processes a CHANNEL_EVENT_CONNECTED event, completing the
 * connection/initialization process of the RDPDR channel.
 *
 * @param rdpdr
 *     The guac_rdpdr structure representing the RDPDR channel.
 */
static void guac_rdpdr_process_connect(guac_rdpdr* rdpdr) {

    /* Get data from client */
    guac_client* client = rdpdr->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Open FreeRDP side of connected channel */
    UINT32 open_status =
        rdpdr->entry_points.pVirtualChannelOpenEx(rdpdr->init_handle,
                &rdpdr->open_handle, rdpdr->channel_def.name,
                guac_rdpdr_handle_open_event);

    /* Warn if the channel cannot be opened after all */
    if (open_status != CHANNEL_RC_OK) {
        guac_client_log(client, GUAC_LOG_WARNING, "RDPDR channel could not be "
                "opened: %s (error %i)", WTSErrorToString(open_status),
                open_status);
        return;
    }

    /* Register printer if enabled */
    if (rdp_client->settings->printing_enabled)
        guac_rdpdr_register_printer(rdpdr, rdp_client->settings->printer_name);

    /* Register drive if enabled */
    if (rdp_client->settings->drive_enabled)
        guac_rdpdr_register_fs(rdpdr, rdp_client->settings->drive_name);

    /* Log that printing, etc. has been loaded */
    guac_client_log(client, GUAC_LOG_INFO, "RDPDR channel connected.");

}

/**
 * Processes a CHANNEL_EVENT_TERMINATED event, freeing all resources associated
 * with the RDPDR channel.
 *
 * @param rdpdr
 *     The guac_rdpdr structure representing the RDPDR channel.
 */
static void guac_rdpdr_process_terminate(guac_rdpdr* rdpdr) {

    int i;

    for (i=0; i<rdpdr->devices_registered; i++) {
        guac_rdpdr_device* device = &(rdpdr->devices[i]);
        guac_client_log(rdpdr->client, GUAC_LOG_INFO, "Unloading device %i (%s)",
                device->device_id, device->device_name);
        device->free_handler(device);
    }

    guac_client_log(rdpdr->client, GUAC_LOG_INFO, "RDPDR channel disconnected.");
    free(rdpdr);

}

/**
 * Event handler for events which deal with the overall lifecycle of the RDPDR
 * channel.  This specific implementation of the event handler currently
 * handles only CHANNEL_EVENT_CONNECTED and CHANNEL_EVENT_TERMINATED events,
 * delegating actual handling of those events to guac_rdpdr_process_connect()
 * and guac_rdpdr_process_terminate() respectively.
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
static VOID guac_rdpdr_handle_init_event(LPVOID user_param,
        LPVOID init_handle, UINT event, LPVOID data, UINT data_length) {

    guac_rdpdr* rdpdr = (guac_rdpdr*) user_param;

    /* Validate relevant handle matches that of the RDPDR channel */
    if (init_handle != rdpdr->init_handle) {
        guac_client_log(rdpdr->client, GUAC_LOG_WARNING, "An init event "
                "(#%i) for the RDPDR channel has been dropped because the "
                "relevant init handle (0x%X) does not match the init handle "
                "of the RDPDR channel (0x%X).", event, init_handle,
                rdpdr->init_handle);
        return;
    }

    switch (event) {

        /* The RDPDR channel has been connected */
        case CHANNEL_EVENT_CONNECTED:
            guac_rdpdr_process_connect(rdpdr);
            break;

        /* The RDPDR channel has disconnected and now must be cleaned up */
        case CHANNEL_EVENT_TERMINATED:
            guac_rdpdr_process_terminate(rdpdr);
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
    guac_rdpdr* rdpdr = (guac_rdpdr*) calloc(1, sizeof(guac_rdpdr));

    /* Init channel def */
    strcpy(rdpdr->channel_def.name, "rdpdr");
    rdpdr->channel_def.options = CHANNEL_OPTION_INITIALIZED
        | CHANNEL_OPTION_ENCRYPT_RDP
        | CHANNEL_OPTION_COMPRESS_RDP;

    /* Maintain reference to associated guac_client */
    rdpdr->client = (guac_client*) entry_points_ex->pExtendedData;

    /* No devices are connected initially */
    rdpdr->devices_registered = 0;

    /* Copy FreeRDP data into RDPSND structure for future reference */
    rdpdr->entry_points = *entry_points_ex;
    rdpdr->init_handle = init_handle;

    /* Complete initialization */
    if (rdpdr->entry_points.pVirtualChannelInitEx(rdpdr, rdpdr, init_handle,
                &rdpdr->channel_def, 1, VIRTUAL_CHANNEL_VERSION_WIN2000,
                guac_rdpdr_handle_init_event) != CHANNEL_RC_OK) {
        return FALSE;
    }

    return TRUE;

}

