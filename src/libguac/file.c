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

#include "file-private.h"
#include "guacamole/error.h"
#include "guacamole/file.h"
#include "guacamole/mem.h"
#include "guacamole/string.h"

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/**
 * Creates the directory with the given path. Where possible (non-Windows
 * platforms), this directory is given "rwxr-x---" (0750) permissions. If the
 * directory cannot be created, errno is set appropriately.
 *
 * @param path
 *     The full path of the directory to create.
 *
 * @return
 *     Zero if the directory was created successfully, non-zero otherwise.
 */
static int guac_mkdir(const char* path) {
#ifdef __MINGW32__
    return _mkdir(path);
#else
    return mkdir(path, S_IRWXU | S_IRGRP | S_IXGRP);
#endif
}

/**
 * Attempts to acquire a lock on the file associated with the given file
 * descriptor. The type of lock acquired is dictated by the read_lock flag. If
 * the lock cannot be acquired, errno is set appropriately.
 *
 * This function currently has no effect under Windows and simply returns
 * success.
 *
 * @param fd
 *     The file descriptor of the file to lock.
 *
 * @param read_lock
 *     Whether the lock acquired should be a read lock (non-zero) or a write
 *     lock (zero).
 *
 * @return
 *     Zero if the lock was successfully acquired, non-zero on error.
 */
static int guac_flock(int fd, int read_lock) {

#ifdef __MINGW32__
    return 0;
#else
    /* Translate requested file open flags (read-only vs. read/write) into
     * the relevant kind of lock */
    struct flock file_lock = {
        .l_type   = read_lock ? F_RDLCK : F_WRLCK,
        .l_whence = SEEK_SET,
        .l_start  = 0,
        .l_len    = 0,
        .l_pid    = getpid()
    };

    /* Abort if file cannot be locked */
    return fcntl(fd, F_SETLK, &file_lock) == -1;
#endif

}

int guac_is_filename(const char* filename) {

    /* Verify no references to current or parent directory */
    if (strcmp(filename, "..") == 0 || strcmp(filename, ".") == 0)
        return 0;

    /* Verify no path separators are present in filename */
    for (const char* current = filename; *current != '\0'; current++) {
        if (*current == '/' || *current == '\\')
            return 0;
    }

    return 1;

}

int guac_openat(const char* path, const char* filename,
        const guac_open_how* how) {

    int dir_fd = -1;
    int fd = -1;

    /* Verify filename does not contain any path separators, etc. (only the
     * path should be used as a path) */
    if (!guac_is_filename(filename))
        goto failed;

    /* Ensure path exists, creating if necessary and requested, fail if
     * impossible */
    if (how->flags & GUAC_O_CREATE_PATH) {
        if (guac_mkdir(path) && errno != EEXIST) {
            guac_error = GUAC_STATUS_SEE_ERRNO;
            guac_error_message = "Containing directory could not be created for file";
            goto failed;
        }
    }

    /* Access directory (resulting file descriptor will be used as the path for
     * the requested file) */
    dir_fd = open(path, O_RDONLY);
    if (dir_fd == -1) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Containing directory could not be opened";
        goto failed;
    }

    /* O_CREAT and O_EXCL should be implicit when a unique suffix is requested
     * (the unique suffix option only makes sense if creating exclusive files) */
    int oflags = how->oflags;
    if (how->flags & GUAC_O_UNIQUE_SUFFIX)
        oflags |= O_CREAT | O_EXCL;

    /* Always return a filename for the opened file if a filename buffer is
     * provided */
    size_t filename_length = 0;
    if (how->filename != NULL) {

        /* We at least need enough storage for the unaltered filename */
        filename_length = guac_strlcpy(how->filename, filename, how->filename_size);
        if (filename_length >= how->filename_size) {
            guac_error = GUAC_STATUS_RESULT_TOO_LARGE;
            guac_error_message = "Insufficient space in provided buffer for filename (even without suffix)";
            goto failed;
        }

    }

    /* Attempt to open requested file beneath specified path */
    fd = openat(dir_fd, filename, oflags, how->mode);
    if (fd == -1) {

        /* Fail now if there's nothing further we can try to resolve the
         * failure */
        if (!(how->flags & GUAC_O_UNIQUE_SUFFIX)) {
            guac_error = GUAC_STATUS_SEE_ERRNO;
            guac_error_message = "File could not be opened";
            goto failed;
        }

        /* Below here, GUAC_O_UNIQUE_SUFFIX is known to be set, and we will be
         * generating alternative filenames */

        /* We can only proceed if we have available storage for alternative
         * filenames */
        if (how->filename == NULL) {
            guac_error = GUAC_STATUS_INVALID_ARGUMENT;
            guac_error_message = "No filename buffer provided for adding unique suffix";
            goto failed;
        }

        /* We also need space for the smallest possible suffix (2 characters -
         * a single period followed by a single digit) */
        char* suffix = ((char*) how->filename) + filename_length;
        size_t suffix_size = guac_mem_ckd_sub_or_die(how->filename_size, filename_length);
        if (suffix_size <= 2) {
            guac_error = GUAC_STATUS_RESULT_TOO_LARGE;
            guac_error_message = "Insufficient space in provided buffer for filename and any suffix";
            goto failed;
        }

        /* Prepare filename for additional suffix, overwriting current null
         * terminator with the leading "." if that suffix */
        *(suffix++) = '.';
        suffix_size--;

        /* Try ".1", ".2", ".3", etc. until one succeeds or we give up due to
         * the sheer quantity of tries */
        for (int i = 1; fd == -1 && i <= GUAC_FILE_UNIQUE_SUFFIX_MAX; i++) {

            /* Generate and append numeric suffix (reusing common leading "."
             * and overwriting any numeric suffix from previous iterations) */
            if (snprintf(suffix, suffix_size, "%i", i) >= suffix_size) {
                guac_error = GUAC_STATUS_RESULT_TOO_LARGE;
                guac_error_message = "Insufficient space in provided buffer for filename and necessary suffix";
                goto failed;
            }

            /* Retry with newly suffixed filename */
            fd = openat(dir_fd, how->filename, oflags, how->mode);

        }

        /* Abort if we've run out of filenames */
        if (fd == -1) {
            guac_error = GUAC_STATUS_NOT_AVAILABLE;
            guac_error_message = "Exhausted all possible unique suffixes";
            goto failed;
        }

    } /* end if open failed */

    /* Explicit file locks are required only on POSIX platforms */
    if (how->flags & GUAC_O_LOCKED) {
        if (guac_flock(fd, (oflags & O_ACCMODE) == O_RDONLY)) {
            guac_error = GUAC_STATUS_SEE_ERRNO;
            guac_error_message = "File could not be locked";
            goto failed;
        }
    }

    close(dir_fd);
    return fd;

failed:

    if (dir_fd != -1)
        close(dir_fd);

    if (fd != -1)
        close(fd);

    return -1;

}
