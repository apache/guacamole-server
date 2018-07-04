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

#include "rdpdr_messages.h"
#include "rdpdr_printer.h"
#include "rdpdr_service.h"
#include "rdp.h"
#include "rdp_print_job.h"
#include "rdp_status.h"
#include "unicode.h"

#include <freerdp/utils/svc_plugin.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>
#include <guacamole/unicode.h>
#include <guacamole/user.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void guac_rdpdr_process_print_job_create(guac_rdpdr_device* device,
        wStream* input_stream, int completion_id) {

    guac_client* client = device->rdpdr->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Log creation of print job */
    guac_client_log(client, GUAC_LOG_INFO, "Print job created");

    /* Create print job */
    rdp_client->active_job = guac_client_for_owner(client,
            guac_rdp_print_job_alloc, NULL);

    /* Respond with success */
    wStream* output_stream = guac_rdpdr_new_io_completion(device,
            completion_id, STATUS_SUCCESS, 4);

    Stream_Write_UINT32(output_stream, 0); /* fileId */
    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_process_print_job_write(guac_rdpdr_device* device,
        wStream* input_stream, int completion_id) {

    guac_client* client = device->rdpdr->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_rdp_print_job* job = (guac_rdp_print_job*) rdp_client->active_job;

    unsigned char* buffer;
    int length;
    int status;

    /* Read buffer of print data */
    Stream_Read_UINT32(input_stream, length);
    Stream_Seek(input_stream, 8);  /* Offset */
    Stream_Seek(input_stream, 20); /* Padding */
    buffer = Stream_Pointer(input_stream);

    /* Write data only if job exists, translating status for RDP */
    if (job != NULL && (length = guac_rdp_print_job_write(job,
                    buffer, length)) >= 0) {
        status = STATUS_SUCCESS;
    }

    /* Report device offline if write fails */
    else {
        status = STATUS_DEVICE_OFF_LINE;
        length = 0;
    }

    wStream* output_stream = guac_rdpdr_new_io_completion(device,
            completion_id, status, 5);

    Stream_Write_UINT32(output_stream, length);
    Stream_Write_UINT8(output_stream, 0); /* Padding */

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_process_print_job_close(guac_rdpdr_device* device,
        wStream* input_stream, int completion_id) {

    guac_client* client = device->rdpdr->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_rdp_print_job* job = (guac_rdp_print_job*) rdp_client->active_job;

    /* End print job */
    if (job != NULL) {
        guac_rdp_print_job_free(job);
        rdp_client->active_job = NULL;
    }

    wStream* output_stream = guac_rdpdr_new_io_completion(device,
            completion_id, STATUS_SUCCESS, 4);

    Stream_Write_UINT32(output_stream, 0); /* Padding */
    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

    /* Log end of print job */
    guac_client_log(client, GUAC_LOG_INFO, "Print job closed");

}

static void guac_rdpdr_device_printer_iorequest_handler(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id, int major_func, int minor_func) {

    switch (major_func) {

        /* Print job create */
        case IRP_MJ_CREATE:
            guac_rdpdr_process_print_job_create(device, input_stream, completion_id);
            break;

        /* Printer job write */
        case IRP_MJ_WRITE:
            guac_rdpdr_process_print_job_write(device, input_stream, completion_id);
            break;

        /* Printer job close */
        case IRP_MJ_CLOSE:
            guac_rdpdr_process_print_job_close(device, input_stream, completion_id);
            break;

        /* Log unknown */
        default:
            guac_client_log(device->rdpdr->client, GUAC_LOG_ERROR,
                    "Unknown printer I/O request function: 0x%x/0x%x",
                    major_func, minor_func);

    }

}

static void guac_rdpdr_device_printer_free_handler(guac_rdpdr_device* device) {

    Stream_Free(device->device_announce, 1);

}

void guac_rdpdr_register_printer(guac_rdpdrPlugin* rdpdr, char* printer_name) {

    int id = rdpdr->devices_registered++;

    /* Get new device */
    guac_rdpdr_device* device = &(rdpdr->devices[id]);

    /* Init device */
    device->rdpdr       = rdpdr;
    device->device_id   = id;
    device->device_name = printer_name;
    int device_name_len = guac_utf8_strlen(device->device_name);
    device->device_type = RDPDR_DTYP_PRINT;
    device->dos_name = "PRN1\0\0\0\0";
    
    /* Set up device announce stream */
    int prt_name_len = (device_name_len + 1) * 2;
    device->device_announce_len = 44 + prt_name_len
            + GUAC_PRINTER_DRIVER_LENGTH;
    device->device_announce = Stream_New(NULL, device->device_announce_len);
    
    /* Write common information. */
    Stream_Write_UINT32(device->device_announce, device->device_type);
    Stream_Write_UINT32(device->device_announce, device->device_id);
    Stream_Write(device->device_announce, device->dos_name, 8);
    
    /* DeviceDataLength */
    Stream_Write_UINT32(device->device_announce, 24 +  prt_name_len + GUAC_PRINTER_DRIVER_LENGTH);
    
    /* Begin printer-specific information */
    Stream_Write_UINT32(device->device_announce,
              RDPDR_PRINTER_ANNOUNCE_FLAG_DEFAULTPRINTER
            | RDPDR_PRINTER_ANNOUNCE_FLAG_NETWORKPRINTER); /* Printer flags */
    Stream_Write_UINT32(device->device_announce, 0); /* Reserved - must be 0. */
    Stream_Write_UINT32(device->device_announce, 0); /* PnPName Length - ignored. */
    Stream_Write_UINT32(device->device_announce, GUAC_PRINTER_DRIVER_LENGTH);
    Stream_Write_UINT32(device->device_announce, prt_name_len);
    Stream_Write_UINT32(device->device_announce, 0); /* CachedFields length. */
    
    Stream_Write(device->device_announce, GUAC_PRINTER_DRIVER, GUAC_PRINTER_DRIVER_LENGTH);
    guac_rdp_utf8_to_utf16((const unsigned char*) device->device_name,
            device_name_len + 1, (char*) Stream_Pointer(device->device_announce),
            prt_name_len);
    Stream_Seek(device->device_announce, prt_name_len);

    /* Set handlers */
    device->iorequest_handler = guac_rdpdr_device_printer_iorequest_handler;
    device->free_handler      = guac_rdpdr_device_printer_free_handler;

}

