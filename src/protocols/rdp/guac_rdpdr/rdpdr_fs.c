
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

#include "rdpdr_messages.h"
#include "rdpdr_fs.h"
#include "rdpdr_service.h"
#include "client.h"
#include "unicode.h"

#include <freerdp/utils/svc_plugin.h>


static void guac_rdpdr_fs_process_create(guac_rdpdr_device* device,
        wStream* input_stream, int completion_id) {

    wStream* output_stream = Stream_New(NULL, 21);
    int file_id;

    int desired_access, file_attributes, shared_access;
    int create_disposition, create_options, path_length;
    char path[1024];

    /* Read "create" information */
    Stream_Read_UINT32(input_stream, desired_access);
    Stream_Seek_UINT64(input_stream); /* allocation size */
    Stream_Read_UINT32(input_stream, file_attributes);
    Stream_Read_UINT32(input_stream, shared_access);
    Stream_Read_UINT32(input_stream, create_disposition);
    Stream_Read_UINT32(input_stream, create_options);
    Stream_Read_UINT32(input_stream, path_length);

    /* Convert path to UTF-8 */
    guac_rdp_utf16_to_utf8(Stream_Pointer(input_stream), path, path_length/2 - 1);

    /* Open file */
    file_id = guac_rdpdr_fs_open(device, path);

    /* Write header */
    Stream_Write_UINT16(output_stream, RDPDR_CTYP_CORE);
    Stream_Write_UINT16(output_stream, PAKID_CORE_DEVICE_IOCOMPLETION);

    /* Write content */
    Stream_Write_UINT32(output_stream, device->device_id);
    Stream_Write_UINT32(output_stream, completion_id);

    /* If no file IDs available, notify server */
    if (file_id == -1) {
        guac_client_log_error(device->rdpdr->client, "File open refused - too many open files");
        Stream_Write_UINT32(output_stream, STATUS_TOO_MANY_OPENED_FILES);
        Stream_Write_UINT32(output_stream, 0); /* fileId */
        Stream_Write_UINT8(output_stream,  0); /* information */
    }
    else if (file_id == -2) {
        guac_client_log_error(device->rdpdr->client,
                "File open refused - does not exist: \"%s\"", path);
        Stream_Write_UINT32(output_stream, STATUS_NO_SUCH_FILE);
        Stream_Write_UINT32(output_stream, 0); /* fileId */
        Stream_Write_UINT8(output_stream,  0); /* information */
    }
    else {

        guac_client_log_info(device->rdpdr->client, "Opened file \"%s\" ... new id=%i", path, file_id);
        guac_client_log_info(device->rdpdr->client,
                "des=%i, attrib=%i, shared=%i, disp=%i, opt=%i",
                desired_access, file_attributes, shared_access, create_disposition,
                create_options);

        Stream_Write_UINT32(output_stream, STATUS_SUCCESS);
        Stream_Write_UINT32(output_stream, file_id);    /* fileId */
        Stream_Write_UINT8(output_stream,  FILE_OPENED); /* information */

    }

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

static void guac_rdpdr_fs_process_read(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {
    /* STUB */
    guac_client_log_error(device->rdpdr->client, "read: %i", file_id);
}

static void guac_rdpdr_fs_process_write(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {
    /* STUB */
    guac_client_log_error(device->rdpdr->client, "write: %i", file_id);
}

static void guac_rdpdr_fs_process_close(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {

    wStream* output_stream = Stream_New(NULL, 21);

    /* Close file */
    guac_client_log_info(device->rdpdr->client, "Closing file id=%i", file_id);
    guac_rdpdr_fs_close(device, file_id);

    /* Write header */
    Stream_Write_UINT16(output_stream, RDPDR_CTYP_CORE);
    Stream_Write_UINT16(output_stream, PAKID_CORE_DEVICE_IOCOMPLETION);

    /* Write content */
    Stream_Write_UINT32(output_stream, device->device_id);
    Stream_Write_UINT32(output_stream, completion_id);
    Stream_Write_UINT32(output_stream, STATUS_SUCCESS);
    Stream_Write(output_stream, "\0\0\0\0\0", 5); /* Padding */

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

static void guac_rdpdr_fs_query_volume_info(guac_rdpdr_device* device, wStream* input_stream,
        int completion_id) {

    wStream* output_stream = Stream_New(NULL, 38 + GUAC_FILESYSTEM_NAME_LENGTH);

    /* Write header */
    Stream_Write_UINT16(output_stream, RDPDR_CTYP_CORE);
    Stream_Write_UINT16(output_stream, PAKID_CORE_DEVICE_IOCOMPLETION);

    /* Write content */
    Stream_Write_UINT32(output_stream, device->device_id);
    Stream_Write_UINT32(output_stream, completion_id);
    Stream_Write_UINT32(output_stream, STATUS_SUCCESS);

    Stream_Write_UINT32(output_stream, 18 + GUAC_FILESYSTEM_NAME_LENGTH);
    Stream_Write_UINT64(output_stream, WINDOWS_TIME(0)); /* VolumeCreationTime */
    Stream_Write(output_stream, "GUAC", 4);              /* VolumeSerialNumber */
    Stream_Write_UINT32(output_stream, GUAC_FILESYSTEM_NAME_LENGTH);
    Stream_Write_UINT8(output_stream, FALSE); /* SupportsObjects */
    Stream_Write_UINT8(output_stream, 0);     /* Reserved */
    Stream_Write(output_stream, GUAC_FILESYSTEM_NAME, GUAC_FILESYSTEM_NAME_LENGTH);

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

static void guac_rdpdr_fs_query_size_info(guac_rdpdr_device* device, wStream* input_stream,
        int completion_id) {
    /* STUB */
    guac_client_log_error(device->rdpdr->client,
            "Unimplemented stub: guac_rdpdr_fs_query_size_info");
}

static void guac_rdpdr_fs_query_device_info(guac_rdpdr_device* device, wStream* input_stream,
        int completion_id) {
    /* STUB */
    guac_client_log_error(device->rdpdr->client,
            "Unimplemented stub: guac_rdpdr_fs_query_devive_info");
}

static void guac_rdpdr_fs_query_attribute_info(guac_rdpdr_device* device, wStream* input_stream,
        int completion_id) {
    /* STUB */
    guac_client_log_error(device->rdpdr->client,
            "Unimplemented stub: guac_rdpdr_fs_query_attribute_info");
}

static void guac_rdpdr_fs_query_full_size_info(guac_rdpdr_device* device, wStream* input_stream,
        int completion_id) {
    /* STUB */
    guac_client_log_error(device->rdpdr->client,
            "Unimplemented stub: guac_rdpdr_fs_query_full_size_info");
}

static void guac_rdpdr_fs_volume_info(guac_rdpdr_device* device, wStream* input_stream,
        int completion_id) {

    int fs_information_class, length;

    /* NOTE: Assuming file is open and a volume */

    Stream_Read_UINT32(input_stream, fs_information_class);
    Stream_Read_UINT32(input_stream, length);
    Stream_Seek(input_stream, 24); /* Padding */

    guac_client_log_info(device->rdpdr->client,
            "Received volume query - class=0x%x, length=%i", fs_information_class, length);

    /* Dispatch to appropriate class-specific handler */
    switch (fs_information_class) {

        case FileFsVolumeInformation:
            guac_rdpdr_fs_query_volume_info(device, input_stream, completion_id);
            break;

        case FileFsSizeInformation:
            guac_rdpdr_fs_query_size_info(device, input_stream, completion_id);
            break;

        case FileFsDeviceInformation:
            guac_rdpdr_fs_query_device_info(device, input_stream, completion_id);
            break;

        case FileFsAttributeInformation:
            guac_rdpdr_fs_query_attribute_info(device, input_stream, completion_id);
            break;

        case FileFsFullSizeInformation:
            guac_rdpdr_fs_query_full_size_info(device, input_stream, completion_id);
            break;

        default:
            guac_client_log_info(device->rdpdr->client,
                    "Unknown volume information class: 0x%x", fs_information_class);
    }

}

static void guac_rdpdr_fs_query_basic_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {

    wStream* output_stream = Stream_New(NULL, 60);
    /*guac_rdpdr_fs_file* file = device->files[file_id];*/

    /* Write header */
    Stream_Write_UINT16(output_stream, RDPDR_CTYP_CORE);
    Stream_Write_UINT16(output_stream, PAKID_CORE_DEVICE_IOCOMPLETION);

    /* Write content */
    Stream_Write_UINT32(output_stream, device->device_id);
    Stream_Write_UINT32(output_stream, completion_id);
    Stream_Write_UINT32(output_stream, STATUS_SUCCESS);

    Stream_Write_UINT32(output_stream, 18 + GUAC_FILESYSTEM_NAME_LENGTH);
    Stream_Write_UINT64(output_stream, WINDOWS_TIME(0));       /* CreationTime   */
    Stream_Write_UINT64(output_stream, WINDOWS_TIME(0));       /* LastAccessTime */
    Stream_Write_UINT64(output_stream, WINDOWS_TIME(0));       /* LastWriteTime  */
    Stream_Write_UINT64(output_stream, WINDOWS_TIME(0));       /* ChangeTime     */
    Stream_Write_UINT32(output_stream, FILE_ATTRIBUTE_NORMAL); /* FileAttributes */
    Stream_Write_UINT32(output_stream, 0);                     /* Reserved */

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

static void guac_rdpdr_fs_query_standard_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {
    /* STUB */
    guac_client_log_error(device->rdpdr->client,
            "Unimplemented stub: guac_rdpdr_fs_query_standard_info");
}

static void guac_rdpdr_fs_query_attribute_tag_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {
    /* STUB */
    guac_client_log_error(device->rdpdr->client,
            "Unimplemented stub: guac_rdpdr_fs_query_attribute_tag_info");
}

static void guac_rdpdr_fs_file_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {

    int fs_information_class, length;

    /* NOTE: Assuming file is open and a volume */

    Stream_Read_UINT32(input_stream, fs_information_class);
    Stream_Read_UINT32(input_stream, length);
    Stream_Seek(input_stream, 24); /* Padding */

    guac_client_log_info(device->rdpdr->client,
            "Received file query - class=0x%x, length=%i", fs_information_class, length);

    /* Dispatch to appropriate class-specific handler */
    switch (fs_information_class) {

        case FileBasicInformation:
            guac_rdpdr_fs_query_basic_info(device, input_stream, file_id, completion_id);
            break;

        case FileStandardInformation:
            guac_rdpdr_fs_query_standard_info(device, input_stream, file_id, completion_id);
            break;

        case FileAttributeTagInformation:
            guac_rdpdr_fs_query_attribute_tag_info(device, input_stream, file_id, completion_id);
            break;

        default:
            guac_client_log_info(device->rdpdr->client,
                    "Unknown file information class: 0x%x", fs_information_class);
    }

}

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
            guac_client_log_error(device->rdpdr->client,
                    "IRP_MJ_DIRECTORY_CONTROL unsupported");
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

int guac_rdpdr_fs_open(guac_rdpdr_device* device, const char* path) {

    guac_rdpdr_fs_data* data = (guac_rdpdr_fs_data*) device->data;

    /* If files available, allocate a new file ID */
    if (data->open_files < GUAC_RDPDR_FS_MAX_FILES) {

        /* Get file ID */
        int file_id = guac_pool_next_int(data->file_id_pool);
        guac_rdpdr_fs_file* file = &(data->files[file_id]);

        data->open_files++;

        /* If path is empty, it refers to the volume itself */
        if (path[0] == '\0')
            return -2;

        /* Otherwise, parse path */
        else {

            file->type = GUAC_RDPDR_FS_FILE;
            /* STUB */

        }

        return file_id;

    }

    /* Otherwise, no file IDs available */
    return -1;

}

void guac_rdpdr_fs_close(guac_rdpdr_device* device, int file_id) {

    guac_rdpdr_fs_data* data = (guac_rdpdr_fs_data*) device->data;

    /* Only close if file ID is valid */
    if (file_id >= 0 && file_id <= GUAC_RDPDR_FS_MAX_FILES-1) {
        guac_pool_free_int(data->file_id_pool, file_id);
        data->open_files--;
    }

}

