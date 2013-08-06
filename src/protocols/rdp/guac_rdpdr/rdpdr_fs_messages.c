
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
#include "rdpdr_fs_messages_vol_info.h"
#include "rdpdr_fs_messages_file_info.h"
#include "rdpdr_fs_messages_dir_info.h"
#include "rdpdr_messages.h"
#include "rdpdr_service.h"
#include "client.h"
#include "unicode.h"

#include <freerdp/utils/svc_plugin.h>


void guac_rdpdr_fs_process_create(guac_rdpdr_device* device,
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
    file_id = guac_rdpdr_fs_open(device, path, desired_access, create_disposition);

    /* Write header */
    Stream_Write_UINT16(output_stream, RDPDR_CTYP_CORE);
    Stream_Write_UINT16(output_stream, PAKID_CORE_DEVICE_IOCOMPLETION);

    /* Write content */
    Stream_Write_UINT32(output_stream, device->device_id);
    Stream_Write_UINT32(output_stream, completion_id);

    /* If no file IDs available, notify server */
    if (file_id == GUAC_RDPDR_FS_ENFILE) {
        guac_client_log_error(device->rdpdr->client, "File open refused - too many open files");
        Stream_Write_UINT32(output_stream, STATUS_TOO_MANY_OPENED_FILES);
        Stream_Write_UINT32(output_stream, 0); /* fileId */
        Stream_Write_UINT8(output_stream,  0); /* information */
    }

    /* If file does not exist, notify server */
    else if (file_id == GUAC_RDPDR_FS_ENOENT) {
        guac_client_log_error(device->rdpdr->client,
                "File open refused - does not exist: \"%s\"", path);
        Stream_Write_UINT32(output_stream, STATUS_NO_SUCH_FILE);
        Stream_Write_UINT32(output_stream, 0); /* fileId */
        Stream_Write_UINT8(output_stream,  0); /* information */
    }

    /* Otherwise, open succeeded */
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

void guac_rdpdr_fs_process_read(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {
    /* STUB */
    guac_client_log_error(device->rdpdr->client, "read: %i", file_id);
}

void guac_rdpdr_fs_process_write(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {
    /* STUB */
    guac_client_log_error(device->rdpdr->client, "write: %i", file_id);
}

void guac_rdpdr_fs_process_close(guac_rdpdr_device* device,
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

void guac_rdpdr_fs_process_volume_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {

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
            guac_rdpdr_fs_process_query_volume_info(device, input_stream,
                    file_id, completion_id);
            break;

        case FileFsSizeInformation:
            guac_rdpdr_fs_process_query_size_info(device, input_stream,
                    file_id, completion_id);
            break;

        case FileFsDeviceInformation:
            guac_rdpdr_fs_process_query_device_info(device, input_stream,
                    file_id, completion_id);
            break;

        case FileFsAttributeInformation:
            guac_rdpdr_fs_process_query_attribute_info(device, input_stream,
                    file_id, completion_id);
            break;

        case FileFsFullSizeInformation:
            guac_rdpdr_fs_process_query_full_size_info(device, input_stream,
                    file_id, completion_id);
            break;

        default:
            guac_client_log_info(device->rdpdr->client,
                    "Unknown volume information class: 0x%x", fs_information_class);
    }

}

void guac_rdpdr_fs_process_file_info(guac_rdpdr_device* device, wStream* input_stream,
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
            guac_rdpdr_fs_process_query_basic_info(device, input_stream,
                    file_id, completion_id);
            break;

        case FileStandardInformation:
            guac_rdpdr_fs_process_query_standard_info(device, input_stream,
                    file_id, completion_id);
            break;

        case FileAttributeTagInformation:
            guac_rdpdr_fs_process_query_attribute_tag_info(device, input_stream,
                    file_id, completion_id);
            break;

        default:
            guac_client_log_info(device->rdpdr->client,
                    "Unknown file information class: 0x%x", fs_information_class);
    }

}

void guac_rdpdr_fs_process_set_volume_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {
    /* STUB */
    guac_client_log_info(device->rdpdr->client, "STUB: %s", __func__);
}

void guac_rdpdr_fs_process_set_file_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {
    /* STUB */
    guac_client_log_info(device->rdpdr->client, "STUB: %s", __func__);
}

void guac_rdpdr_fs_process_device_control(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {
    /* STUB */
    guac_client_log_info(device->rdpdr->client, "STUB: %s", __func__);
}

void guac_rdpdr_fs_process_notify_change_directory(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {
    /* STUB */
    guac_client_log_info(device->rdpdr->client, "STUB: %s", __func__);
}

void guac_rdpdr_fs_process_query_directory(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {

    wStream* output_stream;

    guac_rdpdr_fs_data* data = (guac_rdpdr_fs_data*) device->data;
    guac_rdpdr_fs_file* file = &(data->files[file_id]);
    int fs_information_class, initial_query;
    int path_length;

    const char* entry_name;

    /* Read main header */
    Stream_Read_UINT32(input_stream, fs_information_class);
    Stream_Read_UINT8(input_stream,  initial_query);
    Stream_Read_UINT32(input_stream, path_length);

    /* If this is the first query, the path is included after padding */
    if (initial_query) {

        unsigned char* path;

        Stream_Seek(input_stream, 23);       /* Padding */
        path = Stream_Pointer(input_stream); /* Path */

        guac_client_log_info(device->rdpdr->client,
                "Received initial dir query - class=0x%x, path_length=%i, path=%s",
                fs_information_class, path_length, path);

    }
    else
        guac_client_log_info(device->rdpdr->client,
                "Received continued dir query - class=0x%x",
                fs_information_class);

    /* If entry exists, call appriate handler to send data */
    entry_name = guac_rdpdr_fs_read_dir(device, file_id);
    if (entry_name != NULL) {

        int entry_file_id;

        /* Convert to absolute path */
        char entry_path[GUAC_RDPDR_FS_MAX_PATH];
        if (guac_rdpdr_fs_convert_path(file->absolute_path, entry_name, entry_path))
            guac_client_log_info(device->rdpdr->client,
                    "Conversion failed: parent=\"%s\", name=\"%s\"",
                    file->absolute_path, entry_name); /* FIXME: Return no-more-files */

        else {

            /* Open directory entry */
            entry_file_id = guac_rdpdr_fs_open(device, entry_path,
                    ACCESS_FILE_READ_DATA, DISP_FILE_OVERWRITE);

            if (entry_file_id >= 0) {

                /* Dispatch to appropriate class-specific handler */
                switch (fs_information_class) {

                    case FileDirectoryInformation:
                        guac_rdpdr_fs_process_query_directory_info(device,
                                entry_name, entry_file_id, completion_id);
                        break;

                    case FileFullDirectoryInformation:
                        guac_rdpdr_fs_process_query_full_directory_info(device,
                                entry_name, entry_file_id, completion_id);
                        break;

                    case FileBothDirectoryInformation:
                        guac_rdpdr_fs_process_query_both_directory_info(device,
                                entry_name, entry_file_id, completion_id);
                        break;

                    case FileNamesInformation:
                        guac_rdpdr_fs_process_query_names_info(device,
                                entry_name, entry_file_id, completion_id);
                        break;

                    default:
                        guac_client_log_info(device->rdpdr->client,
                                "Unknown dir information class: 0x%x", fs_information_class);
                }

                guac_rdpdr_fs_close(device, entry_file_id);
                return;

            } /* end if file exists */
        } /* end if path valid */
    } /* end if entry exists */

    /*
     * Handle errors as a lack of files.
     */

    output_stream = Stream_New(NULL, 16);

    /* Write header */
    Stream_Write_UINT16(output_stream, RDPDR_CTYP_CORE);
    Stream_Write_UINT16(output_stream, PAKID_CORE_DEVICE_IOCOMPLETION);

    /* Write content */
    Stream_Write_UINT32(output_stream, device->device_id);
    Stream_Write_UINT32(output_stream, completion_id);
    Stream_Write_UINT32(output_stream, STATUS_NO_MORE_FILES);

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);
    guac_client_log_info(device->rdpdr->client, "Sent STATUS_NO_MORE_FILES");

}

void guac_rdpdr_fs_process_lock_control(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {
    /* STUB */
    guac_client_log_info(device->rdpdr->client, "STUB: %s", __func__);
}

