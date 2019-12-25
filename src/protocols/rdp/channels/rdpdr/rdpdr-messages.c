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
#include "channels/rdpdr/rdpdr-messages.h"
#include "channels/rdpdr/rdpdr.h"
#include "rdp.h"
#include "unicode.h"

#include <freerdp/channels/rdpdr.h>
#include <guacamole/client.h>
#include <guacamole/unicode.h>
#include <winpr/stream.h>

#include <stdlib.h>
#include <string.h>

static void guac_rdpdr_send_client_announce_reply(guac_rdp_common_svc* svc,
        unsigned int major, unsigned int minor, unsigned int client_id) {

    wStream* output_stream = Stream_New(NULL, 12);

    /* Write header */
    Stream_Write_UINT16(output_stream, RDPDR_CTYP_CORE);
    Stream_Write_UINT16(output_stream, PAKID_CORE_CLIENTID_CONFIRM);

    /* Write content */
    Stream_Write_UINT16(output_stream, major);
    Stream_Write_UINT16(output_stream, minor);
    Stream_Write_UINT32(output_stream, client_id);

    guac_rdp_common_svc_write(svc, output_stream);

}

static void guac_rdpdr_send_client_name_request(guac_rdp_common_svc* svc,
        const char* name) {

    int name_bytes = strlen(name) + 1;
    wStream* output_stream = Stream_New(NULL, 16 + name_bytes);

    /* Write header */
    Stream_Write_UINT16(output_stream, RDPDR_CTYP_CORE);
    Stream_Write_UINT16(output_stream, PAKID_CORE_CLIENT_NAME);

    /* Write content */
    Stream_Write_UINT32(output_stream, 0); /* ASCII */
    Stream_Write_UINT32(output_stream, 0); /* 0 required by RDPDR spec */
    Stream_Write_UINT32(output_stream, name_bytes);
    Stream_Write(output_stream, name, name_bytes);

    guac_rdp_common_svc_write(svc, output_stream);

}

static void guac_rdpdr_send_client_capability(guac_rdp_common_svc* svc) {

    wStream* output_stream = Stream_New(NULL, 256);
    guac_client_log(svc->client, GUAC_LOG_INFO, "Sending capabilities...");

    /* Write header */
    Stream_Write_UINT16(output_stream, RDPDR_CTYP_CORE);
    Stream_Write_UINT16(output_stream, PAKID_CORE_CLIENT_CAPABILITY);

    /* Capability count + padding */
    Stream_Write_UINT16(output_stream, 3);
    Stream_Write_UINT16(output_stream, 0); /* Padding */

    /* General capability header */
    Stream_Write_UINT16(output_stream, CAP_GENERAL_TYPE);
    Stream_Write_UINT16(output_stream, 44);
    Stream_Write_UINT32(output_stream, GENERAL_CAPABILITY_VERSION_02);

    /* General capability data */
    Stream_Write_UINT32(output_stream, GUAC_OS_TYPE);                /* osType - required to be ignored */
    Stream_Write_UINT32(output_stream, 0);                           /* osVersion */
    Stream_Write_UINT16(output_stream, 1);                           /* protocolMajor - must be set to 1 */
    Stream_Write_UINT16(output_stream, RDPDR_MINOR_RDP_VERSION_5_2); /* protocolMinor */
    Stream_Write_UINT32(output_stream, 0xFFFF);                      /* ioCode1 */
    Stream_Write_UINT32(output_stream, 0);                           /* ioCode2 */
    Stream_Write_UINT32(output_stream,
                                      RDPDR_DEVICE_REMOVE_PDUS
                                    | RDPDR_CLIENT_DISPLAY_NAME_PDU
                                    | RDPDR_USER_LOGGEDON_PDU); /* extendedPDU */
    Stream_Write_UINT32(output_stream, 0);                      /* extraFlags1 */
    Stream_Write_UINT32(output_stream, 0);                      /* extraFlags2 */
    Stream_Write_UINT32(output_stream, 0);                      /* SpecialTypeDeviceCap */

    /* Printer support header */
    Stream_Write_UINT16(output_stream, CAP_PRINTER_TYPE);
    Stream_Write_UINT16(output_stream, 8);
    Stream_Write_UINT32(output_stream, PRINT_CAPABILITY_VERSION_01);

    /* Drive support header */
    Stream_Write_UINT16(output_stream, CAP_DRIVE_TYPE);
    Stream_Write_UINT16(output_stream, 8);
    Stream_Write_UINT32(output_stream, DRIVE_CAPABILITY_VERSION_02);

    guac_rdp_common_svc_write(svc, output_stream);
    guac_client_log(svc->client, GUAC_LOG_INFO, "Capabilities sent.");

}

static void guac_rdpdr_send_client_device_list_announce_request(guac_rdp_common_svc* svc) {

    guac_rdpdr* rdpdr = (guac_rdpdr*) svc->data;

    /* Calculate number of bytes needed for the stream */
    int streamBytes = 16;
    for (int i=0; i < rdpdr->devices_registered; i++)
        streamBytes += rdpdr->devices[i].device_announce_len;

    /* Allocate the stream */
    wStream* output_stream = Stream_New(NULL, streamBytes);

    /* Write header */
    Stream_Write_UINT16(output_stream, RDPDR_CTYP_CORE);
    Stream_Write_UINT16(output_stream, PAKID_CORE_DEVICELIST_ANNOUNCE);

    /* Get the stream for each of the devices. */
    Stream_Write_UINT32(output_stream, rdpdr->devices_registered);
    for (int i=0; i<rdpdr->devices_registered; i++) {
        
        Stream_Write(output_stream,
                Stream_Buffer(rdpdr->devices[i].device_announce),
                rdpdr->devices[i].device_announce_len);

        guac_client_log(svc->client, GUAC_LOG_INFO, "Registered device %i (%s)",
                rdpdr->devices[i].device_id, rdpdr->devices[i].device_name);
        
    }

    guac_rdp_common_svc_write(svc, output_stream);
    guac_client_log(svc->client, GUAC_LOG_INFO, "All supported devices sent.");

}

void guac_rdpdr_process_server_announce(guac_rdp_common_svc* svc,
        wStream* input_stream) {

    unsigned int major, minor, client_id;

    Stream_Read_UINT16(input_stream, major);
    Stream_Read_UINT16(input_stream, minor);
    Stream_Read_UINT32(input_stream, client_id);

    /* Must choose own client ID if minor not >= 12 */
    if (minor < 12)
        client_id = random() & 0xFFFF;

    guac_client_log(svc->client, GUAC_LOG_INFO, "Connected to RDPDR %u.%u as client 0x%04x", major, minor, client_id);

    /* Respond to announce */
    guac_rdpdr_send_client_announce_reply(svc, major, minor, client_id);

    /* Name request */
    guac_rdpdr_send_client_name_request(svc, ((guac_rdp_client*) svc->client->data)->settings->client_name);

}

void guac_rdpdr_process_clientid_confirm(guac_rdp_common_svc* svc,
        wStream* input_stream) {
    guac_client_log(svc->client, GUAC_LOG_INFO, "Client ID confirmed");
}

void guac_rdpdr_process_device_reply(guac_rdp_common_svc* svc,
        wStream* input_stream) {

    guac_rdpdr* rdpdr = (guac_rdpdr*) svc->data;

    unsigned int device_id, ntstatus;
    int severity, c, n, facility, code;

    Stream_Read_UINT32(input_stream, device_id);
    Stream_Read_UINT32(input_stream, ntstatus);

    severity = (ntstatus & 0xC0000000) >> 30;
    c        = (ntstatus & 0x20000000) >> 29;
    n        = (ntstatus & 0x10000000) >> 28;
    facility = (ntstatus & 0x0FFF0000) >> 16;
    code     =  ntstatus & 0x0000FFFF;

    /* Log error / information */
    if (device_id < rdpdr->devices_registered) {

        if (severity == 0x0)
            guac_client_log(svc->client, GUAC_LOG_INFO, "Device %i (%s) connected successfully",
                    device_id, rdpdr->devices[device_id].device_name);

        else
            guac_client_log(svc->client, GUAC_LOG_ERROR, "Problem connecting device %i (%s): "
                    "severity=0x%x, c=0x%x, n=0x%x, facility=0x%x, code=0x%x",
                     device_id, rdpdr->devices[device_id].device_name,
                     severity,      c,      n,      facility,      code);

    }

    else
        guac_client_log(svc->client, GUAC_LOG_ERROR, "Unknown device ID: 0x%08x", device_id);

}

void guac_rdpdr_process_device_iorequest(guac_rdp_common_svc* svc,
        wStream* input_stream) {

    guac_rdpdr* rdpdr = (guac_rdpdr*) svc->data;

    int device_id, file_id, completion_id, major_func, minor_func;

    /* Read header */
    Stream_Read_UINT32(input_stream, device_id);
    Stream_Read_UINT32(input_stream, file_id);
    Stream_Read_UINT32(input_stream, completion_id);
    Stream_Read_UINT32(input_stream, major_func);
    Stream_Read_UINT32(input_stream, minor_func);

    /* If printer, run printer handlers */
    if (device_id >= 0 && device_id < rdpdr->devices_registered) {

        /* Call handler on device */
        guac_rdpdr_device* device = &(rdpdr->devices[device_id]);
        device->iorequest_handler(svc, device, input_stream,
                file_id, completion_id, major_func, minor_func);

    }

    else
        guac_client_log(svc->client, GUAC_LOG_ERROR, "Unknown device ID: 0x%08x", device_id);

}

void guac_rdpdr_process_server_capability(guac_rdp_common_svc* svc,
        wStream* input_stream) {

    int count;
    int i;

    /* Read header */
    Stream_Read_UINT16(input_stream, count);
    Stream_Seek(input_stream, 2);

    /* Parse capabilities */
    for (i=0; i<count; i++) {

        int type;
        int length;

        Stream_Read_UINT16(input_stream, type);
        Stream_Read_UINT16(input_stream, length);

        /* Ignore all for now */
        guac_client_log(svc->client, GUAC_LOG_INFO, "Ignoring server capability set type=0x%04x, length=%i", type, length);
        Stream_Seek(input_stream, length - 4);

    }

    /* Send own capabilities */
    guac_rdpdr_send_client_capability(svc);

}

void guac_rdpdr_process_user_loggedon(guac_rdp_common_svc* svc,
        wStream* input_stream) {

    guac_client_log(svc->client, GUAC_LOG_INFO, "User logged on");
    guac_rdpdr_send_client_device_list_announce_request(svc);

}

void guac_rdpdr_process_prn_cache_data(guac_rdp_common_svc* svc,
        wStream* input_stream) {
    guac_client_log(svc->client, GUAC_LOG_INFO, "Ignoring printer cached configuration data");
}

void guac_rdpdr_process_prn_using_xps(guac_rdp_common_svc* svc,
        wStream* input_stream) {
    guac_client_log(svc->client, GUAC_LOG_INFO, "Printer unexpectedly switched to XPS mode");
}