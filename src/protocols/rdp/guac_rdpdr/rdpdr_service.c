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
#include <freerdp/utils/svc_plugin.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

/**
 * Entry point for RDPDR virtual channel.
 */
int VirtualChannelEntry(PCHANNEL_ENTRY_POINTS pEntryPoints) {

    /* Allocate plugin */
    guac_rdpdrPlugin* rdpdr =
        (guac_rdpdrPlugin*) calloc(1, sizeof(guac_rdpdrPlugin));

    /* Init channel def */
    strcpy(rdpdr->plugin.channel_def.name, "rdpdr");
    rdpdr->plugin.channel_def.options = 
        CHANNEL_OPTION_INITIALIZED | CHANNEL_OPTION_ENCRYPT_RDP | CHANNEL_OPTION_COMPRESS_RDP;

    /* Set callbacks */
    rdpdr->plugin.connect_callback   = guac_rdpdr_process_connect;
    rdpdr->plugin.receive_callback   = guac_rdpdr_process_receive;
    rdpdr->plugin.event_callback     = guac_rdpdr_process_event;
    rdpdr->plugin.terminate_callback = guac_rdpdr_process_terminate;

    /* Finish init */
    svc_plugin_init((rdpSvcPlugin*) rdpdr, pEntryPoints);
    return 1;

}

/* 
 * Service Handlers
 */

void guac_rdpdr_process_connect(rdpSvcPlugin* plugin) {

    /* Get RDPDR plugin */
    guac_rdpdrPlugin* rdpdr = (guac_rdpdrPlugin*) plugin;

    /* Get client from plugin parameters */
    guac_client* client = (guac_client*)
        plugin->channel_entry_points.pExtendedData;

    /* NULL out pExtendedData so we don't lose our guac_client due to an
     * automatic free() within libfreerdp */
    plugin->channel_entry_points.pExtendedData = NULL;

    /* Get data from client */
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Init plugin */
    rdpdr->client = client;
    rdpdr->devices_registered = 0;

    /* Register printer if enabled */
    if (rdp_client->settings->printing_enabled)
        guac_rdpdr_register_printer(rdpdr, rdp_client->settings->printer_name);

    /* Register drive if enabled */
    if (rdp_client->settings->drive_enabled)
        guac_rdpdr_register_fs(rdpdr, rdp_client->settings->drive_name);

    /* Log that printing, etc. has been loaded */
    guac_client_log(client, GUAC_LOG_INFO, "guacdr connected.");

}

void guac_rdpdr_process_terminate(rdpSvcPlugin* plugin) {

    guac_rdpdrPlugin* rdpdr = (guac_rdpdrPlugin*) plugin;
    int i;

    for (i=0; i<rdpdr->devices_registered; i++) {
        guac_rdpdr_device* device = &(rdpdr->devices[i]);
        guac_client_log(rdpdr->client, GUAC_LOG_INFO, "Unloading device %i (%s)",
                device->device_id, device->device_name);
        device->free_handler(device);
    }

    free(plugin);
}

void guac_rdpdr_process_event(rdpSvcPlugin* plugin, wMessage* event) {
    freerdp_event_free(event);
}

void guac_rdpdr_process_receive(rdpSvcPlugin* plugin,
        wStream* input_stream) {

    guac_rdpdrPlugin* rdpdr = (guac_rdpdrPlugin*) plugin;

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

