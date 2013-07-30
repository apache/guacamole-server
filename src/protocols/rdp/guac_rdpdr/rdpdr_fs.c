
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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

/**
 * Translates an absolute Windows path to an absolute path which is within the "drive path"
 * specified in the connection settings.
 */
static char* __guac_rdpdr_fs_translate_path(guac_rdpdr_device* device,
        const char* path, char* path_buffer) {

    /* Get drive path */
    rdp_guac_client_data* client_data = (rdp_guac_client_data*) device->rdpdr->client->data;
    char* drive_path = client_data->settings.drive_path;

    int i;

    /* Start with path from settings */
    for (i=0; i<GUAC_RDPDR_FS_MAX_PATH-1; i++) {

        /* Copy character, break on end-of-string */
        if ((*(path_buffer++) = *(drive_path++)) == 0)
            break;

    }

    /* Translate path */
    for (; i<GUAC_RDPDR_FS_MAX_PATH-1; i++) {

        /* Stop at end of string */
        char c = *path;
        if (c == 0)
            break;

        /* Disallow ".." */
        if (c == '.' && *(path-1) == '.')
            return NULL;

        /* Translate backslashes to forward slashes */
        if (c == '\\')
            c = '/';

        /* Store in real path buffer */
        path_buffer[i] = c;

        /* Next path character */
        path++;

    }

    /* Null terminator */
    path_buffer[i] = 0;
    return path_buffer;

}

int guac_rdpdr_fs_open(guac_rdpdr_device* device, const char* path,
        int access, int create_disposition) {

    guac_rdpdr_fs_data* data = (guac_rdpdr_fs_data*) device->data;
    char path_buffer[GUAC_RDPDR_FS_MAX_PATH];

    int fd;
    int file_id;
    guac_rdpdr_fs_file* file;

    mode_t mode;
    int flags = 0;

    /* If no files available, return too many open */
    if (data->open_files >= GUAC_RDPDR_FS_MAX_FILES)
        return GUAC_RDPDR_FS_ENFILE;

    /* If path is empty or relative, the file does not exist */
    if (path[0] != '\\')
        return GUAC_RDPDR_FS_ENOENT;

    /* Translate access into mode */
    if (access & ACCESS_FILE_READ_DATA) {
        if (access & (ACCESS_FILE_WRITE_DATA | ACCESS_FILE_APPEND_DATA))
            mode = O_RDWR;
        else
            mode = O_RDONLY;
    }
    else if (access & (ACCESS_FILE_WRITE_DATA | ACCESS_FILE_APPEND_DATA))
        mode = O_WRONLY;
    else
        return GUAC_RDPDR_FS_ENOENT; /* FIXME: Replace with real return value */

    /* If append access requested, add appropriate option */
    if (access & ACCESS_FILE_APPEND_DATA)
        flags |= O_APPEND;

    switch (create_disposition) {

        /* Supersede (replace) if exists, otherwise create */
        case DISP_FILE_SUPERSEDE:
            flags |= O_TRUNC | O_CREAT;
            break;

        /* Open file if exists and do not overwrite, fail otherwise */
        case DISP_FILE_OPEN:
            flags |= O_APPEND;
            break;

        /* Create if not exist, fail otherwise */
        case DISP_FILE_CREATE:
            flags |= O_EXCL;
            break;

        /* Open if exists, create otherwise */
        case DISP_FILE_OPEN_IF:
            flags |= O_APPEND | O_CREAT;
            break;

        /* Overwrite if exists, fail otherwise */
        case DISP_FILE_OVERWRITE:
            /* No flag necessary - default functionality of open */
            break;

        /* Overwrite if exists, create otherwise */
        case DISP_FILE_OVERWRITE_IF:
            flags |= O_CREAT;
            break;

        /* Unrecognised disposition */
        default:
            return GUAC_RDPDR_FS_ENOENT; /* FIXME: Replace with real return value */

    }

    /* If path invalid, return no such file */
    if (__guac_rdpdr_fs_translate_path(device, path, path_buffer) == NULL)
        return GUAC_RDPDR_FS_ENOENT;

    guac_client_log_info(device->rdpdr->client, "Path translated to \"%s\"", path_buffer);

    /* Open file */
    fd = open(path_buffer, flags, mode);
    if (fd == -1)
        return GUAC_RDPDR_FS_ENOENT;

    /* Get file ID, init file */
    file_id = guac_pool_next_int(data->file_id_pool);
    file = &(data->files[file_id]);
    file->fd = fd;

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

