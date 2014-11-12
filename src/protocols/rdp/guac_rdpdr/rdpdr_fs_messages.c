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

#include "rdpdr_fs_messages_dir_info.h"
#include "rdpdr_fs_messages_file_info.h"
#include "rdpdr_fs_messages.h"
#include "rdpdr_fs_messages_vol_info.h"
#include "rdpdr_messages.h"
#include "rdpdr_service.h"
#include "rdp_fs.h"
#include "rdp_status.h"
#include "unicode.h"

#include <freerdp/utils/svc_plugin.h>
#include <guacamole/client.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#include <winpr/wtypes.h>
#else
#include "compat/winpr-stream.h"
#include "compat/winpr-wtypes.h"
#endif

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void guac_rdpdr_fs_process_create(guac_rdpdr_device* device,
        wStream* input_stream, int completion_id) {

    wStream* output_stream;
    int file_id;

    int desired_access, file_attributes;
    int create_disposition, create_options, path_length;
    char path[GUAC_RDP_FS_MAX_PATH];

    /* Read "create" information */
    Stream_Read_UINT32(input_stream, desired_access);
    Stream_Seek_UINT64(input_stream); /* allocation size */
    Stream_Read_UINT32(input_stream, file_attributes);
    Stream_Seek_UINT32(input_stream); /* shared access */
    Stream_Read_UINT32(input_stream, create_disposition);
    Stream_Read_UINT32(input_stream, create_options);
    Stream_Read_UINT32(input_stream, path_length);

    /* Convert path to UTF-8 */
    guac_rdp_utf16_to_utf8(Stream_Pointer(input_stream), path_length/2 - 1,
            path, sizeof(path));

    /* Open file */
    file_id = guac_rdp_fs_open((guac_rdp_fs*) device->data, path,
            desired_access, file_attributes,
            create_disposition, create_options);

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i] "
             "desired_access=0x%x, file_attributes=0x%x, "
             "create_disposition=0x%x, create_options=0x%x, path=\"%s\"",
             __func__, file_id,
             desired_access, file_attributes,
             create_disposition, create_options, path);

    /* If an error occurred, notify server */
    if (file_id < 0) {
        guac_client_log(device->rdpdr->client, GUAC_LOG_ERROR,
                "File open refused (%i): \"%s\"", file_id, path);

        output_stream = guac_rdpdr_new_io_completion(device, completion_id,
                guac_rdp_fs_get_status(file_id), 5);
        Stream_Write_UINT32(output_stream, 0); /* fileId */
        Stream_Write_UINT8(output_stream,  0); /* information */
    }

    /* Otherwise, open succeeded */
    else {

        guac_rdp_fs_file* file;

        output_stream = guac_rdpdr_new_io_completion(device, completion_id,
                STATUS_SUCCESS, 5);
        Stream_Write_UINT32(output_stream, file_id);    /* fileId */
        Stream_Write_UINT8(output_stream,  0);          /* information */

        /* Create \Download if it doesn't exist */
        file = guac_rdp_fs_get_file((guac_rdp_fs*) device->data, file_id);
        if (file != NULL && strcmp(file->absolute_path, "\\") == 0) {
            int download_id =
                guac_rdp_fs_open((guac_rdp_fs*) device->data, "\\Download",
                    ACCESS_GENERIC_READ, 0,
                    DISP_FILE_OPEN_IF, FILE_DIRECTORY_FILE);

            if (download_id >= 0)
                guac_rdp_fs_close((guac_rdp_fs*) device->data, download_id);
        }

    }

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_fs_process_read(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {

    UINT32 length;
    UINT64 offset;
    char* buffer;
    int bytes_read;

    wStream* output_stream;

    /* Read packet */
    Stream_Read_UINT32(input_stream, length);
    Stream_Read_UINT64(input_stream, offset);

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i] length=%i, offset=%" PRIu64,
             __func__, file_id, length, (uint64_t) offset);

    /* Ensure buffer size does not exceed a safe maximum */
    if (length > GUAC_RDP_MAX_READ_BUFFER)
        length = GUAC_RDP_MAX_READ_BUFFER;

    /* Allocate buffer */
    buffer = malloc(length);

    /* Attempt read */
    bytes_read = guac_rdp_fs_read((guac_rdp_fs*) device->data, file_id, offset,
            buffer, length);

    /* If error, return invalid parameter */
    if (bytes_read < 0) {
        output_stream = guac_rdpdr_new_io_completion(device, completion_id,
                guac_rdp_fs_get_status(bytes_read), 4);
        Stream_Write_UINT32(output_stream, 0); /* Length */
    }

    /* Otherwise, send bytes read */
    else {
        output_stream = guac_rdpdr_new_io_completion(device, completion_id,
                STATUS_SUCCESS, 4+bytes_read);
        Stream_Write_UINT32(output_stream, bytes_read);  /* Length */
        Stream_Write(output_stream, buffer, bytes_read); /* ReadData */
    }

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);
    free(buffer);

}

void guac_rdpdr_fs_process_write(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {

    UINT32 length;
    UINT64 offset;
    int bytes_written;

    wStream* output_stream;

    /* Read packet */
    Stream_Read_UINT32(input_stream, length);
    Stream_Read_UINT64(input_stream, offset);
    Stream_Seek(input_stream, 20); /* Padding */

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i] length=%i, offset=%" PRIu64,
             __func__, file_id, length, (uint64_t) offset);

    /* Attempt write */
    bytes_written = guac_rdp_fs_write((guac_rdp_fs*) device->data, file_id,
            offset, Stream_Pointer(input_stream), length);

    /* If error, return invalid parameter */
    if (bytes_written < 0) {
        output_stream = guac_rdpdr_new_io_completion(device, completion_id,
                guac_rdp_fs_get_status(bytes_written), 5);
        Stream_Write_UINT32(output_stream, 0); /* Length */
        Stream_Write_UINT8(output_stream, 0);  /* Padding */
    }

    /* Otherwise, send success */
    else {
        output_stream = guac_rdpdr_new_io_completion(device, completion_id,
                STATUS_SUCCESS, 5);
        Stream_Write_UINT32(output_stream, bytes_written); /* Length */
        Stream_Write_UINT8(output_stream, 0);              /* Padding */
    }

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_fs_process_close(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {

    wStream* output_stream;
    guac_rdp_fs_file* file;

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i]",
            __func__, file_id);

    /* Get file */
    file = guac_rdp_fs_get_file((guac_rdp_fs*) device->data, file_id);
    if (file == NULL)
        return;

    /* If file was written to, and it's in the \Download folder, start stream */
    if (file->bytes_written > 0 &&
            strncmp(file->absolute_path, "\\Download\\", 10) == 0) {
        guac_rdpdr_start_download(device, file->absolute_path);
        guac_rdp_fs_delete((guac_rdp_fs*) device->data, file_id);
    }

    /* Close file */
    guac_rdp_fs_close((guac_rdp_fs*) device->data, file_id);

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
            guac_client_log(device->rdpdr->client, GUAC_LOG_INFO,
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
            guac_client_log(device->rdpdr->client, GUAC_LOG_INFO,
                    "Unknown file information class: 0x%x", fs_information_class);
    }

}

void guac_rdpdr_fs_process_set_volume_info(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {

    wStream* output_stream = guac_rdpdr_new_io_completion(device,
            completion_id, STATUS_NOT_SUPPORTED, 0);

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i] Set volume info not supported",
            __func__, file_id);

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

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
            guac_client_log(device->rdpdr->client, GUAC_LOG_INFO,
                    "Unknown file information class: 0x%x",
                    fs_information_class);
    }

}

void guac_rdpdr_fs_process_device_control(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {

    wStream* output_stream = guac_rdpdr_new_io_completion(device,
            completion_id, STATUS_INVALID_PARAMETER, 4);

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i] IGNORED",
            __func__, file_id);

    /* No content for now */
    Stream_Write_UINT32(output_stream, 0);

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_fs_process_notify_change_directory(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i] Not implemented",
            __func__, file_id);

}

void guac_rdpdr_fs_process_query_directory(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {

    wStream* output_stream;

    guac_rdp_fs_file* file;
    int fs_information_class, initial_query;
    int path_length;

    const char* entry_name;

    /* Get file */
    file = guac_rdp_fs_get_file((guac_rdp_fs*) device->data, file_id);
    if (file == NULL)
        return;

    /* Read main header */
    Stream_Read_UINT32(input_stream, fs_information_class);
    Stream_Read_UINT8(input_stream,  initial_query);
    Stream_Read_UINT32(input_stream, path_length);

    /* If this is the first query, the path is included after padding */
    if (initial_query) {

        Stream_Seek(input_stream, 23);       /* Padding */

        /* Convert path to UTF-8 */
        guac_rdp_utf16_to_utf8(Stream_Pointer(input_stream), path_length/2 - 1,
                file->dir_pattern, sizeof(file->dir_pattern));

    }

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i] initial_query=%i, dir_pattern=\"%s\"",
             __func__, file_id, initial_query, file->dir_pattern);

    /* Find first matching entry in directory */
    while ((entry_name = guac_rdp_fs_read_dir((guac_rdp_fs*) device->data,
                    file_id)) != NULL) {

        /* Convert to absolute path */
        char entry_path[GUAC_RDP_FS_MAX_PATH];
        if (guac_rdp_fs_convert_path(file->absolute_path,
                    entry_name, entry_path) == 0) {

            int entry_file_id;

            /* Pattern defined and match fails, continue with next file */
            if (guac_rdp_fs_matches(entry_path, file->dir_pattern))
                continue;

            /* Open directory entry */
            entry_file_id = guac_rdp_fs_open((guac_rdp_fs*) device->data,
                    entry_path, ACCESS_FILE_READ_DATA, 0, DISP_FILE_OPEN, 0);

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
                        guac_client_log(device->rdpdr->client, GUAC_LOG_INFO,
                                "Unknown dir information class: 0x%x",
                                fs_information_class);
                }

                guac_rdp_fs_close((guac_rdp_fs*) device->data, entry_file_id);
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

    wStream* output_stream = guac_rdpdr_new_io_completion(device,
            completion_id, STATUS_NOT_SUPPORTED, 5);

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i] Lock not supported",
            __func__, file_id);

    Stream_Zero(output_stream, 5); /* Padding */

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

