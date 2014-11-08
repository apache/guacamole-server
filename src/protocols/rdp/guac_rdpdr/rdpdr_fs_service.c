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
#include "rdpdr_fs_messages.h"
#include "rdpdr_messages.h"
#include "rdpdr_service.h"

#include <freerdp/utils/svc_plugin.h>
#include <guacamole/client.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

static void guac_rdpdr_device_fs_announce_handler(guac_rdpdr_device* device,
        wStream* output_stream, int device_id) {

    /* Filesystem header */
    guac_client_log(device->rdpdr->client, GUAC_LOG_INFO, "Sending filesystem");
    Stream_Write_UINT32(output_stream, RDPDR_DTYP_FILESYSTEM);
    Stream_Write_UINT32(output_stream, device_id);
    Stream_Write(output_stream, "GUAC\0\0\0\0", 8); /* DOS name */

    /* Filesystem data */
    Stream_Write_UINT32(output_stream, GUAC_FILESYSTEM_NAME_LENGTH);
    Stream_Write(output_stream, GUAC_FILESYSTEM_NAME, GUAC_FILESYSTEM_NAME_LENGTH);

}

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
}

void guac_rdpdr_register_fs(guac_rdpdrPlugin* rdpdr) {

    rdp_guac_client_data* data = (rdp_guac_client_data*) rdpdr->client->data;
    int id = rdpdr->devices_registered++;

    /* Get new device */
    guac_rdpdr_device* device = &(rdpdr->devices[id]);

    /* Init device */
    device->rdpdr       = rdpdr;
    device->device_id   = id;
    device->device_name = "Guacamole Filesystem";

    /* Set handlers */
    device->announce_handler  = guac_rdpdr_device_fs_announce_handler;
    device->iorequest_handler = guac_rdpdr_device_fs_iorequest_handler;
    device->free_handler      = guac_rdpdr_device_fs_free_handler;

    /* Init data */
    device->data = data->filesystem;

}

