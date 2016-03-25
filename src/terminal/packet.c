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

#include "common.h"
#include "packet.h"

#include <string.h>

int guac_terminal_packet_write(int fd, const void* data, int length) {

    guac_terminal_packet out;

    /* Do not attempt to write packets beyond maximum size */
    if (length > GUAC_TERMINAL_PACKET_SIZE)
        return -1;

    /* Calculate final packet length */
    int packet_length = sizeof(int) + length;

    /* Copy data into packet */
    out.length = length;
    memcpy(out.data, data, length);

    /* Write packet */
    return guac_terminal_write_all(fd, (const char*) &out, packet_length);

}

int guac_terminal_packet_read(int fd, void* data, int length) {

    int bytes;

    /* Read buffers MUST be at least GUAC_TERMINAL_PACKET_SIZE */
    if (length < GUAC_TERMINAL_PACKET_SIZE)
        return -1;

    /* Read length */
    if (guac_terminal_fill_buffer(fd, (char*) &bytes, sizeof(int)) < 0)
        return -1;

    /* Read body */
    if (guac_terminal_fill_buffer(fd, (char*) data, bytes) < 0)
        return -1;

    return bytes;

}

