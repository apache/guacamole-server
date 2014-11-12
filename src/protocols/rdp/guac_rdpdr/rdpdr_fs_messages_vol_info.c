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

#include "rdpdr_messages.h"
#include "rdpdr_service.h"
#include "rdp_fs.h"
#include "rdp_status.h"

#include <freerdp/utils/svc_plugin.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#include <winpr/wtypes.h>
#else
#include "compat/winpr-stream.h"
#include "compat/winpr-wtypes.h"
#endif

void guac_rdpdr_fs_process_query_volume_info(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {

    wStream* output_stream = guac_rdpdr_new_io_completion(device,
            completion_id, STATUS_SUCCESS, 21 + GUAC_FILESYSTEM_LABEL_LENGTH);

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i]",
            __func__, file_id);

    Stream_Write_UINT32(output_stream, 17 + GUAC_FILESYSTEM_LABEL_LENGTH);
    Stream_Write_UINT64(output_stream, 0); /* VolumeCreationTime */
    Stream_Write_UINT32(output_stream, 0); /* VolumeSerialNumber */
    Stream_Write_UINT32(output_stream, GUAC_FILESYSTEM_LABEL_LENGTH);
    Stream_Write_UINT8(output_stream, FALSE); /* SupportsObjects */
    /* Reserved field must not be sent */
    Stream_Write(output_stream, GUAC_FILESYSTEM_LABEL, GUAC_FILESYSTEM_LABEL_LENGTH);

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_fs_process_query_size_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {

    guac_rdp_fs_info info = {0};
    guac_rdp_fs_get_info((guac_rdp_fs*) device->data, &info);

    wStream* output_stream = guac_rdpdr_new_io_completion(device,
            completion_id, STATUS_SUCCESS, 28);

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i]",
            __func__, file_id);

    Stream_Write_UINT32(output_stream, 24);
    Stream_Write_UINT64(output_stream, info.blocks_total);     /* TotalAllocationUnits */
    Stream_Write_UINT64(output_stream, info.blocks_available); /* AvailableAllocationUnits */
    Stream_Write_UINT32(output_stream, 1);                     /* SectorsPerAllocationUnit */
    Stream_Write_UINT32(output_stream, info.block_size);       /* BytesPerSector */

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_fs_process_query_device_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {

    wStream* output_stream = guac_rdpdr_new_io_completion(device,
            completion_id, STATUS_SUCCESS, 12);

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i]",
            __func__, file_id);

    Stream_Write_UINT32(output_stream, 8);
    Stream_Write_UINT32(output_stream, FILE_DEVICE_DISK); /* DeviceType */
    Stream_Write_UINT32(output_stream, 0); /* Characteristics */

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_fs_process_query_attribute_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {

    wStream* output_stream = guac_rdpdr_new_io_completion(device,
            completion_id, STATUS_SUCCESS, 16 + GUAC_FILESYSTEM_NAME_LENGTH);

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i]",
            __func__, file_id);

    Stream_Write_UINT32(output_stream, 12 + GUAC_FILESYSTEM_NAME_LENGTH);
    Stream_Write_UINT32(output_stream,
              FILE_UNICODE_ON_DISK
            | FILE_CASE_SENSITIVE_SEARCH
            | FILE_CASE_PRESERVED_NAMES); /* FileSystemAttributes */
    Stream_Write_UINT32(output_stream, GUAC_RDP_FS_MAX_PATH ); /* MaximumComponentNameLength */
    Stream_Write_UINT32(output_stream, GUAC_FILESYSTEM_NAME_LENGTH);
    Stream_Write(output_stream, GUAC_FILESYSTEM_NAME,
            GUAC_FILESYSTEM_NAME_LENGTH);

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_fs_process_query_full_size_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {

    guac_rdp_fs_info info = {0};
    guac_rdp_fs_get_info((guac_rdp_fs*) device->data, &info);

    wStream* output_stream = guac_rdpdr_new_io_completion(device,
            completion_id, STATUS_SUCCESS, 36);

    guac_client_log(device->rdpdr->client, GUAC_LOG_DEBUG,
            "%s: [file_id=%i]",
            __func__, file_id);

    Stream_Write_UINT32(output_stream, 32);
    Stream_Write_UINT64(output_stream, info.blocks_total);     /* TotalAllocationUnits */
    Stream_Write_UINT64(output_stream, info.blocks_available); /* CallerAvailableAllocationUnits */
    Stream_Write_UINT64(output_stream, info.blocks_available); /* ActualAvailableAllocationUnits */
    Stream_Write_UINT32(output_stream, 1);                     /* SectorsPerAllocationUnit */
    Stream_Write_UINT32(output_stream, info.block_size);       /* BytesPerSector */

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

