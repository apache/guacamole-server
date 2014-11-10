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

#include "error.h"
#include "socket.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef __MINGW32__
#include <winsock2.h>
#else
#include <sys/select.h>
#endif

typedef struct __guac_socket_fd_data {

    int fd;

} __guac_socket_fd_data;

ssize_t __guac_socket_fd_read_handler(guac_socket* socket,
        void* buf, size_t count) {

    __guac_socket_fd_data* data = (__guac_socket_fd_data*) socket->data;

    /* Read from socket */
    int retval = read(data->fd, buf, count);

    /* Record errors in guac_error */
    if (retval < 0) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Error reading data from socket";
    }

    return retval;

}

ssize_t __guac_socket_fd_write_handler(guac_socket* socket,
        const void* buf, size_t count) {

    __guac_socket_fd_data* data = (__guac_socket_fd_data*) socket->data;
    int retval;

#ifdef __MINGW32__
    /* MINGW32 WINSOCK only works with send() */
    retval = send(data->fd, buf, count, 0);
#else
    /* Use write() for all other platforms */
    retval = write(data->fd, buf, count);
#endif

    /* Record errors in guac_error */
    if (retval < 0) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Error writing data to socket";
    }

    return retval;
}

int __guac_socket_fd_select_handler(guac_socket* socket, int usec_timeout) {

    __guac_socket_fd_data* data = (__guac_socket_fd_data*) socket->data;

    fd_set fds;
    struct timeval timeout;
    int retval;

    /* No timeout if usec_timeout is negative */
    if (usec_timeout < 0)
        retval = select(data->fd + 1, &fds, NULL, NULL, NULL); 

    /* Handle timeout if specified */
    else {
        timeout.tv_sec = usec_timeout/1000000;
        timeout.tv_usec = usec_timeout%1000000;

        FD_ZERO(&fds);
        FD_SET(data->fd, &fds);

        retval = select(data->fd + 1, &fds, NULL, NULL, &timeout);
    }

    /* Properly set guac_error */
    if (retval <  0) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Error while waiting for data on socket";
    }

    if (retval == 0) {
        guac_error = GUAC_STATUS_TIMEOUT;
        guac_error_message = "Timeout while waiting for data on socket";
    }

    return retval;

}

guac_socket* guac_socket_open(int fd) {

    /* Allocate socket and associated data */
    guac_socket* socket = guac_socket_alloc();
    __guac_socket_fd_data* data = malloc(sizeof(__guac_socket_fd_data));

    /* Store file descriptor as socket data */
    data->fd = fd;
    socket->data = data;

    /* Set read/write handlers */
    socket->read_handler   = __guac_socket_fd_read_handler;
    socket->write_handler  = __guac_socket_fd_write_handler;
    socket->select_handler = __guac_socket_fd_select_handler;

    return socket;

}

