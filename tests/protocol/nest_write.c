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

#include "config.h"

#include "suite.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <CUnit/Basic.h>
#include <guacamole/error.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>

void test_nest_write() {

    int rfd, wfd;
    int fd[2], childpid;

    /* Create pipe */
    CU_ASSERT_EQUAL_FATAL(pipe(fd), 0);

    /* File descriptors */
    rfd = fd[0];
    wfd = fd[1];

    /* Fork */
    if ((childpid = fork()) == -1) {
        /* ERROR */
        perror("fork");
        return;
    }

    /* Child (pipe writer) */
    if (childpid != 0) {

        guac_socket* nested_socket;
        guac_socket* socket;

        close(rfd);

        /* Open guac socket */
        socket = guac_socket_open(wfd);

        /* Nest socket */
        nested_socket = guac_socket_nest(socket, 0);

        /* Write instruction */
        guac_protocol_send_name(nested_socket, "a" UTF8_4 "b" UTF8_4 "c");
        guac_protocol_send_sync(nested_socket, 12345);
        guac_socket_flush(nested_socket);
        guac_socket_flush(socket);

        guac_socket_free(nested_socket);
        guac_socket_free(socket);
        exit(0);
    }

    /* Parent (unit test) */
    else {

        char expected[] =
            "4.nest,1.0,37."
                "4.name,11.a" UTF8_4 "b" UTF8_4 "c;"
                "4.sync,5.12345;"
            ";";

        int numread;
        char buffer[1024];
        int offset = 0;

        close(wfd);

        /* Read everything available into buffer */
        while ((numread =
                    read(rfd,
                        &(buffer[offset]),
                        sizeof(buffer)-offset)) != 0) {
            offset += numread;
        }

        /* Add NULL terminator */
        buffer[offset] = '\0';

        /* Read value should be equal to expected value */
        CU_ASSERT_STRING_EQUAL(buffer, expected);

    }
 
}

