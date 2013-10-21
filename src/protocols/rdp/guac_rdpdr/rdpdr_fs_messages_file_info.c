
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


void guac_rdpdr_fs_process_query_basic_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {

    wStream* output_stream = Stream_New(NULL, 60);
    guac_rdpdr_fs_file* file;

    /* Get file */
    file = guac_rdpdr_fs_get_file(device, file_id);
    if (file == NULL)
        return;

    /* Write header */
    Stream_Write_UINT16(output_stream, RDPDR_CTYP_CORE);
    Stream_Write_UINT16(output_stream, PAKID_CORE_DEVICE_IOCOMPLETION);

    /* Write content */
    Stream_Write_UINT32(output_stream, device->device_id);
    Stream_Write_UINT32(output_stream, completion_id);
    Stream_Write_UINT32(output_stream, STATUS_SUCCESS);

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

    wStream* output_stream = Stream_New(NULL, 60);
    guac_rdpdr_fs_file* file;
    BOOL is_directory = FALSE;

    /* Get file */
    file = guac_rdpdr_fs_get_file(device, file_id);
    if (file == NULL)
        return;

    if (file->attributes & FILE_ATTRIBUTE_DIRECTORY)
        is_directory = TRUE;

    /* Write header */
    Stream_Write_UINT16(output_stream, RDPDR_CTYP_CORE);
    Stream_Write_UINT16(output_stream, PAKID_CORE_DEVICE_IOCOMPLETION);

    /* Write content */
    Stream_Write_UINT32(output_stream, device->device_id);
    Stream_Write_UINT32(output_stream, completion_id);
    Stream_Write_UINT32(output_stream, STATUS_SUCCESS);

    Stream_Write_UINT32(output_stream, 22);
    Stream_Write_UINT64(output_stream, file->size);   /* AllocationSize */
    Stream_Write_UINT64(output_stream, file->size);   /* EndOfFile      */
    Stream_Write_UINT32(output_stream, 1);            /* NumberOfLinks  */
    Stream_Write_UINT8(output_stream,  0);            /* DeletePending  */
    Stream_Write_UINT8(output_stream,  is_directory); /* Directory      */

    /* Reserved field must not be sent */

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}

void guac_rdpdr_fs_process_query_attribute_tag_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {
    /* STUB */
    guac_client_log_error(device->rdpdr->client,
            "Unimplemented stub: guac_rdpdr_fs_query_attribute_tag_info");
}

void guac_rdpdr_fs_process_set_rename_info(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {
    /* STUB */
    guac_client_log_error(device->rdpdr->client,
            "Unimplemented stub: %s", __func__);
}

void guac_rdpdr_fs_process_set_allocation_info(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {
    /* STUB */
    guac_client_log_error(device->rdpdr->client,
            "Unimplemented stub: %s", __func__);
}

void guac_rdpdr_fs_process_set_disposition_info(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {
    /* STUB */
    guac_client_log_error(device->rdpdr->client,
            "Unimplemented stub: %s", __func__);
}

void guac_rdpdr_fs_process_set_end_of_file_info(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {
    /* STUB */
    guac_client_log_error(device->rdpdr->client,
            "Unimplemented stub: %s", __func__);
}

void guac_rdpdr_fs_process_set_basic_info(guac_rdpdr_device* device,
        wStream* input_stream, int file_id, int completion_id) {
    /* STUB */
    guac_client_log_error(device->rdpdr->client,
            "Unimplemented stub: %s", __func__);
}


