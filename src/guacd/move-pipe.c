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
#include "log.h"
#include "move-pipe.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

#include <guacamole/client-types.h>

/* Windows headers */
#include <errhandlingapi.h>
#include <fcntl.h>
#include <handleapi.h>
#include <io.h>
#include <windows.h>

int guacd_send_pipe(int sock, char* pipe_name) {

    /* Assign data buffer */
    struct iovec io_vector[1];
    io_vector[0].iov_base = pipe_name;
    io_vector[0].iov_len  = GUAC_PIPE_NAME_LENGTH;

    struct msghdr message = {0};
    message.msg_iov    = io_vector;
    message.msg_iovlen = 1;

    /* Send pipe name */
    return (sendmsg(sock, &message, 0) == GUAC_PIPE_NAME_LENGTH);

}

HANDLE guacd_recv_pipe(int sock) {

    /* Assign data buffer */
    char pipe_name[GUAC_PIPE_NAME_LENGTH];
    struct iovec io_vector[1];
    io_vector[0].iov_base = pipe_name;
    io_vector[0].iov_len  = GUAC_PIPE_NAME_LENGTH;

    struct msghdr message = {0};
    message.msg_iov    = io_vector;
    message.msg_iovlen = 1;
    
    /* Receive file descriptor */
    if (recvmsg(sock, &message, 0) == GUAC_PIPE_NAME_LENGTH) {

        /* 
         * Make sure the value is always null-terminated, even if an invalid
         * name was sent.
         */
        pipe_name[GUAC_PIPE_NAME_LENGTH - 1] = '\0';

        return CreateFile(

            pipe_name,

            /* Desired access level */
            GENERIC_READ | GENERIC_WRITE,

            /* Sharing level - do not allow any other usage of this pipe */
            0,

            /* Default security mode - do not allow child processes to inherir */
            NULL,

            /* Open the existing pipe; don't try to create a new one */
            OPEN_EXISTING,

            /* Open in "overlapped" (async) mode */
            FILE_FLAG_NO_BUFFERING | FILE_FLAG_OVERLAPPED,

            /* Ignored for existing pipes */
            NULL
        );

    } /* end if recvmsg() success */

    /* Failed to get the pipe */
    return NULL;

}

