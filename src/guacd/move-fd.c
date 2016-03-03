/*
 * Copyright (C) 2014 Glyptodon LLC
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
#include "move-fd.h"

/* Required for CMSG_* macros on BSD */
#define __BSD_VISIBLE 1

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

int guacd_send_fd(int sock, int fd) {

    struct msghdr message = {0};
    char message_data[] = {'G'};

    /* Assign data buffer */
    struct iovec io_vector[1];
    io_vector[0].iov_base = message_data;
    io_vector[0].iov_len  = sizeof(message_data);
    message.msg_iov    = io_vector;
    message.msg_iovlen = 1;

    /* Assign ancillary data buffer */
    char buffer[CMSG_SPACE(sizeof(fd))];
    message.msg_control = buffer;
    message.msg_controllen = sizeof(buffer);

    /* Set fields of control message header */
    struct cmsghdr* control = CMSG_FIRSTHDR(&message);
    control->cmsg_level = SOL_SOCKET;
    control->cmsg_type  = SCM_RIGHTS;
    control->cmsg_len   = CMSG_LEN(sizeof(fd));

    /* Add file descriptor to message data */
    memcpy(CMSG_DATA(control), &fd, sizeof(fd));

    /* Send file descriptor */
    return (sendmsg(sock, &message, 0) == sizeof(message_data));

}

int guacd_recv_fd(int sock) {

    int fd;

    struct msghdr message = {0};
    char message_data[1];

    /* Assign data buffer */
    struct iovec io_vector[1];
    io_vector[0].iov_base = message_data;
    io_vector[0].iov_len  = sizeof(message_data);
    message.msg_iov    = io_vector;
    message.msg_iovlen = 1;


    /* Assign ancillary data buffer */
    char buffer[CMSG_SPACE(sizeof(fd))];
    message.msg_control = buffer;
    message.msg_controllen = sizeof(buffer);

    /* Receive file descriptor */
    if (recvmsg(sock, &message, 0) == sizeof(message_data)) {

        /* Validate payload */
        if (message_data[0] != 'G') {
            errno = EPROTO;
            return -1;
        }

        /* Iterate control headers, looking for the sent file descriptor */
        struct cmsghdr* control;
        for (control = CMSG_FIRSTHDR(&message); control != NULL; control = CMSG_NXTHDR(&message, control)) {

            /* Pull file descriptor from data */
            if (control->cmsg_level == SOL_SOCKET && control->cmsg_type == SCM_RIGHTS) {
                memcpy(&fd, CMSG_DATA(control), sizeof(fd));
                return fd;
            }

        }

    } /* end if recvmsg() success */

    /* Failed to receive file descriptor */
    return -1;

}

