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

#include "guac_recording.h"

#include <guacamole/client.h>
#include <guacamole/socket.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/**
 * Attempts to open a new recording within the given path and having the given
 * name. If such a file already exists, sequential numeric suffixes (.1, .2,
 * .3, etc.) are appended until a filename is found which does not exist (or
 * until the maximum number of numeric suffixes has been tried). If the file
 * absolutely cannot be opened due to an error, -1 is returned and errno is set
 * appropriately.
 *
 * @param path
 *     The full path to the directory in which the data file should be created.
 *
 * @param name
 *     The name of the data file which should be crated within the given path.
 *
 * @param basename
 *     A buffer in which the path, a path separator, the filename, any
 *     necessary suffix, and a NULL terminator will be stored. If insufficient
 *     space is available, -1 will be returned, and errno will be set to
 *     ENAMETOOLONG.
 *
 * @param basename_size
 *     The number of bytes available within the provided basename buffer.
 *
 * @return
 *     The file descriptor of the open data file if open succeeded, or -1 on
 *     failure.
 */
static int guac_common_recording_open(const char* path,
        const char* name, char* basename, int basename_size) {

    int i;

    /* Concatenate path and name (separated by a single slash) */
    int basename_length = snprintf(basename,
            basename_size - GUAC_COMMON_RECORDING_MAX_SUFFIX_LENGTH,
            "%s/%s", path, name);

    /* Abort if maximum length reached */
    if (basename_length ==
            basename_size - GUAC_COMMON_RECORDING_MAX_SUFFIX_LENGTH) {
        errno = ENAMETOOLONG;
        return -1;
    }

    /* Attempt to open recording */
    int fd = open(basename,
            O_CREAT | O_EXCL | O_WRONLY,
            S_IRUSR | S_IWUSR);

    /* Continuously retry with alternate names on failure */
    if (fd == -1) {

        /* Prepare basename for additional suffix */
        basename[basename_length] = '.';
        char* suffix = &(basename[basename_length + 1]);

        /* Continue retrying alternative suffixes if file already exists */
        for (i = 1; fd == -1 && errno == EEXIST
                && i <= GUAC_COMMON_RECORDING_MAX_SUFFIX; i++) {

            /* Append new suffix */
            sprintf(suffix, "%i", i);

            /* Retry with newly-suffixed filename */
            fd = open(basename,
                    O_CREAT | O_EXCL | O_WRONLY,
                    S_IRUSR | S_IWUSR);

        }

        /* Abort if we've run out of filenames */
        if (fd == -1)
            return -1;

    } /* end if open succeeded */

    /* Lock entire output file for writing by the current process */
    struct flock file_lock = {
        .l_type   = F_WRLCK,
        .l_whence = SEEK_SET,
        .l_start  = 0,
        .l_len    = 0,
        .l_pid    = getpid()
    };

    /* Abort if file cannot be locked for reading */
    if (fcntl(fd, F_SETLK, &file_lock) == -1) {
        close(fd);
        return -1;
    }

    return fd;

}

int guac_common_recording_create(guac_client* client, const char* path,
        const char* name, int create_path) {

    char filename[GUAC_COMMON_RECORDING_MAX_NAME_LENGTH];

    /* Create path if it does not exist, fail if impossible */
    if (create_path && mkdir(path, S_IRWXU) && errno != EEXIST) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "Creation of recording failed: %s", strerror(errno));
        return 1;
    }

    /* Attempt to open recording file */
    int fd = guac_common_recording_open(path, name, filename, sizeof(filename));
    if (fd == -1) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "Creation of recording failed: %s", strerror(errno));
        return 1;
    }

    /* Replace client socket with wrapped socket */
    client->socket = guac_socket_tee(client->socket, guac_socket_open(fd));

    /* Recording creation succeeded */
    guac_client_log(client, GUAC_LOG_INFO,
            "Recording of session will be saved to \"%s\".",
            filename);

    return 0;

}

