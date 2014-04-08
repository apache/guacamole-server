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

