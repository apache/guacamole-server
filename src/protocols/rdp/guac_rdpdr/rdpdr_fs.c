
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

int guac_rdpdr_fs_open(guac_rdpdr_device* device, const char* path) {

    guac_rdpdr_fs_data* data = (guac_rdpdr_fs_data*) device->data;

    int file_id;
    guac_rdpdr_fs_file* file;

    /* If no files available, return too many open */
    if (data->open_files >= GUAC_RDPDR_FS_MAX_FILES)
        return GUAC_RDPDR_FS_ENFILE;

    /* If path is empty, the file does not exist */
    if (path[0] == '\0')
        return GUAC_RDPDR_FS_ENOENT;

    file->type = GUAC_RDPDR_FS_FILE;
    /* STUB */

    /* Get file ID */
    file_id = guac_pool_next_int(data->file_id_pool);
    file = &(data->files[file_id]);

    data->open_files++;

    return file_id;

}

void guac_rdpdr_fs_close(guac_rdpdr_device* device, int file_id) {

    guac_rdpdr_fs_data* data = (guac_rdpdr_fs_data*) device->data;

    /* Only close if file ID is valid */
    if (file_id >= 0 && file_id <= GUAC_RDPDR_FS_MAX_FILES-1) {
        guac_pool_free_int(data->file_id_pool, file_id);
        data->open_files--;
    }

}

