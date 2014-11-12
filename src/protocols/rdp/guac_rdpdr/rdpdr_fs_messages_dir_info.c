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

#include "rdpdr_service.h"
#include "rdp_fs.h"
#include "rdp_status.h"
#include "unicode.h"

#include <freerdp/utils/svc_plugin.h>
#include <guacamole/unicode.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

#include <stddef.h>

void guac_rdpdr_fs_process_query_directory_info(guac_rdpdr_device* device,
        const char* entry_name, int file_id, int completion_id) {

    guac_rdp_fs_file* file;

    wStream* output_stream;
    int length = guac_utf8_strlen(entry_name);
    int utf16_length = length*2;

    unsigned char utf16_entry_name[256];
    guac_rdp_utf8_to_utf16((const unsigned char*) entry_name, length,
            (char*) utf16_entry_name, sizeof(utf16_entry_name));

    /* Get file */
    file = guac_rdp_fs_get_file((guac_rdp_fs*) device->data, file_id);
    if (file == NULL)
        return;

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i (entry_name=\"%s\")]",
            __func__, file_id, entry_name);

    output_stream = guac_rdpdr_new_io_completion(device, completion_id,
            STATUS_SUCCESS, 4 + 64 + utf16_length + 2);

    Stream_Write_UINT32(output_stream,
            64 + utf16_length + 2); /* Length */

    Stream_Write_UINT32(output_stream, 0); /* NextEntryOffset */
    Stream_Write_UINT32(output_stream, 0); /* FileIndex */
    Stream_Write_UINT64(output_stream, file->ctime); /* CreationTime */
    Stream_Write_UINT64(output_stream, file->atime); /* LastAccessTime */
    Stream_Write_UINT64(output_stream, file->mtime); /* LastWriteTime */
    Stream_Write_UINT64(output_stream, file->mtime); /* ChangeTime */
    Stream_Write_UINT64(output_stream, file->size);  /* EndOfFile */
    Stream_Write_UINT64(output_stream, file->size);  /* AllocationSize */
    Stream_Write_UINT32(output_stream, file->attributes);   /* FileAttributes */
    Stream_Write_UINT32(output_stream, utf16_length+2); /* FileNameLength*/

    Stream_Write(output_stream, utf16_entry_name, utf16_length); /* FileName */
    Stream_Write(output_stream, "\0\0", 2);

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_fs_process_query_full_directory_info(guac_rdpdr_device* device,
        const char* entry_name, int file_id, int completion_id) {

    guac_rdp_fs_file* file;

    wStream* output_stream;
    int length = guac_utf8_strlen(entry_name);
    int utf16_length = length*2;

    unsigned char utf16_entry_name[256];
    guac_rdp_utf8_to_utf16((const unsigned char*) entry_name, length,
            (char*) utf16_entry_name, sizeof(utf16_entry_name));

    /* Get file */
    file = guac_rdp_fs_get_file((guac_rdp_fs*) device->data, file_id);
    if (file == NULL)
        return;

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i (entry_name=\"%s\")]",
            __func__, file_id, entry_name);

    output_stream = guac_rdpdr_new_io_completion(device, completion_id,
            STATUS_SUCCESS, 4 + 68 + utf16_length + 2);

    Stream_Write_UINT32(output_stream,
            68 + utf16_length + 2); /* Length */

    Stream_Write_UINT32(output_stream, 0); /* NextEntryOffset */
    Stream_Write_UINT32(output_stream, 0); /* FileIndex */
    Stream_Write_UINT64(output_stream, file->ctime); /* CreationTime */
    Stream_Write_UINT64(output_stream, file->atime); /* LastAccessTime */
    Stream_Write_UINT64(output_stream, file->mtime); /* LastWriteTime */
    Stream_Write_UINT64(output_stream, file->mtime); /* ChangeTime */
    Stream_Write_UINT64(output_stream, file->size);  /* EndOfFile */
    Stream_Write_UINT64(output_stream, file->size);  /* AllocationSize */
    Stream_Write_UINT32(output_stream, file->attributes);   /* FileAttributes */
    Stream_Write_UINT32(output_stream, utf16_length+2); /* FileNameLength*/
    Stream_Write_UINT32(output_stream, 0); /* EaSize */

    Stream_Write(output_stream, utf16_entry_name, utf16_length); /* FileName */
    Stream_Write(output_stream, "\0\0", 2);

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_fs_process_query_both_directory_info(guac_rdpdr_device* device,
        const char* entry_name, int file_id, int completion_id) {

    guac_rdp_fs_file* file;

    wStream* output_stream;
    int length = guac_utf8_strlen(entry_name);
    int utf16_length = length*2;

    unsigned char utf16_entry_name[256];
    guac_rdp_utf8_to_utf16((const unsigned char*) entry_name, length,
            (char*) utf16_entry_name, sizeof(utf16_entry_name));

    /* Get file */
    file = guac_rdp_fs_get_file((guac_rdp_fs*) device->data, file_id);
    if (file == NULL)
        return;

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i (entry_name=\"%s\")]",
            __func__, file_id, entry_name);

    output_stream = guac_rdpdr_new_io_completion(device, completion_id,
            STATUS_SUCCESS, 4 + 69 + 24 + utf16_length + 2);

    Stream_Write_UINT32(output_stream,
            69 + 24 + utf16_length + 2); /* Length */

    Stream_Write_UINT32(output_stream, 0); /* NextEntryOffset */
    Stream_Write_UINT32(output_stream, 0); /* FileIndex */
    Stream_Write_UINT64(output_stream, file->ctime); /* CreationTime */
    Stream_Write_UINT64(output_stream, file->atime); /* LastAccessTime */
    Stream_Write_UINT64(output_stream, file->mtime); /* LastWriteTime */
    Stream_Write_UINT64(output_stream, file->mtime); /* ChangeTime */
    Stream_Write_UINT64(output_stream, file->size);  /* EndOfFile */
    Stream_Write_UINT64(output_stream, file->size);  /* AllocationSize */
    Stream_Write_UINT32(output_stream, file->attributes);   /* FileAttributes */
    Stream_Write_UINT32(output_stream, utf16_length+2); /* FileNameLength*/
    Stream_Write_UINT32(output_stream, 0); /* EaSize */
    Stream_Write_UINT8(output_stream,  0); /* ShortNameLength */

    /* Apparently, the reserved byte here must be skipped ... */

    Stream_Zero(output_stream, 24); /* FileName */
    Stream_Write(output_stream, utf16_entry_name, utf16_length); /* FileName */
    Stream_Write(output_stream, "\0\0", 2);

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_fs_process_query_names_info(guac_rdpdr_device* device,
        const char* entry_name, int file_id, int completion_id) {

    guac_rdp_fs_file* file;

    wStream* output_stream;
    int length = guac_utf8_strlen(entry_name);
    int utf16_length = length*2;

    unsigned char utf16_entry_name[256];
    guac_rdp_utf8_to_utf16((const unsigned char*) entry_name, length,
            (char*) utf16_entry_name, sizeof(utf16_entry_name));

    /* Get file */
    file = guac_rdp_fs_get_file((guac_rdp_fs*) device->data, file_id);
    if (file == NULL)
        return;

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i (entry_name=\"%s\")]",
            __func__, file_id, entry_name);

    output_stream = guac_rdpdr_new_io_completion(device, completion_id,
            STATUS_SUCCESS, 4 + 12 + utf16_length + 2);

    Stream_Write_UINT32(output_stream,
            12 + utf16_length + 2); /* Length */

    Stream_Write_UINT32(output_stream, 0); /* NextEntryOffset */
    Stream_Write_UINT32(output_stream, 0); /* FileIndex */
    Stream_Write_UINT32(output_stream, utf16_length+2); /* FileNameLength*/
    Stream_Write(output_stream, utf16_entry_name, utf16_length); /* FileName */
    Stream_Write(output_stream, "\0\0", 2);

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

