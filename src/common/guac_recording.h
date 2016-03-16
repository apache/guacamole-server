/*
 * Copyright (C) 2016 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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

