
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

void guac_rdpdr_fs_process_query_volume_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {

    wStream* output_stream = Stream_New(NULL, 38 + GUAC_FILESYSTEM_NAME_LENGTH);
    guac_rdpdr_fs_data* data = (guac_rdpdr_fs_data*) device->data;
    guac_rdpdr_fs_file* file = &(data->files[file_id]);

    /* Write header */
    Stream_Write_UINT16(output_stream, RDPDR_CTYP_CORE);
    Stream_Write_UINT16(output_stream, PAKID_CORE_DEVICE_IOCOMPLETION);

    /* Write content */
    Stream_Write_UINT32(output_stream, device->device_id);
    Stream_Write_UINT32(output_stream, completion_id);
    Stream_Write_UINT32(output_stream, STATUS_SUCCESS);

    Stream_Write_UINT32(output_stream, 18 + GUAC_FILESYSTEM_NAME_LENGTH);
    Stream_Write_UINT64(output_stream, WINDOWS_TIME(file->ctime)); /* VolumeCreationTime */
    Stream_Write(output_stream, "GUAC", 4);                        /* VolumeSerialNumber */
    Stream_Write_UINT32(output_stream, GUAC_FILESYSTEM_NAME_LENGTH);
    Stream_Write_UINT8(output_stream, FALSE); /* SupportsObjects */
    Stream_Write_UINT8(output_stream, 0);     /* Reserved */
    Stream_Write(output_stream, GUAC_FILESYSTEM_NAME, GUAC_FILESYSTEM_NAME_LENGTH);

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
    /* STUB */
    guac_client_log_error(device->rdpdr->client,
            "Unimplemented stub: guac_rdpdr_fs_query_attribute_info");
}

void guac_rdpdr_fs_process_query_full_size_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {
    /* STUB */
    guac_client_log_error(device->rdpdr->client,
            "Unimplemented stub: guac_rdpdr_fs_query_full_size_info");
}

