
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

#include <inttypes.h>
#include <guacamole/pool.h>

#include "rdpdr_messages.h"
#include "rdp_fs.h"
#include "rdp_status.h"
#include "rdpdr_service.h"
#include "client.h"
#include "debug.h"
#include "unicode.h"

#include <freerdp/utils/svc_plugin.h>


void guac_rdpdr_fs_process_query_basic_info(guac_rdpdr_device* device, wStream* input_stream,
        int file_id, int completion_id) {

    wStream* output_stream;
    guac_rdp_fs_file* file;

    /* Get file */
    file = guac_rdp_fs_get_file((guac_rdp_fs*) device->data, file_id);
    if (file == NULL)
        return;

    GUAC_RDP_DEBUG(2, "[file_id=%i]", file_id);

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

    GUAC_RDP_DEBUG(2, "[file_id=%i]", file_id);

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

    GUAC_RDP_DEBUG(2, "[file_id=%i]", file_id);

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
    guac_rdp_utf16_to_utf8(Stream_Pointer(input_stream),
            destination_path, filename_length/2);

    GUAC_RDP_DEBUG(2, "[file_id=%i] destination_path=\"%s\"", file_id, destination_path);

    /* Perform rename */
    result = guac_rdp_fs_rename((guac_rdp_fs*) device->data, file_id,
            destination_path);
    if (result < 0)
        output_stream = guac_rdpdr_new_io_completion(device,
                completion_id, guac_rdp_fs_get_status(result), 4);
    else
        output_stream = guac_rdpdr_new_io_completion(device,
                completion_id, STATUS_SUCCESS, 4);

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

    GUAC_RDP_DEBUG(2, "[file_id=%i] size=%" PRIu64, file_id, (uint64_t) size);

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

    GUAC_RDP_DEBUG(2, "[file_id=%i]", file_id);

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

    GUAC_RDP_DEBUG(2, "[file_id=%i] size=%" PRIu64, file_id, (uint64_t) size);

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

    GUAC_RDP_DEBUG(2, "[file_id=%i] IGNORED", file_id);

    svc_plugin_send((rdpSvcPlugin*) device->rdpdr, output_stream);

}


