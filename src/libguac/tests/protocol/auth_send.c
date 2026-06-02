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
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>

#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/**
 * Reads everything available from the given file descriptor into a
 * heap-allocated buffer. The buffer grows geometrically as needed.
 * Closes the file descriptor before returning.
 *
 * @param fd
 *     The file descriptor to read from.
 *
 * @param out_len
 *     Out parameter that receives the number of bytes read.
 *
 * @return
 *     A heap-allocated buffer containing the bytes read. The caller
 *     takes ownership and must free the buffer.
 */
static char* read_all(int fd, int* out_len) {

    int capacity = 4096;
    char* buffer = malloc(capacity);
    int offset = 0;
    int numread;

    while ((numread = read(fd, buffer + offset, capacity - offset)) > 0) {
        offset += numread;
        if (offset == capacity) {
            capacity *= 2;
            buffer = realloc(buffer, capacity);
        }
    }

    close(fd);
    *out_len = offset;
    return buffer;

}

/**
 * Runs the given sender inside a forked child process, writing to
 * write_fd, then reads everything from read_fd in the parent and asserts
 * equality against the expected bytes.
 *
 * @param sender
 *     The function the child process invokes to write its bytes to
 *     write_fd.
 *
 * @param write_fd
 *     The file descriptor the sender should write to. Closed in the
 *     parent before reading.
 *
 * @param read_fd
 *     The file descriptor the parent reads from. Closed in the child
 *     before sending.
 *
 * @param expected
 *     The byte sequence expected to be received on read_fd.
 *
 * @param expected_len
 *     The length of the expected byte sequence.
 */
static void run_sender_check(void (*sender)(int), int write_fd, int read_fd,
        const char* expected, int expected_len) {

    int childpid;
    CU_ASSERT_NOT_EQUAL_FATAL((childpid = fork()), -1);

    if (childpid == 0) {
        close(read_fd);
        sender(write_fd);
        _exit(0);
    }

    close(write_fd);

    int len;
    char* buffer = read_all(read_fd, &len);

    int status;
    waitpid(childpid, &status, 0);

    CU_ASSERT_EQUAL(len, expected_len);
    if (len == expected_len)
        CU_ASSERT_EQUAL(memcmp(buffer, expected, expected_len), 0);

    free(buffer);

}

/**
 * Sender for the auth-challenge test. Emits a single
 * guac_protocol_send_auth_challenge with a synthetic UUID-shaped
 * challenge id and a WebAuthn create mimetype, then flushes the socket
 * so the parent process can read the bytes.
 *
 * @param fd
 *     The file descriptor to write the wire bytes to.
 */
static void write_auth_challenge(int fd) {

    guac_socket* socket = guac_socket_open(fd);
    if (socket == NULL)
        return;

    guac_stream stream = { .index = 7 };
    guac_protocol_send_auth_challenge(socket, &stream,
            "application/x-webauthn-create+json",
            "aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa");
    guac_socket_flush(socket);

    guac_socket_free(socket);

}

/**
 * Verifies that guac_protocol_send_auth_challenge emits the expected wire
 * form announcing a stream carrying the challenge body.
 */
void test_protocol__send_auth_challenge(void) {

    int fd[2];
    CU_ASSERT_EQUAL_FATAL(pipe(fd), 0);

    const char expected[] =
        "14.auth-challenge,1.7,34.application/x-webauthn-create+json,"
        "36.aaaaaaaa-aaaa-4aaa-8aaa-aaaaaaaaaaaa;";

    run_sender_check(write_auth_challenge, fd[1], fd[0],
            expected, sizeof(expected) - 1);

}

