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
#include <guacamole/parser.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>

void test_instruction_read() {

    int rfd, wfd;
    int fd[2], childpid;

    char test_string[] = "4.test,6.a" UTF8_4 "b,"
                         "5.12345,10.a" UTF8_8 "c;"
                         "5.test2,10.hellohello,15.worldworldworld;";

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
        close(rfd);
        CU_ASSERT_EQUAL(
            write(wfd, test_string, sizeof(test_string)),
            sizeof(test_string)
        );
        exit(0);
    }

    /* Parent (unit test) */
    else {

        guac_socket* socket;
        guac_parser* parser;

        close(wfd);

        /* Open guac socket */
        socket = guac_socket_open(rfd);
        CU_ASSERT_PTR_NOT_NULL_FATAL(socket);

        /* Allocate parser */
        parser = guac_parser_alloc();
        CU_ASSERT_PTR_NOT_NULL_FATAL(parser);

        /* Read instruction */
        CU_ASSERT_EQUAL_FATAL(guac_parser_read(parser, socket, 1000000), 0);
        
        /* Validate contents */
        CU_ASSERT_STRING_EQUAL(parser->opcode, "test");
        CU_ASSERT_EQUAL_FATAL(parser->argc, 3);
        CU_ASSERT_STRING_EQUAL(parser->argv[0], "a" UTF8_4 "b");
        CU_ASSERT_STRING_EQUAL(parser->argv[1], "12345");
        CU_ASSERT_STRING_EQUAL(parser->argv[2], "a" UTF8_8 "c");
        
        /* Read another instruction */
        CU_ASSERT_EQUAL_FATAL(guac_parser_read(parser, socket, 1000000), 0);

        /* Validate contents */
        CU_ASSERT_STRING_EQUAL(parser->opcode, "test2");
        CU_ASSERT_EQUAL_FATAL(parser->argc, 2);
        CU_ASSERT_STRING_EQUAL(parser->argv[0], "hellohello");
        CU_ASSERT_STRING_EQUAL(parser->argv[1], "worldworldworld");

        guac_parser_free(parser);
        guac_socket_free(socket);

    }
 
}

