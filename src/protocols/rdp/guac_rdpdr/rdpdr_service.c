/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"

#include "client.h"
#include "debug.h"
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
    rdp_guac_client_data* client_data = (rdp_guac_client_data*) client->data;

    /* Init plugin */
    rdpdr->client = client;
    rdpdr->devices_registered = 0;

    /* Register printer if enabled */
    if (client_data->settings.printing_enabled)
        guac_rdpdr_register_printer(rdpdr);

    /* Register drive if enabled */
    if (client_data->settings.drive_enabled)
        guac_rdpdr_register_fs(rdpdr);

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

void guac_rdpdr_start_download(guac_rdpdr_device* device, const char* path) {

    /* Get client and stream */
    guac_client* client = device->rdpdr->client;

    int file_id = guac_rdp_fs_open((guac_rdp_fs*) device->data, path,
            ACCESS_FILE_READ_DATA, 0, DISP_FILE_OPEN, 0);

    /* If file opened successfully, start stream */
    if (file_id >= 0) {

        guac_rdp_stream* rdp_stream;
        const char* basename;

        int i;
        char c;

        /* Associate stream with transfer status */
        guac_stream* stream = guac_client_alloc_stream(client);
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

        GUAC_RDP_DEBUG(2, "Initiating download of \"%s\"", path);

        /* Begin stream */
        guac_protocol_send_file(client->socket, stream,
                "application/octet-stream", basename);
        guac_socket_flush(client->socket);

    }
    else
        guac_client_log(client, GUAC_LOG_ERROR, "Unable to download \"%s\"", path);

}

