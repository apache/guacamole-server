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

#ifndef GUAC_FILE_H
#define GUAC_FILE_H

/**
 * Convenience functions for manipulating files.
 *
 * @file file.h
 */

#include "file-constants.h"
#include "file-types.h"

#include <stddef.h>
#include <sys/types.h>

struct guac_open_how {

    /**
     * Any flags that should be passed to the underlying call open(), openat(),
     * etc., as accepted by these functions' "oflags" parameter, such as
     * O_RDONLY or O_APPEND.
     */
    int oflags;

    /**
     * Any additional flags describing how the file should be opened. These
     * flags describe behavior that is not otherwise provided by open() or
     * openat().
     */
    guac_open_flag flags;

    /**
     * The file permissions (mode) that should be assigned to the file if it is
     * created as a result of this operation.
     */
    mode_t mode;

    /**
     * The buffer that should receive the generated filename if
     * GUAC_O_UNIQUE_SUFFIX is provided within flags.
     */
    void* filename;

    /**
     * The number of bytes available within the filename buffer.
     */
    size_t filename_size;

};

/**
 * Opens the given file located within the directory at the given path,
 * performing the operation as requested by the given guac_open_how structure.
 * The guac_open_how structure contains multiple types of flags that dictate
 * how the file should be opened, whether a lock should be acquired, etc.
 *
 * If a filename buffer is provided within the guac_open_how structure, and the
 * call to guac_openat() succeeds, that filename buffer will contain the
 * filename ultimately used to open the file (excluding the path), regardless
 * of whether the filename was modified from the value provided. NOTE: This
 * buffer may be touched even if the operation fails, and is not guaranteed to
 * be null terminated if the operation fails.
 *
 * If the GUAC_O_UNIQUE_SUFFIX flag is given in the guac_open_how structure, a
 * buffer for storage of the filename MUST be provided within that same
 * structure.
 *
 * If the operation succeeds, the resulting file descriptor is returned. If the
 * operation fails, -1 is returned and guac_error and guac_error_message are
 * set appropriately.
 *
 * @param path
 *     The path of the directory containing the file.
 *
 * @param filename
 *     The filename of the file to open within the directory at the given path.
 *
 * @param how
 *     A pointer to a guac_open_how structure that describes how the file
 *     should be opened.
 *
 * @return
 *     The file descriptor of the opened file, if successful, -1 otherwise.
 */
int guac_openat(const char* path, const char* filename,
        const guac_open_how* how);

#endif
