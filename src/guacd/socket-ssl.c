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

#include "socket-ssl.h"

#include <stdlib.h>
#include <sys/select.h>

#include <guacamole/error.h>
#include <guacamole/socket.h>
#include <openssl/ssl.h>

static ssize_t __guac_socket_ssl_read_handler(guac_socket* socket,
        void* buf, size_t count) {

    /* Read from socket */
    guac_socket_ssl_data* data = (guac_socket_ssl_data*) socket->data;
    int retval;

    retval = SSL_read(data->ssl, buf, count);

    /* Record errors in guac_error */
    if (retval <= 0) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Error reading data from secure socket";
    }

    return retval;

}

static ssize_t __guac_socket_ssl_write_handler(guac_socket* socket,
        const void* buf, size_t count) {

    /* Write data to socket */
    guac_socket_ssl_data* data = (guac_socket_ssl_data*) socket->data;
    int retval;

    retval = SSL_write(data->ssl, buf, count);

    /* Record errors in guac_error */
    if (retval <= 0) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Error writing data to secure socket";
    }

    return retval;

}

static int __guac_socket_ssl_select_handler(guac_socket* socket, int usec_timeout) {

    guac_socket_ssl_data* data = (guac_socket_ssl_data*) socket->data;

    fd_set fds;
    struct timeval timeout;
    int retval;

    /* Initialize fd_set with single underlying file descriptor */
    FD_ZERO(&fds);
    FD_SET(data->fd, &fds);

    /* No timeout if usec_timeout is negative */
    if (usec_timeout < 0)
        retval = select(data->fd + 1, &fds, NULL, NULL, NULL); 

    /* Handle timeout if specified */
    else {
        timeout.tv_sec = usec_timeout/1000000;
        timeout.tv_usec = usec_timeout%1000000;
        retval = select(data->fd + 1, &fds, NULL, NULL, &timeout);
    }

    /* Properly set guac_error */
    if (retval <  0) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Error while waiting for data on secure socket";
    }

    if (retval == 0) {
        guac_error = GUAC_STATUS_TIMEOUT;
        guac_error_message = "Timeout while waiting for data on secure socket";
    }

    return retval;

}

static int __guac_socket_ssl_free_handler(guac_socket* socket) {

    /* Shutdown SSL */
    guac_socket_ssl_data* data = (guac_socket_ssl_data*) socket->data;
    SSL_shutdown(data->ssl);

    /* Close file descriptor */
    close(data->fd);

    free(data);
    return 0;
}

guac_socket* guac_socket_open_secure(SSL_CTX* context, int fd) {

    /* Allocate socket and associated data */
    guac_socket* socket = guac_socket_alloc();
    guac_socket_ssl_data* data = malloc(sizeof(guac_socket_ssl_data));

    /* Init SSL */
    data->context = context;
    data->ssl = SSL_new(context);
    SSL_set_fd(data->ssl, fd);

    /* Accept SSL connection, handle errors */
    if (SSL_accept(data->ssl) <= 0) {

        guac_error = GUAC_STATUS_INTERNAL_ERROR;
        guac_error_message = "SSL accept failed";

        free(data);
        guac_socket_free(socket);
        return NULL;
    }

    /* Store file descriptor as socket data */
    data->fd = fd;
    socket->data = data;

    /* Set read/write handlers */
    socket->read_handler   = __guac_socket_ssl_read_handler;
    socket->write_handler  = __guac_socket_ssl_write_handler;
    socket->select_handler = __guac_socket_ssl_select_handler;
    socket->free_handler   = __guac_socket_ssl_free_handler;

    return socket;

}

