/*
 * Copyright (C) 2013 Glyptodon LLC
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

#include "config.h"

#include "suite.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <CUnit/Basic.h>
#include <guacamole/error.h>
#include <guacamole/instruction.h>
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
        guac_instruction* instruction;

        close(wfd);

        /* Open guac socket */
        socket = guac_socket_open(rfd);
        CU_ASSERT_PTR_NOT_NULL_FATAL(socket);

        /* Read instruction */
        instruction = guac_instruction_read(socket, 1000000);
        CU_ASSERT_PTR_NOT_NULL_FATAL(instruction);
        
        /* Validate contents */
        CU_ASSERT_STRING_EQUAL(instruction->opcode, "test");
        CU_ASSERT_EQUAL_FATAL(instruction->argc, 3);
        CU_ASSERT_STRING_EQUAL(instruction->argv[0], "a" UTF8_4 "b");
        CU_ASSERT_STRING_EQUAL(instruction->argv[1], "12345");
        CU_ASSERT_STRING_EQUAL(instruction->argv[2], "a" UTF8_8 "c");
        
        /* Read another instruction */
        guac_instruction_free(instruction);
        instruction = guac_instruction_read(socket, 1000000);

        /* Validate contents */
        CU_ASSERT_STRING_EQUAL(instruction->opcode, "test2");
        CU_ASSERT_EQUAL_FATAL(instruction->argc, 2);
        CU_ASSERT_STRING_EQUAL(instruction->argv[0], "hellohello");
        CU_ASSERT_STRING_EQUAL(instruction->argv[1], "worldworldworld");

        guac_instruction_free(instruction);
        guac_socket_free(socket);

    }
 
}

