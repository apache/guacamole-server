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

#include <CUnit/CUnit.h>
#include <guacamole/layer.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * Writes a series of Guacamole instructions covering boundary values for
 * the length-prefixed int/double/string element encoding used by every
 * guac_protocol_send_*() function (__guac_socket_write_length_int/double/
 * string in protocol.c): the extremes of a 64-bit integer, negative
 * numbers, an empty string, a NULL string, and very large/small/negative
 * doubles.
 *
 * @param fd
 *     The file descriptor to write instructions to.
 */
static void write_instructions(int fd) {

    guac_socket* socket = guac_socket_open(fd);
    if (socket == NULL) {
        close(fd);
        return;
    }

    guac_layer layer;
    layer.index = 0;

    /* Integer extremes and negative values */
    guac_protocol_send_sync(socket, INT64_MIN, -1);
    guac_protocol_send_sync(socket, INT64_MAX, 0);

    /* Empty string, and NULL (must be treated the same as "") */
    guac_protocol_send_name(socket, "");
    guac_protocol_send_name(socket, NULL);

    /* Double extremes: negative, very large, very small */
    guac_protocol_send_transform(socket, &layer,
            -1.5, 0.0, 1e300, -1e-300, 1e14, -0.0);

    guac_socket_flush(socket);
    guac_socket_free(socket);

}

/**
 * Reads raw bytes from the given file descriptor until no further bytes
 * remain, verifying that those bytes represent the series of Guacamole
 * instructions expected to be written by write_instructions().
 *
 * @param fd
 *     The file descriptor to read data from.
 */
static void read_expected_instructions(int fd) {

    char expected[512];
    snprintf(expected, sizeof(expected),
        "4.sync,20.%lld,2.-1;"
        "4.sync,19.%lld,1.0;"
        "4.name,0.;"
        "4.name,0.;"
        "9.transform,1.0,4.-1.5,1.0,6.1e+300,7.-1e-300,15.100000000000000,2.-0;",
        (long long) INT64_MIN, (long long) INT64_MAX);

    int numread;
    char buffer[1024];
    int offset = 0;

    while ((numread = read(fd, &(buffer[offset]),
                    sizeof(buffer) - offset)) > 0) {
        offset += numread;
    }

    CU_ASSERT_EQUAL(offset, strlen(expected));

    buffer[offset] = '\0';
    CU_ASSERT_STRING_EQUAL(buffer, expected);

    close(fd);

}

/**
 * Verifies that guac_protocol_send_*() correctly encodes boundary values
 * (INT64_MIN/MAX, negative numbers, an empty string, and extreme doubles)
 * with the proper length-prefixed element format.
 */
void test_socket__edge_case_values() {

    int fd[2];

    CU_ASSERT_EQUAL_FATAL(pipe(fd), 0);

    int read_fd = fd[0];
    int write_fd = fd[1];

    int childpid;
    CU_ASSERT_NOT_EQUAL_FATAL((childpid = fork()), -1);

    if (childpid == 0) {
        close(read_fd);
        write_instructions(write_fd);
        exit(0);
    }

    close(write_fd);
    read_expected_instructions(read_fd);

}
