
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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>
#include <fnmatch.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <guacamole/pool.h>

#include "rdpdr_messages.h"
#include "rdpdr_fs.h"
#include "rdpdr_service.h"
#include "client.h"
#include "unicode.h"

#include <freerdp/utils/svc_plugin.h>

/**
 * Translates an absolute Windows virtual_path to an absolute virtual_path
 * which is within the "drive virtual_path" specified in the connection
 * settings.
 */
static void __guac_rdpdr_fs_translate_path(guac_rdpdr_device* device,
        const char* virtual_path, char* real_path) {

    /* Get drive path */
    rdp_guac_client_data* client_data = (rdp_guac_client_data*) device->rdpdr->client->data;
    char* drive_path = client_data->settings.drive_path;

    int i;

    /* Start with path from settings */
    for (i=0; i<GUAC_RDPDR_FS_MAX_PATH-1; i++) {

        /* Break on end-of-string */
        char c = *(drive_path++);
        if (c == 0)
            break;

        /* Copy character */
        *(real_path++) = c;

    }

    /* Translate path */
    for (; i<GUAC_RDPDR_FS_MAX_PATH-1; i++) {

        /* Stop at end of string */
        char c = *(virtual_path++);
        if (c == 0)
            break;

        /* Translate backslashes to forward slashes */
        if (c == '\\')
            c = '/';

        /* Store in real path buffer */
        *(real_path++)= c;

    }

    /* Null terminator */
    *real_path = 0;

}

int guac_rdpdr_fs_open(guac_rdpdr_device* device, const char* path,
        int access, int file_attributes, int create_disposition,
        int create_options) {

    guac_rdpdr_fs_data* data = (guac_rdpdr_fs_data*) device->data;
    char real_path[GUAC_RDPDR_FS_MAX_PATH];
    char normalized_path[GUAC_RDPDR_FS_MAX_PATH];

    struct stat file_stat;
    int fd;
    int file_id;
    guac_rdpdr_fs_file* file;

    int flags = 0;

    /* If no files available, return too many open */
    if (data->open_files >= GUAC_RDPDR_FS_MAX_FILES)
        return GUAC_RDPDR_FS_ENFILE;

    /* If path empty, transform to root path */
    if (path[0] == '\0')
        path = "\\";

    /* If path is relative, the file does not exist */
    else if (path[0] != '\\')
        return GUAC_RDPDR_FS_ENOENT;

    /* Translate access into flags */
    if (access & ACCESS_GENERIC_ALL)
        flags = O_RDWR;
    else if ((access & (ACCESS_GENERIC_WRITE | ACCESS_FILE_WRITE_DATA))
          && (access & (ACCESS_GENERIC_READ  | ACCESS_FILE_READ_DATA)))
        flags = O_RDWR;
    else if (access & (ACCESS_GENERIC_WRITE | ACCESS_FILE_WRITE_DATA))
        flags = O_WRONLY;
    else
        flags = O_RDONLY;

    /* If append access requested, add appropriate option */
    if (access & ACCESS_FILE_APPEND_DATA)
        flags |= O_APPEND;

    /* Normalize path, return no-such-file if invalid  */
    if (guac_rdpdr_fs_normalize_path(path, normalized_path))
        return GUAC_RDPDR_FS_ENOENT;

    /* Translate normalized path to real path */
    __guac_rdpdr_fs_translate_path(device, normalized_path, real_path);

    switch (create_disposition) {

        /* Create if not exist, fail otherwise */
        case DISP_FILE_CREATE:
            flags |= O_CREAT | O_EXCL;
            break;

        /* Open file if exists and do not overwrite, fail otherwise */
        case DISP_FILE_OPEN:
            /* No flag necessary - default functionality of open */
            break;

        /* Open if exists, create otherwise */
        case DISP_FILE_OPEN_IF:
            flags |= O_CREAT;
            break;

        /* Overwrite if exists, fail otherwise */
        case DISP_FILE_OVERWRITE:
            flags |= O_TRUNC;
            break;

        /* Overwrite if exists, create otherwise */
        case DISP_FILE_OVERWRITE_IF:
            flags |= O_CREAT | O_TRUNC;
            break;

        /* Supersede (replace) if exists, otherwise create */
        case DISP_FILE_SUPERSEDE:
            unlink(real_path);
            flags |= O_CREAT | O_TRUNC;
            break;

        /* Unrecognised disposition */
        default:
            return GUAC_RDPDR_FS_ENOENT; /* FIXME: Replace with real return value */

    }

    /* Open file */
    fd = open(real_path, flags, S_IRUSR | S_IWUSR);
    if (fd == -1)
        return GUAC_RDPDR_FS_ENOENT;

    /* Get file ID, init file */
    file_id = guac_pool_next_int(data->file_id_pool);
    file = &(data->files[file_id]);
    file->fd  = fd;
    file->dir = NULL;
    file->dir_pattern[0] = '\0';
    file->absolute_path = strdup(normalized_path);

    /* Attempt to pull file information */
    if (fstat(fd, &file_stat) == 0) {

        /* Load size and times */
        file->size  = file_stat.st_size;
        file->ctime = WINDOWS_TIME(file_stat.st_ctime);
        file->mtime = WINDOWS_TIME(file_stat.st_mtime);
        file->atime = WINDOWS_TIME(file_stat.st_atime);

        /* Set type */
        if (S_ISDIR(file_stat.st_mode))
            file->attributes = FILE_ATTRIBUTE_DIRECTORY;
        else
            file->attributes = FILE_ATTRIBUTE_NORMAL;

    }

    /* If information cannot be retrieved, fake it */
    else {

        guac_client_log_info(device->rdpdr->client,
                "Unable to read information for \"%s\"",
                real_path);

        /* Init information to 0, lacking any alternative */
        file->size  = 0;
        file->ctime = 0;
        file->mtime = 0;
        file->atime = 0;
        file->attributes = FILE_ATTRIBUTE_NORMAL;

    }

    data->open_files++;

    return file_id;

}

void guac_rdpdr_fs_close(guac_rdpdr_device* device, int file_id) {

    guac_rdpdr_fs_data* data = (guac_rdpdr_fs_data*) device->data;
    guac_rdpdr_fs_file* file;

    /* Only close if file ID is valid */
    if (file_id < 0 || file_id >= GUAC_RDPDR_FS_MAX_FILES)
        return;

    file = &(data->files[file_id]);

    /* Close directory, if open */
    if (file->dir != NULL)
        closedir(file->dir);

    /* Close file */
    close(file->fd);

    /* Free name */
    free(file->absolute_path);

    /* Free ID back to pool */
    guac_pool_free_int(data->file_id_pool, file_id);
    data->open_files--;

}

const char* guac_rdpdr_fs_read_dir(guac_rdpdr_device* device, int file_id) {

    guac_rdpdr_fs_data* data = (guac_rdpdr_fs_data*) device->data;
    guac_rdpdr_fs_file* file;

    struct dirent* result;

    /* Only read if file ID is valid */
    if (file_id < 0 || file_id >= GUAC_RDPDR_FS_MAX_FILES)
        return NULL;

    file = &(data->files[file_id]);

    /* Open directory if not yet open, stop if error */
    if (file->dir == NULL) {
        file->dir = fdopendir(file->fd);
        if (file->dir == NULL)
            return NULL;
    }

    /* Read next entry, stop if error */
    if (readdir_r(file->dir, &(file->__dirent), &result))
        return NULL;

    /* If no more entries, return NULL */
    if (result == NULL)
        return NULL;

    /* Return filename */
    return file->__dirent.d_name;

}

int guac_rdpdr_fs_normalize_path(const char* path, char* abs_path) {

    int i;
    int path_depth = 0;
    char path_component_data[GUAC_RDPDR_FS_MAX_PATH];
    const char* path_components[64];

    const char** current_path_component      = &(path_components[0]);
    const char*  current_path_component_data = &(path_component_data[0]);

    /* If original path is not absolute, normalization fails */
    if (path[0] != '\\' && path[0] != '/')
        return 1;

    /* Skip past leading slash */
    path++;

    /* Copy path into component data for parsing */
    strncpy(path_component_data, path, GUAC_RDPDR_FS_MAX_PATH-1);

    /* Find path components within path */
    for (i=0; i<GUAC_RDPDR_FS_MAX_PATH; i++) {

        /* If current character is a path separator, parse as component */
        char c = path_component_data[i];
        if (c == '/' || c == '\\' || c == 0) {

            /* Terminate current component */
            path_component_data[i] = 0;

            /* If component refers to parent, just move up in depth */
            if (strcmp(current_path_component_data, "..") == 0) {
                if (path_depth > 0)
                    path_depth--;
            }

            /* Otherwise, if component not current directory, add to list */
            else if (strcmp(current_path_component_data,   ".") != 0
                     && strcmp(current_path_component_data, "") != 0)
                path_components[path_depth++] = current_path_component_data;

            /* If end of string, stop */
            if (c == 0)
                break;

            /* Update start of next component */
            current_path_component_data = &(path_component_data[i+1]);

        } /* end if separator */

    } /* end for each character */

    /* If no components, the path is simply root */
    if (path_depth == 0) {
        strcpy(abs_path, "\\");
        return 0;
    }

    /* Ensure last component is null-terminated */
    path_component_data[i] = 0;

    /* Convert components back into path */
    for (; path_depth > 0; path_depth--) {

        const char* filename = *(current_path_component++);

        /* Add separator */
        *(abs_path++) = '\\';

        /* Copy string */
        while (*filename != 0)
            *(abs_path++) = *(filename++);

    }

    /* Terminate absolute path */
    *(abs_path++) = 0;
    return 0;

}

int guac_rdpdr_fs_convert_path(const char* parent, const char* rel_path, char* abs_path) {

    int i;
    char combined_path[GUAC_RDPDR_FS_MAX_PATH];
    char* current = combined_path;

    /* Copy parent path */
    for (i=0; i<GUAC_RDPDR_FS_MAX_PATH; i++) {

        char c = *(parent++);
        if (c == 0)
            break;

        *(current++) = c;

    }

    /* Add trailing slash */
    *(current++) = '\\';

    /* Copy remaining path */
    strncpy(current, rel_path, GUAC_RDPDR_FS_MAX_PATH-i-2);

    /* Normalize into provided buffer */
    return guac_rdpdr_fs_normalize_path(combined_path, abs_path);

}

guac_rdpdr_fs_file* guac_rdpdr_fs_get_file(guac_rdpdr_device* device,
        int file_id) {

    /* Validate ID */
    guac_rdpdr_fs_data* data = (guac_rdpdr_fs_data*) device->data;
    if (file_id < 0 || file_id >= GUAC_RDPDR_FS_MAX_FILES)
        return NULL;

    /* Return file at given ID */
    return &(data->files[file_id]);

}

int guac_rdpdr_fs_matches(const char* filename, const char* pattern) {
    return fnmatch(pattern, filename, FNM_NOESCAPE) != 0;
}

