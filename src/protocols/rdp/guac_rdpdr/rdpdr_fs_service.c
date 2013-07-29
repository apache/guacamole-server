
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is libguac-client-rdp.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

#include <guacamole/pool.h>

#include "rdpdr_fs.h"
#include "rdpdr_fs_messages.h"
#include "rdpdr_messages.h"
#include "rdpdr_service.h"
#include "client.h"
#include "unicode.h"

#include <freerdp/utils/svc_plugin.h>

static void guac_rdpdr_device_fs_announce_handler(guac_rdpdr_device* device,
        wStream* output_stream, int device_id) {

    /* Filesystem header */
    guac_client_log_info(device->rdpdr->client, "Sending filesystem");
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

        case IRP_MJ_DEVICE_CONTROL:
            guac_client_log_error(device->rdpdr->client,
                    "IRP_MJ_DEVICE_CONTROL unsupported");
            break;

        case IRP_MJ_QUERY_VOLUME_INFORMATION:
            guac_rdpdr_fs_volume_info(device, input_stream, completion_id);
            break;

        case IRP_MJ_SET_VOLUME_INFORMATION:
            guac_client_log_error(device->rdpdr->client,
                    "IRP_MJ_SET_VOLUME_INFORMATION unsupported");
            break;

        case IRP_MJ_QUERY_INFORMATION:
            guac_rdpdr_fs_file_info(device, input_stream, file_id, completion_id);
            break;

        case IRP_MJ_SET_INFORMATION:
            guac_client_log_error(device->rdpdr->client,
                    "IRP_MJ_SET_INFORMATION unsupported");
            break;

        case IRP_MJ_DIRECTORY_CONTROL:

            if (minor_func == IRP_MN_QUERY_DIRECTORY)
                guac_client_log_error(device->rdpdr->client,
                        "IRP_MN_QUERY_DIRECTORY unsupported");

            else if (minor_func == IRP_MN_NOTIFY_CHANGE_DIRECTORY)
                guac_client_log_error(device->rdpdr->client,
                        "IRP_MN_NOTIFY_CHANGE_DIRECTORY unsupported");

            break;

        case IRP_MJ_LOCK_CONTROL:
            guac_client_log_error(device->rdpdr->client,
                    "IRP_MJ_LOCK_CONTROL unsupported");
            break;

        default:
            guac_client_log_error(device->rdpdr->client,
                    "Unknown filesystem I/O request function: 0x%x/0x%x",
                    major_func, minor_func);
    }

}

static void guac_rdpdr_device_fs_free_handler(guac_rdpdr_device* device) {
    guac_rdpdr_fs_data* data = (guac_rdpdr_fs_data*) device->data;
    guac_pool_free(data->file_id_pool);
    free(data);
}

void guac_rdpdr_register_fs(guac_rdpdrPlugin* rdpdr) {

    guac_rdpdr_fs_data* data;
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
    data = device->data = malloc(sizeof(guac_rdpdr_fs_data));
    data->file_id_pool  = guac_pool_alloc(0);
    data->open_files    = 0;

}

