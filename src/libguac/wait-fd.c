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

#include "guacamole/error.h"
#include <errno.h>

#ifdef ENABLE_WINSOCK
#    include <winsock2.h>
#else
#    ifdef HAVE_POLL
#        include <poll.h>
#    else
#        include <sys/select.h>
#    endif
#endif

#ifdef HAVE_POLL
int guac_wait_for_fd(int fd, int usec_timeout) {

    /* Initialize with single underlying file descriptor */
    struct pollfd fds[1] = {{
        .fd      = fd,
        .events  = POLLIN,
        .revents = 0
    }};

    int retval;

    /* No timeout if usec_timeout is negative */
    if (usec_timeout < 0) {
        GUAC_RETRY_EINTR(retval, poll(fds, 1, -1));
        return retval;
    }

    /* Handle timeout if specified, rounding up to poll()'s granularity */
    GUAC_RETRY_EINTR(retval, poll(fds, 1, (usec_timeout + 999) / 1000));
    return retval;

}
#else
int guac_wait_for_fd(int fd, int usec_timeout) {

    /* Prevent overflowing fd_set. */
    if (fd >= FD_SETSIZE) {
        errno = EINVAL;
        return -1;
    }

    int retval;
    fd_set fds;

    /* Initialize fd_set with single underlying file descriptor */
    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    /* No timeout if usec_timeout is negative */
    if (usec_timeout < 0) {
        GUAC_RETRY_EINTR(retval, select(fd + 1, &fds, NULL, NULL, NULL));
        return retval;
    }

    /* Handle timeout if specified */
    struct timeval timeout = {
        .tv_sec  = usec_timeout / 1000000,
        .tv_usec = usec_timeout % 1000000
    };

    /* Linux (kernel/glibc verified): select() does not modify
       fd_set on -1, and updates struct timeval to reflect elapsed
       time on EINTR, so neither needs reinitialization on retry. */
    GUAC_RETRY_EINTR(retval, select(fd + 1, &fds, NULL, NULL, &timeout));
    return retval;

}
#endif
