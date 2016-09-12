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

#ifndef GUAC_COMMON_RECORDING_H
#define GUAC_COMMON_RECORDING_H

#include <guacamole/client.h>

/**
 * The maximum numeric value allowed for the .1, .2, .3, etc. suffix appended
 * to the end of the session recording filename if a recording having the
 * requested name already exists.
 */
#define GUAC_COMMON_RECORDING_MAX_SUFFIX 255

/**
 * The maximum length of the string containing a sequential numeric suffix
 * between 1 and GUAC_COMMON_RECORDING_MAX_SUFFIX inclusive, in bytes,
 * including NULL terminator.
 */
#define GUAC_COMMON_RECORDING_MAX_SUFFIX_LENGTH 4

/**
 * The maximum overall length of the full path to the session recording file,
 * including any additional suffix and NULL terminator, in bytes.
 */
#define GUAC_COMMON_RECORDING_MAX_NAME_LENGTH 2048

/**
 * Replaces the socket of the given client such that all further Guacamole
 * protocol output will be copied into a file within the given path and having
 * the given name. If the create_path flag is non-zero, the given path will be
 * created if it does not yet exist. If creation of the recording file or path
 * fails, error messages will automatically be logged, and no recording will be
 * written. The recording will automatically be closed once the client is
 * freed.
 *
 * @param client
 *     The client whose output should be copied to a recording file.
 *
 * @param path
 *     The full absolute path to a directory in which the recording file should
 *     be created.
 *
 * @param name
 *     The base name to use for the recording file created within the specified
 *     path.
 *
 * @param create_path
 *     Zero if the specified path MUST exist for the recording file to be
 *     written, or non-zero if the path should be created if it does not yet
 *     exist.
 *
 * @return
 *     Zero if the recording file has been successfully created and a recording
 *     will be written, non-zero otherwise.
 */
int guac_common_recording_create(guac_client* client, const char* path,
        const char* name, int create_path);

#endif

