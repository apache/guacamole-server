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

#ifndef GUAC_RDP_DRIVE_H
#define GUAC_RDP_DRIVE_H

/**
 * Process-wide limit on how many files an RDP redirected drive may keep
 * open. guacd sets the limit from guacd.conf and its command line before
 * it accepts connections. Forked connection processes inherit the value,
 * and the RDP plugin reads it when it allocates a filesystem.
 *
 * @file rdp-drive.h
 */

/**
 * The open-file limit used when guacd has not been given another value.
 */
#define GUAC_RDP_DRIVE_MAX_FILES_DEFAULT 128

/**
 * The largest open-file table that may be configured. Each slot embeds a
 * path buffer, so the table is allocated per session.
 */
#define GUAC_RDP_DRIVE_MAX_FILES_LIMIT 4096

/**
 * Parses a configured RDP redirected-drive open-file limit. The value must
 * be a positive decimal integer no greater than
 * GUAC_RDP_DRIVE_MAX_FILES_LIMIT. Leading zeros are accepted. A sign,
 * whitespace, or any other trailing character is rejected.
 *
 * @param value
 *     The text to parse. NULL and an empty string are rejected.
 *
 * @return
 *     The parsed limit, or -1 if value is NULL, empty, not a positive
 *     decimal integer, or greater than GUAC_RDP_DRIVE_MAX_FILES_LIMIT.
 */
int guac_rdp_drive_parse_max_files(const char* value);

/**
 * Sets the process-wide RDP redirected-drive open-file limit. guacd calls
 * this after configuration has been loaded and before it forks connection
 * processes. A value outside the range accepted by
 * guac_rdp_drive_parse_max_files() is ignored and the previous limit is
 * kept.
 *
 * @param max_files
 *     The open-file limit. Must be a positive integer no greater than
 *     GUAC_RDP_DRIVE_MAX_FILES_LIMIT.
 */
void guac_rdp_drive_set_max_files(int max_files);

/**
 * Returns the process-wide RDP redirected-drive open-file limit.
 *
 * @return
 *     The configured limit. The initial value is
 *     GUAC_RDP_DRIVE_MAX_FILES_DEFAULT. A stored value outside the
 *     supported range is reported as that default.
 */
int guac_rdp_drive_get_max_files(void);

#endif
