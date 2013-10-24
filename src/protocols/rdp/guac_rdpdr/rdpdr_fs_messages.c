
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
#include <errno.h>

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

    wStream* output_stream;
    int file_id;

    int desired_access, file_attributes;
    int create_disposition, create_options, path_length;
    char path[GUAC_RDPDR_FS_MAX_PATH];

    /* Read "create" information */
    Stream_Read_UINT32(input_stream, desired_access);
    Stream_Seek_UINT64(input_stream); /* allocation size */
    Stream_Read_UINT32(input_stream, file_attributes);
    Stream_Seek_UINT32(input_stream); /* shared access */
    Stream_Read_UINT32(input_stream, create_disposition);
    Stream_Read_UINT32(input_stream, create_options);
    Stream_Read_UINT32(input_stream, path_length);

    /* FIXME: Validate path length */

    /* Convert path to UTF-8 */
    guac_rdp_utf16_to_utf8(Stream_Pointer(input_stream),
            path, path_length/2 - 1);

    /* Open file */
    file_id = guac_rdpdr_fs_open(device, path, desired_access, file_attributes,
            create_disposition, create_options);

    /* If an error occurred, notify server */
    if (file_id < 0) {
        guac_client_log_error(device->rdpdr->client,
                "File open refused (%i): \"%s\"", file_id, path);

        output_stream = guac_rdpdr_new_io_completion(device, completion_id,
                guac_rdpdr_fs_get_status(file_id), 5);
        Stream_Write_UINT32(output_stream, 0); /* fileId */
        Stream_Write_UINT8(output_stream,  0); /* information */
    }

    /* Otherwise, open succeeded */
    else {
        output_stream = guac_rdpdr_new_io_completion(device, completion_id,
                STATUS_SUCCESS, 5);
        Stream_Write_UINT32(output_stream, file_id);    /* fileId */
        Stream_Write_UINT8(output_stream,  0);          /* information */
    }

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_fs_process_read(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {

    UINT32 length;
    UINT64 offset;
    char buffer[4096];

    wStream* output_stream;
    guac_rdpdr_fs_file* file;

    /* Get file */
    file = guac_rdpdr_fs_get_file(device, file_id);
    if (file == NULL)
        return;

    /* Read packet */
    Stream_Read_UINT32(input_stream, length);
    Stream_Read_UINT64(input_stream, offset);

    /* If file is a directory, fail */
    if (file->attributes & FILE_ATTRIBUTE_DIRECTORY) {
        guac_client_log_error(device->rdpdr->client,
                "Refusing to read directory as a file");

        output_stream = guac_rdpdr_new_io_completion(device, completion_id,
                STATUS_FILE_IS_A_DIRECTORY, 4);
        Stream_Write_UINT32(output_stream, 0); /* Length */
    }

    /* Otherwise, perform read */
    else {

        int bytes_read;

        /* Read no more than size of buffer */
        if (length > sizeof(buffer))
            length = sizeof(buffer);

        /* Attempt read */
        lseek(file->fd, offset, SEEK_SET);
        bytes_read = read(file->fd, buffer, length);

        /* If error, return invalid parameter */
        if (bytes_read < 0) {
            guac_client_log_error(device->rdpdr->client,
                    "Unable to read from file: %s", strerror(errno));

            output_stream = guac_rdpdr_new_io_completion(device, completion_id,
                    STATUS_INVALID_PARAMETER, 4);
            Stream_Write_UINT32(output_stream, 0); /* Length */
        }

        /* Otherwise, send bytes read */
        else {
            output_stream = guac_rdpdr_new_io_completion(device, completion_id,
                    STATUS_SUCCESS, 4+bytes_read);
            Stream_Write_UINT32(output_stream, bytes_read);  /* Length */
            Stream_Write(output_stream, buffer, bytes_read); /* ReadData */
        }

    }

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_fs_process_write(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {

    UINT32 length;
    UINT64 offset;

    wStream* output_stream;
    guac_rdpdr_fs_file* file;

    /* Get file */
    file = guac_rdpdr_fs_get_file(device, file_id);
    if (file == NULL)
        return;

    /* Read packet */
    Stream_Read_UINT32(input_stream, length);
    Stream_Read_UINT64(input_stream, offset);
    Stream_Seek(input_stream, 20); /* Padding */

    /* If file is a directory, fail */
    if (file->attributes & FILE_ATTRIBUTE_DIRECTORY) {
        guac_client_log_error(device->rdpdr->client,
                "Refusing to write directory as a file");

        output_stream = guac_rdpdr_new_io_completion(device, completion_id,
                STATUS_FILE_IS_A_DIRECTORY, 5);
        Stream_Write_UINT32(output_stream, 0); /* Length */
        Stream_Write_UINT8(output_stream, 0);  /* Padding */
    }

    /* Otherwise, perform write */
    else {

        int bytes_written;

        /* Attempt read */
        lseek(file->fd, offset, SEEK_SET);
        bytes_written = write(file->fd, Stream_Pointer(input_stream), length);

        /* If error, return invalid parameter */
        if (bytes_written < 0) {
            guac_client_log_error(device->rdpdr->client,
                    "Unable to write to file %i: %s", file->fd,
                    strerror(errno));

            output_stream = guac_rdpdr_new_io_completion(device, completion_id,
                    STATUS_ACCESS_DENIED, 5);
            Stream_Write_UINT32(output_stream, 0); /* Length */
            Stream_Write_UINT8(output_stream, 0);  /* Padding */
        }

        /* Otherwise, send success */
        else {
            output_stream = guac_rdpdr_new_io_completion(device, completion_id,
                    STATUS_SUCCESS, 5);
            Stream_Write_UINT32(output_stream, bytes_written);  /* Length */
            Stream_Write_UINT8(output_stream, 0);  /* Padding */
        }

    }

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_fs_process_close(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {

    wStream* output_stream;

    /* Close file */
    guac_rdpdr_fs_close(device, file_id);

    output_stream = guac_rdpdr_new_io_completion(device, completion_id,
            STATUS_SUCCESS, 4);
    Stream_Write(output_stream, "\0\0\0\0", 4); /* Padding */

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_fs_process_volume_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {

    int fs_information_class;

    Stream_Read_UINT32(input_stream, fs_information_class);

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

    int fs_information_class;

    Stream_Read_UINT32(input_stream, fs_information_class);

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

void guac_rdpdr_fs_process_set_file_info(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {

    int fs_information_class;
    int length;

    Stream_Read_UINT32(input_stream, fs_information_class);
    Stream_Read_UINT32(input_stream, length); /* Length */
    Stream_Seek(input_stream, 24);            /* Padding */

    /* Dispatch to appropriate class-specific handler */
    switch (fs_information_class) {

        case FileBasicInformation:
            guac_rdpdr_fs_process_set_basic_info(device, input_stream,
                    file_id, completion_id, length);
            break;

        case FileEndOfFileInformation:
            guac_rdpdr_fs_process_set_end_of_file_info(device, input_stream,
                    file_id, completion_id, length);
            break;

        case FileDispositionInformation:
            guac_rdpdr_fs_process_set_disposition_info(device, input_stream,
                    file_id, completion_id, length);
            break;

        case FileRenameInformation:
            guac_rdpdr_fs_process_set_rename_info(device, input_stream,
                    file_id, completion_id, length);
            break;

        case FileAllocationInformation:
            guac_rdpdr_fs_process_set_allocation_info(device, input_stream,
                    file_id, completion_id, length);
            break;

        default:
            guac_client_log_info(device->rdpdr->client,
                    "Unknown file information class: 0x%x",
                    fs_information_class);
    }

}

void guac_rdpdr_fs_process_device_control(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {

    wStream* output_stream = guac_rdpdr_new_io_completion(device,
            completion_id, STATUS_INVALID_PARAMETER, 4);

    /* No content for now */
    Stream_Write_UINT32(output_stream, 0);

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_fs_process_notify_change_directory(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {
    /* STUB */
    guac_client_log_info(device->rdpdr->client, "STUB: %s", __func__);
}

void guac_rdpdr_fs_process_query_directory(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {

    wStream* output_stream;

    guac_rdpdr_fs_file* file;
    int fs_information_class, initial_query;
    int path_length;

    const char* entry_name;

    /* Get file */
    file = guac_rdpdr_fs_get_file(device, file_id);
    if (file == NULL)
        return;

    /* Read main header */
    Stream_Read_UINT32(input_stream, fs_information_class);
    Stream_Read_UINT8(input_stream,  initial_query);
    Stream_Read_UINT32(input_stream, path_length);

    /* If this is the first query, the path is included after padding */
    if (initial_query) {

        Stream_Seek(input_stream, 23);       /* Padding */

        /* FIXME: Validate path length */

        /* Convert path to UTF-8 */
        guac_rdp_utf16_to_utf8(Stream_Pointer(input_stream),
                file->dir_pattern, path_length/2 - 1);

    }

    /* Find first matching entry in directory */
    while ((entry_name = guac_rdpdr_fs_read_dir(device, file_id)) != NULL) {

        /* Convert to absolute path */
        char entry_path[GUAC_RDPDR_FS_MAX_PATH];
        if (guac_rdpdr_fs_convert_path(file->absolute_path,
                    entry_name, entry_path) == 0) {

            int entry_file_id;

            /* Pattern defined and match fails, continue with next file */
            if (guac_rdpdr_fs_matches(entry_path, file->dir_pattern))
                continue;

            /* Open directory entry */
            entry_file_id = guac_rdpdr_fs_open(device, entry_path,
                    ACCESS_FILE_READ_DATA, 0, DISP_FILE_OPEN, 0);

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
                                "Unknown dir information class: 0x%x",
                                fs_information_class);
                }

                guac_rdpdr_fs_close(device, entry_file_id);
                return;

            } /* end if file exists */
        } /* end if path valid */
    } /* end if entry exists */

    /*
     * Handle errors as a lack of files.
     */

    output_stream = guac_rdpdr_new_io_completion(device, completion_id,
            STATUS_NO_MORE_FILES, 5);

    Stream_Write_UINT32(output_stream, 0); /* Length */
    Stream_Write_UINT8(output_stream, 0);  /* Padding */

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_fs_process_lock_control(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {
    /* STUB */
    guac_client_log_info(device->rdpdr->client, "STUB: %s", __func__);
}

