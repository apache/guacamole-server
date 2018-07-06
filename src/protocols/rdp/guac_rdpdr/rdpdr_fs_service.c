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
#include "rdpdr_fs_messages.h"
#include "rdpdr_messages.h"
#include "rdpdr_service.h"

#include <freerdp/utils/svc_plugin.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/unicode.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

static void guac_rdpdr_device_fs_iorequest_handler(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id, int major_func, int minor_func) {

    switch (major_func) {

        /* File open */
        case IRP_MJ_CREATE:
            guac_rdpdr_fs_process_create(device, input_stream, completion_id);
            break;

        /* File close */
        case IRP_MJ_CLOSE:
            guac_rdpdr_fs_process_close(device, input_stream, file_id, completion_id);
            break;

        /* File read */
        case IRP_MJ_READ:
            guac_rdpdr_fs_process_read(device, input_stream, file_id, completion_id);
            break;

        /* File write */
        case IRP_MJ_WRITE:
            guac_rdpdr_fs_process_write(device, input_stream, file_id, completion_id);
            break;

        /* Device control request (Windows FSCTL_ control codes) */
        case IRP_MJ_DEVICE_CONTROL:
            guac_rdpdr_fs_process_device_control(device, input_stream, file_id, completion_id);
            break;

        /* Query volume (drive) information */
        case IRP_MJ_QUERY_VOLUME_INFORMATION:
            guac_rdpdr_fs_process_volume_info(device, input_stream, file_id, completion_id);
            break;

        /* Set volume (drive) information */
        case IRP_MJ_SET_VOLUME_INFORMATION:
            guac_rdpdr_fs_process_set_volume_info(device, input_stream, file_id, completion_id);
            break;

        /* Query file information */
        case IRP_MJ_QUERY_INFORMATION:
            guac_rdpdr_fs_process_file_info(device, input_stream, file_id, completion_id);
            break;

        /* Set file information */
        case IRP_MJ_SET_INFORMATION:
            guac_rdpdr_fs_process_set_file_info(device, input_stream, file_id, completion_id);
            break;

        case IRP_MJ_DIRECTORY_CONTROL:

            /* Enumerate directory contents */
            if (minor_func == IRP_MN_QUERY_DIRECTORY)
                guac_rdpdr_fs_process_query_directory(device, input_stream, file_id, completion_id);

            /* Request notification of changes to directory */
            else if (minor_func == IRP_MN_NOTIFY_CHANGE_DIRECTORY)
                guac_rdpdr_fs_process_notify_change_directory(device, input_stream,
                        file_id, completion_id);

            break;

        /* Lock/unlock portions of a file */
        case IRP_MJ_LOCK_CONTROL:
            guac_rdpdr_fs_process_lock_control(device, input_stream, file_id, completion_id);
            break;

        default:
            guac_client_log(device->rdpdr->client, GUAC_LOG_ERROR,
                    "Unknown filesystem I/O request function: 0x%x/0x%x",
                    major_func, minor_func);
    }

}

static void guac_rdpdr_device_fs_free_handler(guac_rdpdr_device* device) {

    Stream_Free(device->device_announce, 1);
    
}

void guac_rdpdr_register_fs(guac_rdpdrPlugin* rdpdr, char* drive_name) {

    guac_client* client = rdpdr->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    int id = rdpdr->devices_registered++;

    /* Get new device */
    guac_rdpdr_device* device = &(rdpdr->devices[id]);

    /* Init device */
    device->rdpdr       = rdpdr;
    device->device_id   = id;
    device->device_name = drive_name;
    int device_name_len = guac_utf8_strlen(device->device_name);
    device->device_type = RDPDR_DTYP_FILESYSTEM;
    device->dos_name = "GUACFS\0\0";

    /* Set up the device announcement */
    device->device_announce_len = 20 + device_name_len;
    device->device_announce = Stream_New(NULL, device->device_announce_len);
    Stream_Write_UINT32(device->device_announce, device->device_type);
    Stream_Write_UINT32(device->device_announce, device->device_id);
    Stream_Write(device->device_announce, device->dos_name, 8);
    Stream_Write_UINT32(device->device_announce, device_name_len);
    Stream_Write(device->device_announce, device->device_name, device_name_len);
    

    /* Set handlers */
    device->iorequest_handler = guac_rdpdr_device_fs_iorequest_handler;
    device->free_handler      = guac_rdpdr_device_fs_free_handler;

    /* Init data */
    device->data = rdp_client->filesystem;

}

