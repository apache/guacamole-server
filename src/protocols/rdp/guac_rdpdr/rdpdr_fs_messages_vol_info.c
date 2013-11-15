
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
#include "rdp_fs.h"
#include "rdp_status.h"
#include "rdpdr_service.h"
#include "client.h"
#include "debug.h"
#include "unicode.h"

#include <freerdp/utils/svc_plugin.h>

void guac_rdpdr_fs_process_query_volume_info(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {

    wStream* output_stream = guac_rdpdr_new_io_completion(device,
            completion_id, STATUS_SUCCESS, 21 + GUAC_FILESYSTEM_LABEL_LENGTH);

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
    /* STUB */
    guac_client_log_error(device->rdpdr->client,
            "Unimplemented stub: guac_rdpdr_fs_query_size_info");
}

void guac_rdpdr_fs_process_query_device_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {
    /* STUB */
    guac_client_log_error(device->rdpdr->client,
            "Unimplemented stub: guac_rdpdr_fs_query_devive_info");
}

void guac_rdpdr_fs_process_query_attribute_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {

    wStream* output_stream = guac_rdpdr_new_io_completion(device,
            completion_id, STATUS_SUCCESS, 16 + GUAC_FILESYSTEM_NAME_LENGTH);

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
            completion_id, STATUS_SUCCESS, 16 + GUAC_FILESYSTEM_NAME_LENGTH);

    Stream_Write_UINT64(output_stream, info.blocks_total);     /* TotalAllocationUnits */
    Stream_Write_UINT64(output_stream, info.blocks_available); /* CallerAvailableAllocationUnits */
    Stream_Write_UINT64(output_stream, info.blocks_available); /* ActualAvailableAllocationUnits */
    Stream_Write_UINT64(output_stream, 1);                     /* SectorsPerAllocationUnit */
    Stream_Write_UINT64(output_stream, info.block_size);       /* BytesPerSector */

    GUAC_RDP_DEBUG(2, "total=%i, avail=%i, size=%i", info.blocks_total, info.blocks_available, info.block_size);
    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

