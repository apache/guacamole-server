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

#include "guacamole/mem.h"
#include "guacamole/error.h"
#include "guacamole/socket-ssl.h"
#include "guacamole/socket.h"
#include "wait-fd.h"

#include <pthread.h>
#include <stdlib.h>
#ifdef ENABLE_WINSOCK
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

#include <openssl/ssl.h>

/**
 * SSL socket-specific data.
 */
typedef struct guac_socket_ssl_data {

    /**
     * The file descriptor that SSL communication will take place
     * over.
     */
    int fd;

    /**
     * The current SSL context.
     */
    SSL_CTX* context;

    /**
     * The SSL connection, created automatically via
     * guac_socket_open_secure().
     */
    SSL* ssl;

    /**
     * Lock that is acquired when an instruction is being written, and released
     * when the instruction is finished being written.
     */
    pthread_mutex_t socket_lock;

    /**
     * Lock which serializes all use of the SSL connection. OpenSSL does not
     * allow a connection to be read and written at the same time.
     */
    pthread_mutex_t ssl_lock;

} guac_socket_ssl_data;

/**
 * Returns whether data already decrypted by OpenSSL awaits a read from the
 * given SSL socket.
 *
 * @param socket
 *     The guac_socket to test.
 *
 * @return
 *     Non-zero if decrypted data awaits a read, zero otherwise.
 */
static int __guac_socket_ssl_pending(guac_socket* socket) {

    guac_socket_ssl_data* data = (guac_socket_ssl_data*) socket->data;

    pthread_mutex_lock(&(data->ssl_lock));
    int pending = SSL_pending(data->ssl);
    pthread_mutex_unlock(&(data->ssl_lock));

    return pending > 0;

}

static ssize_t __guac_socket_ssl_read_handler(guac_socket* socket,
        void* buf, size_t count) {

    guac_socket_ssl_data* data = (guac_socket_ssl_data*) socket->data;
    int retval;

    /* Wait for input before locking the SSL connection (SSL_read() must not
     * block while we hold ssl_lock or writes will be blocked, as well) */
    if (!__guac_socket_ssl_pending(socket)) {
        char peeked;
        int received;
        GUAC_RETRY_EINTR(received, recv(data->fd, &peeked, 1, MSG_PEEK));
        if (received < 0) {
            guac_error = GUAC_STATUS_SEE_ERRNO;
            guac_error_message = "Error reading data from secure socket";
            return -1;
        }
    }

    /* Read from socket */
    pthread_mutex_lock(&(data->ssl_lock));
    retval = SSL_read(data->ssl, buf, count);
    pthread_mutex_unlock(&(data->ssl_lock));

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

    pthread_mutex_lock(&(data->ssl_lock));
    retval = SSL_write(data->ssl, buf, count);
    pthread_mutex_unlock(&(data->ssl_lock));

    /* Record errors in guac_error */
    if (retval <= 0) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Error writing data to secure socket";
    }

    return retval;

}

static int __guac_socket_ssl_select_handler(guac_socket* socket, int usec_timeout) {

    guac_socket_ssl_data* data = (guac_socket_ssl_data*) socket->data;

    /* Data already decrypted by OpenSSL is not visible to a wait on the
     * descriptor */
    if (__guac_socket_ssl_pending(socket))
        return 1;

    int retval = guac_wait_for_fd(data->fd, usec_timeout);

    /* Properly set guac_error */
    if (retval <  0) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Error while waiting for data on secure socket";
    }

    else if (retval == 0) {
        guac_error = GUAC_STATUS_TIMEOUT;
        guac_error_message = "Timeout while waiting for data on secure socket";
    }

    return retval;

}

static void __guac_socket_ssl_shutdown_handler(guac_socket* socket) {

    guac_socket_ssl_data* data = (guac_socket_ssl_data*) socket->data;

#ifdef ENABLE_WINSOCK
    shutdown(data->fd, SD_BOTH);
#else
    shutdown(data->fd, SHUT_RDWR);
#endif

}

static int __guac_socket_ssl_free_handler(guac_socket* socket) {

    /* Shutdown SSL */
    guac_socket_ssl_data* data = (guac_socket_ssl_data*) socket->data;
    SSL_shutdown(data->ssl);
    SSL_free(data->ssl);

    /* Close file descriptor */
    close(data->fd);

    pthread_mutex_destroy(&(data->socket_lock));
    pthread_mutex_destroy(&(data->ssl_lock));

    guac_mem_free(data);
    return 0;
}

/**
 * Acquires exclusive access to the given socket.
 *
 * @param socket
 *      The guac_socket to which exclusive access is requested.
 */
static void __guac_socket_ssl_lock_handler(guac_socket* socket) {

    guac_socket_ssl_data* data = (guac_socket_ssl_data*) socket->data;

    /* Acquire exclusive access to the socket */
    pthread_mutex_lock(&(data->socket_lock));

}

/**
 * Releases exclusive access to the given socket.
 *
 * @param socket
 *      The guac_socket to which exclusive access is released.
 */
static void __guac_socket_ssl_unlock_handler(guac_socket* socket) {

    guac_socket_ssl_data* data = (guac_socket_ssl_data*) socket->data;

    /* Relinquish exclusive access to the socket */
    pthread_mutex_unlock(&(data->socket_lock));

}

guac_socket* guac_socket_open_secure(SSL_CTX* context, int fd) {

    /* Create new SSL structure */
    SSL* ssl = SSL_new(context);
    if (ssl == NULL)
        return NULL;

    /* Allocate socket and associated data */
    guac_socket* socket = guac_socket_alloc();
    guac_socket_ssl_data* data = guac_mem_alloc(sizeof(guac_socket_ssl_data));

    /* Init SSL */
    data->context = context;
    data->ssl = ssl;
    SSL_set_fd(data->ssl, fd);

    /* Accept SSL connection, handle errors */
    if (SSL_accept(ssl) <= 0) {

        guac_error = GUAC_STATUS_INTERNAL_ERROR;
        guac_error_message = "SSL accept failed";

        guac_mem_free(data);
        guac_socket_free(socket);
        SSL_free(ssl);
        return NULL;
    }

    pthread_mutexattr_t lock_attributes;
    pthread_mutexattr_init(&lock_attributes);
    pthread_mutexattr_setpshared(&lock_attributes, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&(data->socket_lock), &lock_attributes);
    pthread_mutex_init(&(data->ssl_lock), &lock_attributes);

    /* Store file descriptor as socket data */
    data->fd = fd;
    socket->data = data;

    /* Set read/write handlers */
    socket->read_handler     = __guac_socket_ssl_read_handler;
    socket->write_handler    = __guac_socket_ssl_write_handler;
    socket->select_handler   = __guac_socket_ssl_select_handler;
    socket->free_handler     = __guac_socket_ssl_free_handler;
    socket->shutdown_handler = __guac_socket_ssl_shutdown_handler;
    socket->lock_handler     = __guac_socket_ssl_lock_handler;
    socket->unlock_handler   = __guac_socket_ssl_unlock_handler;

    return socket;

}

