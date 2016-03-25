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

#include "rdpdr_service.h"
#include "rdp_fs.h"
#include "rdp_status.h"
#include "unicode.h"

#include <freerdp/utils/svc_plugin.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#include <winpr/wtypes.h>
#else
#include "compat/winpr-stream.h"
#include "compat/winpr-wtypes.h"
#endif

#include <inttypes.h>
#include <stdint.h>
#include <string.h>

void guac_rdpdr_fs_process_query_basic_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {

    wStream* output_stream;
    guac_rdp_fs_file* file;

    /* Get file */
    file = guac_rdp_fs_get_file((guac_rdp_fs*) device->data, file_id);
    if (file == NULL)
        return;

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i]",
            __func__, file_id);

    output_stream = guac_rdpdr_new_io_completion(device, completion_id,
            STATUS_SUCCESS, 40);

    Stream_Write_UINT32(output_stream, 36);
    Stream_Write_UINT64(output_stream, file->ctime);      /* CreationTime   */
    Stream_Write_UINT64(output_stream, file->atime);      /* LastAccessTime */
    Stream_Write_UINT64(output_stream, file->mtime);      /* LastWriteTime  */
    Stream_Write_UINT64(output_stream, file->mtime);      /* ChangeTime     */
    Stream_Write_UINT32(output_stream, file->attributes); /* FileAttributes */

    /* Reserved field must not be sent */

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_fs_process_query_standard_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {

    wStream* output_stream;
    guac_rdp_fs_file* file;
    BOOL is_directory = FALSE;

    /* Get file */
    file = guac_rdp_fs_get_file((guac_rdp_fs*) device->data, file_id);
    if (file == NULL)
        return;

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i]",
            __func__, file_id);

    if (file->attributes & FILE_ATTRIBUTE_DIRECTORY)
        is_directory = TRUE;

    output_stream = guac_rdpdr_new_io_completion(device, completion_id,
            STATUS_SUCCESS, 26);

    Stream_Write_UINT32(output_stream, 22);
    Stream_Write_UINT64(output_stream, file->size);   /* AllocationSize */
    Stream_Write_UINT64(output_stream, file->size);   /* EndOfFile      */
    Stream_Write_UINT32(output_stream, 1);            /* NumberOfLinks  */
    Stream_Write_UINT8(output_stream,  0);            /* DeletePending  */
    Stream_Write_UINT8(output_stream,  is_directory); /* Directory      */

    /* Reserved field must not be sent */

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_fs_process_query_attribute_tag_info(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {

    wStream* output_stream;
    guac_rdp_fs_file* file;

    /* Get file */
    file = guac_rdp_fs_get_file((guac_rdp_fs*) device->data, file_id);
    if (file == NULL)
        return;

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i]",
            __func__, file_id);

    output_stream = guac_rdpdr_new_io_completion(device, completion_id,
            STATUS_SUCCESS, 12);

    Stream_Write_UINT32(output_stream, 8);
    Stream_Write_UINT32(output_stream, file->attributes); /* FileAttributes */
    Stream_Write_UINT32(output_stream, 0);                /* ReparseTag */

    /* Reserved field must not be sent */

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_fs_process_set_rename_info(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id, int length) {

    int result;
    int filename_length;
    wStream* output_stream;
    char destination_path[GUAC_RDP_FS_MAX_PATH];

    /* Read structure */
    Stream_Seek_UINT8(input_stream); /* ReplaceIfExists */
    Stream_Seek_UINT8(input_stream); /* RootDirectory */
    Stream_Read_UINT32(input_stream, filename_length); /* FileNameLength */

    /* Convert name to UTF-8 */
    guac_rdp_utf16_to_utf8(Stream_Pointer(input_stream), filename_length/2,
            destination_path, sizeof(destination_path));

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i] destination_path=\"%s\"",
            __func__, file_id, destination_path);

    /* If file moving to \Download folder, start stream, do not move */
    if (strncmp(destination_path, "\\Download\\", 10) == 0) {

        guac_rdp_fs_file* file;

        /* Get file */
        file = guac_rdp_fs_get_file((guac_rdp_fs*) device->data, file_id);
        if (file == NULL)
            return;

        /* Initiate download, pretend move succeeded */
        guac_rdpdr_start_download(device, file->absolute_path);
        output_stream = guac_rdpdr_new_io_completion(device,
                completion_id, STATUS_SUCCESS, 4);

    }

    /* Otherwise, rename as requested */
    else {

        result = guac_rdp_fs_rename((guac_rdp_fs*) device->data, file_id,
                destination_path);
        if (result < 0)
            output_stream = guac_rdpdr_new_io_completion(device,
                    completion_id, guac_rdp_fs_get_status(result), 4);
        else
            output_stream = guac_rdpdr_new_io_completion(device,
                    completion_id, STATUS_SUCCESS, 4);

    }

    Stream_Write_UINT32(output_stream, length);
    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_fs_process_set_allocation_info(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id, int length) {

    int result;
    UINT64 size;
    wStream* output_stream;

    /* Read new size */
    Stream_Read_UINT64(input_stream, size); /* AllocationSize */

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i] size=%" PRIu64,
            __func__, file_id, (uint64_t) size);

    /* Truncate file */
    result = guac_rdp_fs_truncate((guac_rdp_fs*) device->data, file_id, size);
    if (result < 0)
        output_stream = guac_rdpdr_new_io_completion(device,
                completion_id, guac_rdp_fs_get_status(result), 4);
    else
        output_stream = guac_rdpdr_new_io_completion(device,
                completion_id, STATUS_SUCCESS, 4);

    Stream_Write_UINT32(output_stream, length);
    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_fs_process_set_disposition_info(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id, int length) {

    wStream* output_stream;

    /* Delete file */
    int result = guac_rdp_fs_delete((guac_rdp_fs*) device->data, file_id);
    if (result < 0)
        output_stream = guac_rdpdr_new_io_completion(device,
                completion_id, guac_rdp_fs_get_status(result), 4);
    else
        output_stream = guac_rdpdr_new_io_completion(device,
                completion_id, STATUS_SUCCESS, 4);

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i]",
            __func__, file_id);

    Stream_Write_UINT32(output_stream, length);

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_fs_process_set_end_of_file_info(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id, int length) {

    int result;
    UINT64 size;
    wStream* output_stream;

    /* Read new size */
    Stream_Read_UINT64(input_stream, size); /* AllocationSize */

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i] size=%" PRIu64,
            __func__, file_id, (uint64_t) size);

    /* Truncate file */
    result = guac_rdp_fs_truncate((guac_rdp_fs*) device->data, file_id, size);
    if (result < 0)
        output_stream = guac_rdpdr_new_io_completion(device,
                completion_id, guac_rdp_fs_get_status(result), 4);
    else
        output_stream = guac_rdpdr_new_io_completion(device,
                completion_id, STATUS_SUCCESS, 4);

    Stream_Write_UINT32(output_stream, length);
    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_fs_process_set_basic_info(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id, int length) {

    wStream* output_stream = guac_rdpdr_new_io_completion(device,
            completion_id, STATUS_SUCCESS, 4);

    /* Currently do nothing, just respond */
    Stream_Write_UINT32(output_stream, length);

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i] IGNORED",
            __func__, file_id);

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

