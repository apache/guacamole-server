/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "config.h"

#include "file.h"
#include "file-download.h"
#include "file-ls.h"
#include "file-upload.h"

#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/string.h>

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Detect openat2() support. openat2() with RESOLVE_BENEATH | RESOLVE_NO_SYMLINKS
 * (Linux >= 5.6) resolves an entire path beneath a directory descriptor in a
 * single, atomic, symlink-refusing call, which is the strongest available
 * defense against path-traversal via symlinked/swapped components. The syscall
 * is invoked directly (glibc has no wrapper), and its absence at build time or
 * runtime (ENOSYS) is handled by falling back to an openat() component walk. */
#ifdef __linux__
#   if defined(__has_include)
#       if __has_include(<linux/openat2.h>)
#           include <linux/openat2.h>
#           include <sys/syscall.h>
#           if defined(SYS_openat2) && defined(RESOLVE_BENEATH) \
                    && defined(RESOLVE_NO_SYMLINKS)
#               define GUAC_SPICE_HAVE_OPENAT2 1
#           endif
#       endif
#   endif
#endif

/**
 * Translates an absolute path for a shared folder to an absolute path which is
 * within the real "shared folder" path specified in the connection settings.
 * No checking is performed on the path provided, which is assumed to have
 * already been normalized and validated as absolute.
 *
 * @param folder
 *     The folder containing the file whose path is being translated.
 *
 * @param virtual_path
 *     The absolute path to the file on the simulated folder, relative to the
 *     shared folder root.
 *
 * @param real_path
 *     The buffer in which to store the absolute path to the real file on the
 *     local filesystem.
 */
static void __guac_spice_folder_translate_path(guac_spice_folder* folder,
        const char* virtual_path, char* real_path) {

    guac_client_log(folder->client, GUAC_LOG_DEBUG, "%s: virtual_path=\"%s\", drive_path=\"%s\"", __func__, virtual_path, folder->path);

    /* Get drive path */
    char* path = folder->path;

    int i;

    /* Start with path from settings */
    for (i=0; i<GUAC_SPICE_FOLDER_MAX_PATH-1; i++) {

        /* Break on end-of-string */
        char c = *(path++);
        if (c == 0)
            break;

        /* Copy character */
        *(real_path++) = c;

    }

    /* Translate path */
    for (; i<GUAC_SPICE_FOLDER_MAX_PATH-1; i++) {

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

    guac_client_log(folder->client, GUAC_LOG_DEBUG, "%s: virtual_path=\"%s\", real_path=\"%s\"", __func__, virtual_path, real_path);

}

/**
 * Resolves the parent directory of the given normalized path, returning an
 * open directory descriptor for that parent. Every intermediate component is
 * opened relative to the previous one via openat() with O_NOFOLLOW and
 * O_DIRECTORY, starting from the trusted shared folder root descriptor, so no
 * symlinked or swapped component can cause resolution to escape the shared
 * folder. Intermediate directories are NOT created; they must already exist
 * (matching the original behavior, which only created the final component).
 *
 * @param root_fd
 *     A directory descriptor for the shared folder root. Must be valid (>= 0).
 *
 * @param normalized_path
 *     The normalized, absolute (leading '/') path, free of "." and ".."
 *     components, as produced by guac_spice_folder_normalize_path().
 *
 * @return
 *     An open directory descriptor for the parent directory of the final path
 *     component, which the caller must close(), or -1 on error with errno set.
 */
static int guac_spice_folder_resolve_parent_fd(int root_fd,
        const char* normalized_path) {

    /* Start at the shared folder root itself */
    int cur = openat(root_fd, ".",
            O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (cur < 0)
        return -1;

    /* Walk every component preceding the final one, descending one directory
     * at a time. The final component (after the last '/') is left for the
     * caller to open/create/unlink relative to the returned descriptor. */
    const char* p = normalized_path + 1;
    const char* final_slash = strrchr(normalized_path, '/');

    while (p < final_slash) {

        /* Isolate the next component */
        const char* slash = memchr(p, '/', final_slash - p);
        size_t len = slash ? (size_t) (slash - p) : (size_t) (final_slash - p);

        if (len == 0 || len >= GUAC_SPICE_FOLDER_MAX_PATH) {
            close(cur);
            errno = ENOENT;
            return -1;
        }

        char component[GUAC_SPICE_FOLDER_MAX_PATH];
        memcpy(component, p, len);
        component[len] = '\0';

        /* Descend into the component without following symlinks */
        int next = openat(cur, component,
                O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
        int saved_errno = errno;
        close(cur);

        if (next < 0) {
            errno = saved_errno;
            return -1;
        }

        cur = next;
        p = slash ? slash + 1 : final_slash;

    }

    return cur;

}

#ifdef GUAC_SPICE_HAVE_OPENAT2
/**
 * Thin wrapper around the openat2() syscall, which glibc does not expose.
 */
static int guac_spice_folder_openat2(int dirfd, const char* pathname,
        struct open_how* how, size_t size) {
    return syscall(SYS_openat2, dirfd, pathname, how, size);
}
#endif

/**
 * Opens the final target of the given normalized path beneath the shared
 * folder root, without following symlinks in any component. Where openat2() is
 * available, a single RESOLVE_BENEATH | RESOLVE_NO_SYMLINKS call is used;
 * otherwise the parent directory is resolved via an openat() component walk and
 * the final component is opened with O_NOFOLLOW. In both cases there is no gap
 * between confinement check and use.
 *
 * @param folder
 *     The shared folder whose root descriptor confines the open.
 *
 * @param normalized_path
 *     The normalized, absolute path to open, relative to the shared folder root.
 *
 * @param flags
 *     Standard POSIX open() flags.
 *
 * @param mode
 *     The mode to use when O_CREAT is set.
 *
 * @return
 *     An open file descriptor, or -1 on error with errno set.
 */
static int guac_spice_folder_confined_open(guac_spice_folder* folder,
        const char* normalized_path, int flags, mode_t mode) {

#ifdef GUAC_SPICE_HAVE_OPENAT2
    /* Fast path: resolve and open the whole path atomically beneath the root,
     * refusing every symlink and any escape above the root. */
    {
        const char* relative = normalized_path + 1;
        if (*relative == '\0')
            relative = ".";

        struct open_how how;
        memset(&how, 0, sizeof(how));
        how.flags = (uint64_t) (flags | O_CLOEXEC | O_NOFOLLOW);
        if (flags & O_CREAT)
            how.mode = (uint64_t) mode;
        how.resolve = RESOLVE_BENEATH | RESOLVE_NO_SYMLINKS;

        int fd = guac_spice_folder_openat2(folder->root_fd, relative,
                &how, sizeof(how));

        /* Only fall back if openat2() itself is unavailable at runtime; any
         * other result (including a genuine open error) is authoritative. */
        if (!(fd < 0 && errno == ENOSYS))
            return fd;
    }
#endif

    /* Fallback: resolve the parent via an openat() component walk, then open
     * the final component with O_NOFOLLOW. */
    int parent_fd = guac_spice_folder_resolve_parent_fd(folder->root_fd,
            normalized_path);
    if (parent_fd < 0)
        return -1;

    const char* basename = strrchr(normalized_path, '/') + 1;
    const char* target = (*basename == '\0') ? "." : basename;

    int fd = openat(parent_fd, target, flags | O_NOFOLLOW | O_CLOEXEC, mode);
    int saved_errno = errno;
    close(parent_fd);
    errno = saved_errno;

    return fd;

}

/**
 * Creates the final directory component of the given normalized path beneath
 * the shared folder root, resolving the parent without following symlinks.
 * Mirrors the semantics of the original mkdir()-based directory creation: an
 * existing directory is tolerated unless O_EXCL is set.
 *
 * @param folder
 *     The shared folder whose root descriptor confines the operation.
 *
 * @param normalized_path
 *     The normalized, absolute path of the directory to create.
 *
 * @param flags
 *     The open() flags requested; O_EXCL forces failure if the directory
 *     already exists.
 *
 * @return
 *     Zero on success (including a pre-existing directory when O_EXCL is not
 *     set), or -1 on error with errno set.
 */
static int guac_spice_folder_confined_mkdir(guac_spice_folder* folder,
        const char* normalized_path, int flags) {

    int parent_fd = guac_spice_folder_resolve_parent_fd(folder->root_fd,
            normalized_path);
    if (parent_fd < 0)
        return -1;

    const char* basename = strrchr(normalized_path, '/') + 1;

    /* The root itself always exists and cannot be (re)created */
    if (*basename == '\0') {
        close(parent_fd);
        errno = EEXIST;
        return (flags & O_EXCL) ? -1 : 0;
    }

    int result = mkdirat(parent_fd, basename, S_IRWXU);
    int saved_errno = errno;
    close(parent_fd);

    if (result) {
        if (saved_errno != EEXIST || (flags & O_EXCL)) {
            errno = saved_errno;
            return -1;
        }
    }

    return 0;

}

guac_spice_folder* guac_spice_folder_alloc(guac_client* client, const char* folder_path,
        int create_folder, int disable_download, int disable_upload) {

    guac_client_log(client, GUAC_LOG_DEBUG, "Initializing shared folder at "
            "\"%s\".", folder_path);

    /* Create folder if it does not exist */
    if (create_folder) {
        guac_client_log(client, GUAC_LOG_DEBUG,
               "%s: Creating folder \"%s\" if necessary.",
               __func__, folder_path);

        /* Log error if directory creation fails */
        if (mkdir(folder_path, S_IRWXU) && errno != EEXIST) {
            guac_client_log(client, GUAC_LOG_ERROR,
                    "Unable to create folder \"%s\": %s",
                    folder_path, strerror(errno));
        }
    }

    guac_spice_folder* folder = guac_mem_alloc(sizeof(guac_spice_folder));

    folder->client = client;
    folder->path = guac_strdup(folder_path);
    folder->file_id_pool = guac_pool_alloc(0);
    folder->open_files = 0;
    folder->disable_download = disable_download;
    folder->disable_upload = disable_upload;

    /* Open a trusted directory descriptor for the shared folder root. All
     * subsequent file operations resolve their targets relative to this
     * descriptor without following symlinks, closing the check/use race that
     * path-string-based confinement is subject to. O_NOFOLLOW is intentionally
     * NOT used here: the root itself is administrator-configured and may
     * legitimately be a symlink, exactly as the previous realpath()-based
     * confinement permitted. If this open fails, root_fd stays -1 and
     * operations fall back to the original path-based confinement so behavior
     * degrades safely rather than breaking. */
    folder->root_fd = open(folder_path, O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (folder->root_fd < 0)
        guac_client_log(client, GUAC_LOG_WARNING,
                "Unable to open shared folder root \"%s\" for confined access: "
                "%s. Falling back to path-based confinement.",
                folder_path, strerror(errno));

    /* Set up Download directory and watch it. */
    if (!disable_download) {

        guac_client_log(client, GUAC_LOG_DEBUG, "%s: Setting up Download/ folder watch.", __func__);

        if (create_folder) {
            guac_client_log(client, GUAC_LOG_DEBUG, "%s: Creating Download/ folder.",
                    __func__);

            char download_path[GUAC_SPICE_FOLDER_MAX_PATH];
            guac_strlcpy(download_path, folder_path, sizeof(download_path));
            guac_strlcat(download_path, "/Download", sizeof(download_path));

            if (mkdir(download_path, S_IRWXU) && errno != EEXIST) {
                guac_client_log(client, GUAC_LOG_ERROR,
                        "%s: Unable to create folder \"%s\": %s", __func__,
                        download_path, strerror(errno));
            }

        }

        /* The automatic download-on-create monitor is intentionally NOT
         * started. Its transfer action is unimplemented (dead code), and the
         * inotify thread held a raw pointer to this folder which
         * guac_spice_folder_free() releases at disconnect without stopping or
         * joining the thread — a use-after-free. If/when the feature is
         * completed, re-introduce the thread together with a proper shutdown
         * path (wake the blocking read via eventfd/self-pipe, pthread_join, and
         * close the inotify fd inside guac_spice_folder_free() before the
         * folder is freed). */

    }

    return folder;

}

void guac_spice_folder_free(guac_spice_folder* folder) {
    if (folder->root_fd >= 0)
        close(folder->root_fd);
    guac_pool_free(folder->file_id_pool);
    guac_mem_free(folder->path);
    guac_mem_free(folder);
}

guac_object* guac_spice_folder_alloc_object(guac_spice_folder *folder, guac_user* user) {
    
    /* Init folder */
    guac_object* folder_object = guac_user_alloc_object(user);
    folder_object->get_handler = guac_spice_file_download_get_handler;
    
    /* Assign upload handler only if uploads are not disabled. */
    if (!folder->disable_upload)
        folder_object->put_handler = guac_spice_file_upload_put_handler;
    
    folder_object->data = folder;

    /* Send filesystem to user */
    guac_protocol_send_filesystem(user->socket, folder_object, "Shared Folder");
    guac_socket_flush(user->socket);

    return folder_object;

}

int guac_spice_folder_append_filename(char* fullpath, const char* path,
        const char* filename) {

    int i;

    /* Disallow "." as a filename */
    if (strcmp(filename, ".") == 0)
        return 0;

    /* Disallow ".." as a filename */
    if (strcmp(filename, "..") == 0)
        return 0;

    /* Copy path, append trailing slash */
    for (i=0; i<GUAC_SPICE_FOLDER_MAX_PATH; i++) {

        /*
         * Append trailing slash only if:
         *  1) Trailing slash is not already present
         *  2) Path is non-empty
         */

        char c = path[i];
        if (c == '\0') {
            if (i > 0 && path[i-1] != '/' && path[i-1] != '\\')
                fullpath[i++] = '/';
            break;
        }

        /* Copy character if not end of string */
        fullpath[i] = c;

    }

    /* Append filename */
    for (; i<GUAC_SPICE_FOLDER_MAX_PATH; i++) {

        char c = *(filename++);
        if (c == '\0')
            break;

        /* Filenames may not contain slashes */
        if (c == '\\' || c == '/')
            return 0;

        /* Append each character within filename */
        fullpath[i] = c;

    }

    /* Verify path length is within maximum */
    if (i == GUAC_SPICE_FOLDER_MAX_PATH)
        return 0;

    /* Terminate path string */
    fullpath[i] = '\0';

    /* Append was successful */
    return 1;

}

const char* guac_spice_folder_basename(const char* path) {

    for (const char* c = path; *c != '\0'; c++) {

        /* Reset beginning of path if a path separator is found */
        if (*c == '/' || *c == '\\')
            path = c + 1;

    }

    /* path now points to the first character after the last path separator */
    return path;

}

void guac_spice_folder_close(guac_spice_folder* folder, int file_id) {

    guac_spice_folder_file* file = guac_spice_folder_get_file(folder, file_id);
    if (file == NULL) {
        guac_client_log(folder->client, GUAC_LOG_DEBUG,
                "%s: Ignoring close for bad file_id: %i",
                __func__, file_id);
        return;
    }

    file = &(folder->files[file_id]);

    guac_client_log(folder->client, GUAC_LOG_DEBUG,
            "%s: Closed \"%s\" (file_id=%i)",
            __func__, file->absolute_path, file_id);

    /* Close directory, if open */
    if (file->dir != NULL)
        closedir(file->dir);

    /* Close file */
    close(file->fd);

    /* Free paths */
    guac_mem_free(file->absolute_path);
    guac_mem_free(file->real_path);

    /* Free ID back to pool */
    guac_pool_free_int(folder->file_id_pool, file_id);
    folder->open_files--;

}

int guac_spice_folder_delete(guac_spice_folder* folder, int file_id) {

    /* Get file */
    guac_spice_folder_file* file = guac_spice_folder_get_file(folder, file_id);
    if (file == NULL) {
        guac_client_log(folder->client, GUAC_LOG_DEBUG,
                "%s: Delete of bad file_id: %i", __func__, file_id);
        return GUAC_SPICE_FOLDER_EINVAL;
    }

    /* Preferred path: remove the entry relative to its parent directory,
     * resolved beneath the trusted root descriptor without following symlinks.
     * This avoids re-resolving the retained real_path string, which is subject
     * to a check/use race if a component is swapped for a symlink after open. */
    if (folder->root_fd >= 0) {

        int parent_fd = guac_spice_folder_resolve_parent_fd(folder->root_fd,
                file->absolute_path);
        if (parent_fd < 0) {
            guac_client_log(folder->client, GUAC_LOG_DEBUG,
                    "%s: unable to resolve parent of \"%s\": %s", __func__,
                    file->absolute_path, strerror(errno));
            return guac_spice_folder_get_errorcode(errno);
        }

        const char* basename = guac_spice_folder_basename(file->absolute_path);
        int remove_result = unlinkat(parent_fd, basename,
                S_ISDIR(file->stmode) ? AT_REMOVEDIR : 0);
        int saved_errno = errno;
        close(parent_fd);

        if (remove_result) {
            guac_client_log(folder->client, GUAC_LOG_DEBUG,
                    "%s: unlinkat() failed: \"%s\"", __func__,
                    file->absolute_path);
            errno = saved_errno;
            return guac_spice_folder_get_errorcode(errno);
        }

        return 0;

    }

    /* Fallback path: operate on the retained real path string (used only when
     * the trusted root descriptor is unavailable; see guac_spice_folder_alloc) */

    /* If directory, attempt removal */
    if (S_ISDIR(file->stmode)) {
        if (rmdir(file->real_path)) {
            guac_client_log(folder->client, GUAC_LOG_DEBUG,
                    "%s: rmdir() failed: \"%s\"", __func__, file->real_path);
            return guac_spice_folder_get_errorcode(errno);
        }
    }

    /* Otherwise, attempt deletion */
    else if (unlink(file->real_path)) {
        guac_client_log(folder->client, GUAC_LOG_DEBUG,
                "%s: unlink() failed: \"%s\"", __func__, file->real_path);
        return guac_spice_folder_get_errorcode(errno);
    }

    return 0;

}

void* guac_spice_folder_expose(guac_user* user, void* data) {

    guac_spice_folder* folder = (guac_spice_folder*) data;

    guac_user_log(user, GUAC_LOG_DEBUG, "%s: Exposing folder \"%s\" to user.", __func__, folder->path);

    /* No need to expose if there is no folder or the user has left */
    if (user == NULL || folder == NULL)
        return NULL;

    /* Allocate and expose folder object for user */
    return guac_spice_folder_alloc_object(folder, user);

}

int guac_spice_folder_get_errorcode(int err) {

    /* Translate errno codes to GUAC_SPICE_FOLDER codes */
    switch(err) {
        case ENFILE:
            return GUAC_SPICE_FOLDER_ENFILE;

        case ENOENT:
            return GUAC_SPICE_FOLDER_ENOENT;

        case ENOTDIR:
            return GUAC_SPICE_FOLDER_ENOTDIR;

        case ENOSPC:
            return GUAC_SPICE_FOLDER_ENOSPC;

        case EISDIR:
            return GUAC_SPICE_FOLDER_EISDIR;

        case EACCES:
            return GUAC_SPICE_FOLDER_EACCES;
        
        case EEXIST:
            return GUAC_SPICE_FOLDER_EEXIST;

        case EINVAL:
            return GUAC_SPICE_FOLDER_EINVAL;

        case ENOSYS:
            return GUAC_SPICE_FOLDER_ENOSYS;

        case ENOTSUP:
            return GUAC_SPICE_FOLDER_ENOTSUP;

        default:
            return GUAC_SPICE_FOLDER_EINVAL;

    }

}

guac_spice_folder_file* guac_spice_folder_get_file(guac_spice_folder* folder,
        int file_id) {

    /* Validate ID */
    if (file_id < 0 || file_id >= GUAC_SPICE_FOLDER_MAX_FILES)
        return NULL;

    /* Return file at given ID */
    return &(folder->files[file_id]);

}

int guac_spice_folder_normalize_path(const char* path, char* abs_path) {

    int path_depth = 0;
    const char* path_components[GUAC_SPICE_FOLDER_MAX_PATH_DEPTH];

    /* If original path is not absolute, normalization fails */
    if (path[0] != '/')
        return 1;

    /* Create scratch copy of path excluding leading slash (we will be
     * replacing path separators with null terminators and referencing those
     * substrings directly as path components) */
    char path_scratch[GUAC_SPICE_FOLDER_MAX_PATH - 1];
    int length = guac_strlcpy(path_scratch, path + 1,
            sizeof(path_scratch));

    /* Fail if provided path is too long */
    if (length >= sizeof(path_scratch))
        return 1;

    /* Locate all path components within path */
    const char* current_path_component = &(path_scratch[0]);
    for (int i = 0; i <= length; i++) {

        /* If current character is a path separator, parse as component.
         * Both forward slashes and backslashes are treated as separators so
         * that backslash-delimited components (e.g. "..\..\..") cannot slip
         * through normalization unrecognized and later be rewritten into real
         * "../" traversal by __guac_spice_folder_translate_path(). */
        char c = path_scratch[i];
        if (c == '/' || c == '\\' || c == '\0') {

            /* Terminate current component */
            path_scratch[i] = '\0';

            /* If component refers to parent, just move up in depth */
            if (strcmp(current_path_component, "..") == 0) {
                if (path_depth > 0)
                    path_depth--;
            }

            /* Otherwise, if component not current directory, add to list */
            else if (strcmp(current_path_component, ".") != 0
                    && strcmp(current_path_component, "") != 0) {

                /* Fail normalization if path is too deep */
                if (path_depth >= GUAC_SPICE_FOLDER_MAX_PATH_DEPTH)
                    return 1;

                path_components[path_depth++] = current_path_component;

            }

            /* Update start of next component */
            current_path_component = &(path_scratch[i+1]);

        } /* end if separator */

        /* We do not currently support named streams */
        else if (c == ':')
            return 1;

    } /* end for each character */

    /* Add leading slash for resulting absolute path */
    abs_path[0] = '/';

    /* Append normalized components to path, separated by slashes */
    guac_strljoin(abs_path + 1, path_components, path_depth,
            "/", GUAC_SPICE_FOLDER_MAX_PATH - 1);

    return 0;

}

int guac_spice_folder_open(guac_spice_folder* folder, const char* path,
        int flags, bool overwrite, bool directory) {

    char real_path[GUAC_SPICE_FOLDER_MAX_PATH];
    char normalized_path[GUAC_SPICE_FOLDER_MAX_PATH];

    struct stat file_stat;
    int fd;
    int file_id;
    guac_spice_folder_file* file;

    guac_client_log(folder->client, GUAC_LOG_DEBUG,
            "%s: path=\"%s\", flags=0x%x, overwrite=0x%x, "
            "directory=0x%x", __func__, path, flags, overwrite, directory);

    /* If no files available, return too many open */
    if (folder->open_files >= GUAC_SPICE_FOLDER_MAX_FILES) {
        guac_client_log(folder->client, GUAC_LOG_DEBUG,
                "%s: Too many open files.",
                __func__);
        return GUAC_SPICE_FOLDER_ENFILE;
    }

    /* If path empty, return an error */
    if (path[0] == '\0')
        return GUAC_SPICE_FOLDER_EINVAL;

    /* If path is relative, the file does not exist */
    else if (path[0] != '\\' && path[0] != '/') {
        guac_client_log(folder->client, GUAC_LOG_DEBUG,
                "%s: Access denied - supplied path \"%s\" is relative.",
                __func__, path);
        return GUAC_SPICE_FOLDER_ENOENT;
    }

    /* Translate access into flags */
    if (directory)
        flags |= O_DIRECTORY;

    else if (overwrite)
        flags |= O_TRUNC;

    /* Normalize path, return no-such-file if invalid  */
    if (guac_spice_folder_normalize_path(path, normalized_path)) {
        guac_client_log(folder->client, GUAC_LOG_DEBUG,
                "%s: Normalization of path \"%s\" failed.", __func__, path);
        return GUAC_SPICE_FOLDER_ENOENT;
    }

    guac_client_log(folder->client, GUAC_LOG_DEBUG,
            "%s: Normalized path \"%s\" to \"%s\".",
            __func__, path, normalized_path);

        /* Translate normalized path to real path */
    __guac_spice_folder_translate_path(folder, normalized_path, real_path);

    guac_client_log(folder->client, GUAC_LOG_DEBUG,
            "%s: Translated path \"%s\" to \"%s\".",
            __func__, normalized_path, real_path);

    /* Preferred path: resolve and open the target relative to the trusted root
     * directory descriptor, without following symlinks in any component. This
     * is atomic with respect to the filesystem (there is no check/use gap a
     * concurrent writer could exploit by swapping a component for a symlink)
     * and confines the target beneath the shared folder root. */
    if (folder->root_fd >= 0) {

        /* Create directory first, if necessary */
        if (directory && (flags & O_CREAT)) {

            if (guac_spice_folder_confined_mkdir(folder, normalized_path,
                    flags)) {
                guac_client_log(folder->client, GUAC_LOG_DEBUG,
                        "%s: mkdirat() failed: %s",
                        __func__, strerror(errno));
                return guac_spice_folder_get_errorcode(errno);
            }

            /* Unset O_CREAT and O_EXCL as directory must exist before open() */
            flags &= ~(O_CREAT | O_EXCL);

        }

        guac_client_log(folder->client, GUAC_LOG_DEBUG,
                "%s: confined open: real_path=\"%s\", flags=0x%x",
                __func__, real_path, flags);

        fd = guac_spice_folder_confined_open(folder, normalized_path, flags,
                S_IRUSR | S_IWUSR);

        /* If open failed as we're trying to write a dir, retry read-only */
        if (fd == -1 && errno == EISDIR) {
            flags &= ~(O_WRONLY | O_RDWR);
            flags |= O_RDONLY;
            fd = guac_spice_folder_confined_open(folder, normalized_path,
                    flags, S_IRUSR | S_IWUSR);
        }

        if (fd == -1) {
            guac_client_log(folder->client, GUAC_LOG_DEBUG,
                    "%s: open() failed: %s", __func__, strerror(errno));
            return guac_spice_folder_get_errorcode(errno);
        }

    }

    /* Fallback path: used only when the trusted root descriptor could not be
     * opened at allocation time (see guac_spice_folder_alloc). This preserves
     * the original realpath()-based confinement so access degrades safely
     * rather than breaking outright. */
    else {

        /* Create directory first, if necessary */
        if (directory && (flags & O_CREAT)) {

            /* Create directory */
            if (mkdir(real_path, S_IRWXU)) {
                if (errno != EEXIST || (flags & O_EXCL)) {
                    guac_client_log(folder->client, GUAC_LOG_DEBUG,
                            "%s: mkdir() failed: %s",
                            __func__, strerror(errno));
                    return guac_spice_folder_get_errorcode(errno);
                }
            }

            /* Unset O_CREAT and O_EXCL as directory must exist before open() */
            flags &= ~(O_CREAT | O_EXCL);

        }

        /* Defense-in-depth confinement: canonicalize the parent directory of
         * the target and verify it resolves to within the shared folder root.
         * This catches any symlink (including intermediate path components,
         * which O_NOFOLLOW alone does not cover) that would resolve the target
         * outside of folder->path. By this point any directory to be opened has
         * already been created above, so the parent directory is guaranteed to
         * exist and can be canonicalized with realpath(). */
        {

            char parent_path[GUAC_SPICE_FOLDER_MAX_PATH];
            guac_strlcpy(parent_path, real_path, sizeof(parent_path));

            /* Split the final path component from its parent directory */
            char* last_slash = strrchr(parent_path, '/');
            if (last_slash == NULL) {
                guac_client_log(folder->client, GUAC_LOG_DEBUG,
                        "%s: Access denied - no path separator in real path "
                        "\"%s\".", __func__, real_path);
                return GUAC_SPICE_FOLDER_ENOENT;
            }

            /* Reduce to the parent directory, keeping the root "/" intact */
            if (last_slash == parent_path)
                parent_path[1] = '\0';
            else
                *last_slash = '\0';

            /* Canonicalize the parent directory and the shared folder root */
            char canonical_parent[PATH_MAX];
            char canonical_root[PATH_MAX];
            if (realpath(parent_path, canonical_parent) == NULL
                    || realpath(folder->path, canonical_root) == NULL) {
                guac_client_log(folder->client, GUAC_LOG_DEBUG,
                        "%s: Access denied - unable to canonicalize path "
                        "\"%s\".", __func__, real_path);
                return GUAC_SPICE_FOLDER_ENOENT;
            }

            /* Verify the canonical parent is the shared folder root itself or a
             * directory beneath it (prefix match bounded by a path separator,
             * so e.g. "/share-evil" does not pass as being under "/share"). The
             * root-slash exception covers a shared folder mounted at "/". */
            size_t root_len = strlen(canonical_root);
            if (strncmp(canonical_parent, canonical_root, root_len) != 0
                    || (canonical_root[root_len - 1] != '/'
                        && canonical_parent[root_len] != '\0'
                        && canonical_parent[root_len] != '/')) {
                guac_client_log(folder->client, GUAC_LOG_DEBUG,
                        "%s: Access denied - path \"%s\" resolves outside of "
                        "shared folder.", __func__, real_path);
                return GUAC_SPICE_FOLDER_ENOENT;
            }

        }

        guac_client_log(folder->client, GUAC_LOG_DEBUG,
                "%s: native open: real_path=\"%s\", flags=0x%x",
                __func__, real_path, flags);

        /* Open file. O_NOFOLLOW ensures a symlink placed as the final path
         * component is not followed, preventing symlink-based escapes out of
         * the shared folder. */
        fd = open(real_path, flags | O_NOFOLLOW, S_IRUSR | S_IWUSR);

        /* If open failed as we're trying to write a dir, retry read-only */
        if (fd == -1 && errno == EISDIR) {
            flags &= ~(O_WRONLY | O_RDWR);
            flags |= O_RDONLY;
            fd = open(real_path, flags | O_NOFOLLOW, S_IRUSR | S_IWUSR);
        }

        if (fd == -1) {
            guac_client_log(folder->client, GUAC_LOG_DEBUG,
                    "%s: open() failed: %s", __func__, strerror(errno));
            return guac_spice_folder_get_errorcode(errno);
        }

    }

    /* Get file ID, init file */
    file_id = guac_pool_next_int(folder->file_id_pool);
    file = &(folder->files[file_id]);
    file->id = file_id;
    file->fd  = fd;
    file->dir = NULL;
    file->dir_pattern[0] = '\0';
    file->absolute_path = guac_strdup(normalized_path);
    file->real_path = guac_strdup(real_path);
    file->bytes_written = 0;

    guac_client_log(folder->client, GUAC_LOG_DEBUG,
            "%s: Opened \"%s\" as file_id=%i",
            __func__, normalized_path, file_id);

    /* Attempt to pull file information */
    if (fstat(fd, &file_stat) == 0) {

        /* Load size and times */
        file->size  = file_stat.st_size;
        file->ctime = file_stat.st_ctime;
        file->mtime = file_stat.st_mtime;
        file->atime = file_stat.st_atime;
        file->stmode = file_stat.st_mode;

    }

    /* If information cannot be retrieved, fake it */
    else {

        /* Init information to 0, lacking any alternative */
        file->size  = 0;
        file->ctime = 0;
        file->mtime = 0;
        file->atime = 0;
        file->stmode = 0;

    }

    folder->open_files++;

    return file_id;

}

int guac_spice_folder_read(guac_spice_folder* folder, int file_id, uint64_t offset,
        void* buffer, int length) {

    guac_client_log(folder->client, GUAC_LOG_DEBUG, "%s: Attempt to read from file: %s", __func__, folder->path);

    int bytes_read;

    guac_spice_folder_file* file = guac_spice_folder_get_file(folder, file_id);
    if (file == NULL) {
        guac_client_log(folder->client, GUAC_LOG_DEBUG,
                "%s: Read from bad file_id: %i", __func__, file_id);
        return GUAC_SPICE_FOLDER_EINVAL;
    }

    /* Reject an offset which cannot be represented as an off_t, as it would
     * otherwise be truncated/wrapped to a different, in-bounds file position */
    if (offset > (uint64_t) INT64_MAX) {
        guac_client_log(folder->client, GUAC_LOG_DEBUG,
                "%s: Rejecting read at out-of-range offset", __func__);
        return GUAC_SPICE_FOLDER_EINVAL;
    }

    /* Attempt read at the requested offset */
    bytes_read = pread(file->fd, buffer, length, (off_t) offset);

    /* Translate errno on error */
    if (bytes_read < 0)
        return guac_spice_folder_get_errorcode(errno);

    return bytes_read;

}

const char* guac_spice_folder_read_dir(guac_spice_folder* folder, int file_id) {

    guac_client_log(folder->client, GUAC_LOG_DEBUG, "%s: Attempt to read directory: %s", __func__, folder->path);

    guac_spice_folder_file* file;

    struct dirent* result;

    /* Only read if file ID is valid */
    if (file_id < 0 || file_id >= GUAC_SPICE_FOLDER_MAX_FILES)
        return NULL;

    file = &(folder->files[file_id]);

    /* Open directory if not yet open, stop if error */
    if (file->dir == NULL) {
        file->dir = fdopendir(file->fd);
        if (file->dir == NULL)
            return NULL;
    }

    /* Read next entry, stop if error or no more entries */
    if ((result = readdir(file->dir)) == NULL)
        return NULL;

    /* Return filename */
    return result->d_name;

}

int guac_spice_folder_write(guac_spice_folder* folder, int file_id, uint64_t offset,
        void* buffer, int length) {

    guac_client_log(folder->client, GUAC_LOG_DEBUG, "%s: Attempt to write file: %s", __func__, folder->path);

    int bytes_written;

    guac_spice_folder_file* file = guac_spice_folder_get_file(folder, file_id);
    if (file == NULL) {
        guac_client_log(folder->client, GUAC_LOG_DEBUG,
                "%s: Write to bad file_id: %i", __func__, file_id);
        return GUAC_SPICE_FOLDER_EINVAL;
    }

    /* Reject an offset which cannot be represented as an off_t, as it would
     * otherwise be truncated/wrapped to a different, in-bounds file position */
    if (offset > (uint64_t) INT64_MAX) {
        guac_client_log(folder->client, GUAC_LOG_DEBUG,
                "%s: Rejecting write at out-of-range offset", __func__);
        return GUAC_SPICE_FOLDER_EINVAL;
    }

    /* Attempt write at the requested offset */
    bytes_written = pwrite(file->fd, buffer, length, (off_t) offset);

    /* Translate errno on error */
    if (bytes_written < 0)
        return guac_spice_folder_get_errorcode(errno);

    file->bytes_written += bytes_written;
    return bytes_written;

}

void guac_spice_client_file_transfer_handler(SpiceMainChannel* main_channel,
        SpiceFileTransferTask* task, guac_client* client) {
    
    guac_client_log(client, GUAC_LOG_DEBUG, "File transfer handler.");

}