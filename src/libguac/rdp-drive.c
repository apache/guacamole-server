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

#include "guacamole/rdp-drive.h"

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>

/**
 * The process-wide RDP redirected-drive open-file limit. Connection
 * processes inherit this value across fork().
 */
static int rdp_drive_max_files = GUAC_RDP_DRIVE_MAX_FILES_DEFAULT;

/**
 * Returns whether max_files is a limit the RDP plugin can allocate.
 *
 * @param max_files
 *     The candidate limit.
 *
 * @return
 *     Non-zero if max_files is a positive integer no greater than
 *     GUAC_RDP_DRIVE_MAX_FILES_LIMIT, zero otherwise.
 */
static int guac_rdp_drive_max_files_supported(int max_files) {
    return max_files > 0 && max_files <= GUAC_RDP_DRIVE_MAX_FILES_LIMIT;
}

int guac_rdp_drive_parse_max_files(const char* value) {

    char* end;
    const char* current;
    long max_files;

    if (value == NULL || *value == '\0')
        return -1;

    for (current = value; *current != '\0'; current++) {
        if (!isdigit((unsigned char) *current))
            return -1;
    }

    errno = 0;
    max_files = strtol(value, &end, 10);

    if (errno == ERANGE || end == value || *end != '\0'
            || max_files <= 0 || max_files > GUAC_RDP_DRIVE_MAX_FILES_LIMIT)
        return -1;

    return (int) max_files;

}

void guac_rdp_drive_set_max_files(int max_files) {

    if (!guac_rdp_drive_max_files_supported(max_files))
        return;

    rdp_drive_max_files = max_files;

}

int guac_rdp_drive_get_max_files(void) {

    if (!guac_rdp_drive_max_files_supported(rdp_drive_max_files))
        return GUAC_RDP_DRIVE_MAX_FILES_DEFAULT;

    return rdp_drive_max_files;

}
