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

#ifndef GUAC_FILE_TYPES_H
#define GUAC_FILE_TYPES_H

/**
 * Types used by the functions defined by "file.h".
 *
 * @file file-types.h
 */

/**
 * Structure that defines how a file should be opened, analogous to the
 * open_how structure used by Linux' openat2() function.
 */
typedef struct guac_open_how guac_open_how;

/**
 * All flags supported by the guac_openat2() function.
 */
typedef enum guac_open_flag {

    /**
     * If the file already exists, a numeric suffix (".1", ".2", ".3", etc.)
     * should be used such that the file does not already exist.
     */
    GUAC_O_UNIQUE_SUFFIX = 1,

    /**
     * Once the file has been opened, it should be locked. If the file is
     * opened in read-only mode, this will be a read lock. The lock acquired is
     * otherwise a write lock.
     *
     * This flag is currently unimplemented and silently ignored under Windows.
     */
    GUAC_O_LOCKED = 2,

    /**
     * If the path containing the file does not yet exist, it should be
     * created. The directory created will be given "rwxr-x---" (0750)
     * permissions where possible (on non-Windows platforms).
     */
    GUAC_O_CREATE_PATH = 4

} guac_open_flag;

#endif
