
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

#include <guacamole/unicode.h>

#include "rdpdr_service.h"
#include "rdpdr_messages.h"
#include "rdp_fs.h"
#include "rdp_status.h"
#include "unicode.h"

void guac_rdpdr_fs_process_query_directory_info(guac_rdpdr_device* device,
        const char* entry_name, int file_id, int completion_id) {
    /* STUB */
    guac_client_log_info(device->rdpdr->client, "STUB: %s", __func__);
}

void guac_rdpdr_fs_process_query_full_directory_info(guac_rdpdr_device* device,
        const char* entry_name, int file_id, int completion_id) {
    /* STUB */
    guac_client_log_info(device->rdpdr->client, "STUB: %s", __func__);
}

void guac_rdpdr_fs_process_query_both_directory_info(guac_rdpdr_device* device,
        const char* entry_name, int file_id, int completion_id) {

    guac_rdp_fs_file* file;

    wStream* output_stream = Stream_New(NULL, 256);
    int length = guac_utf8_strlen(entry_name);
    int utf16_length = length*2;

    unsigned char utf16_entry_name[256];
    guac_rdp_utf8_to_utf16((const unsigned char*) entry_name, (char*) utf16_entry_name, length);

    /* Get file */
    file = guac_rdp_fs_get_file((guac_rdp_fs*) device->data, file_id);
    if (file == NULL)
        return;

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
    /* STUB */
    guac_client_log_info(device->rdpdr->client, "STUB: %s", __func__);
}

