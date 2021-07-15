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
#include "guacamole/socket-ssl.h"
#include "guacamole/socket.h"
#include "wait-fd.h"

#include <stdlib.h>

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

static int __guac_socket_ssl_free_handler(guac_socket* socket) {

    /* Shutdown SSL */
    guac_socket_ssl_data* data = (guac_socket_ssl_data*) socket->data;
    SSL_shutdown(data->ssl);
    SSL_free(data->ssl);

    /* Close file descriptor */
    close(data->fd);

    free(data);
    return 0;
}

guac_socket* guac_socket_open_secure(SSL_CTX* context, int fd) {

    /* Create new SSL structure */
    SSL* ssl = SSL_new(context);
    if (ssl == NULL)
        return NULL;

    /* Allocate socket and associated data */
    guac_socket* socket = guac_socket_alloc();
    guac_socket_ssl_data* data = malloc(sizeof(guac_socket_ssl_data));

    /* Init SSL */
    data->context = context;
    data->ssl = ssl;
    SSL_set_fd(data->ssl, fd);

    /* Accept SSL connection, handle errors */
    if (SSL_accept(ssl) <= 0) {

        guac_error = GUAC_STATUS_INTERNAL_ERROR;
        guac_error_message = "SSL accept failed";

        free(data);
        guac_socket_free(socket);
        SSL_free(ssl);
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


#ifdef OPENSSL_REQUIRES_THREADING_CALLBACKS
/**
 * Array of mutexes, used by OpenSSL.
 */
static pthread_mutex_t* guacd_openssl_locks = NULL;

/**
 * Called by OpenSSL when locking or unlocking the Nth mutex.
 *
 * @param mode
 *     A bitmask denoting the action to be taken on the Nth lock, such as
 *     CRYPTO_LOCK or CRYPTO_UNLOCK.
 *
 * @param n
 *     The index of the lock to lock or unlock.
 *
 * @param file
 *     The filename of the function setting the lock, for debugging purposes.
 *
 * @param line
 *     The line number of the function setting the lock, for debugging
 *     purposes.
 */
static void guacd_openssl_locking_callback(int mode, int n,
        const char* file, int line){

    /* Lock given mutex upon request */
    if (mode & CRYPTO_LOCK)
        pthread_mutex_lock(&(guacd_openssl_locks[n]));

    /* Unlock given mutex upon request */
    else if (mode & CRYPTO_UNLOCK)
        pthread_mutex_unlock(&(guacd_openssl_locks[n]));

}

/**
 * Called by OpenSSL when determining the current thread ID.
 *
 * @return
 *     An ID which uniquely identifies the current thread.
 */
static unsigned long guacd_openssl_id_callback() {
    return (unsigned long) pthread_self();
}

/**
 * Creates the given number of mutexes, such that OpenSSL will have at least
 * this number of mutexes at its disposal.
 *
 * @param count
 *     The number of mutexes (locks) to create.
 */
static void guacd_openssl_init_locks(int count) {

    int i;

    /* Allocate required number of locks */
    guacd_openssl_locks =
        malloc(sizeof(pthread_mutex_t) * count);

    /* Initialize each lock */
    for (i=0; i < count; i++)
        pthread_mutex_init(&(guacd_openssl_locks[i]), NULL);

}

/**
 * Frees the given number of mutexes.
 *
 * @param count
 *     The number of mutexes (locks) to free.
 */
static void guacd_openssl_free_locks(int count) {

    int i;

    /* SSL lock array was not initialized */
    if (guacd_openssl_locks == NULL)
        return;

    /* Free all locks */
    for (i=0; i < count; i++)
        pthread_mutex_destroy(&(guacd_openssl_locks[i]));

    /* Free lock array */
    free(guacd_openssl_locks);

}


/**
 * Sets up the the necessary callbacks for thread safety in OpenSSL.
 */
void guacd_openssl_init_thread_safety() {
    guacd_openssl_init_locks(CRYPTO_num_locks());
    CRYPTO_set_id_callback(guacd_openssl_id_callback);
    CRYPTO_set_locking_callback(guacd_openssl_locking_callback);
}

/**
 * Frees the locks associated with OpenSSL thread safety.
 */
void guacd_openssl_free_thread_safety() {
    guacd_openssl_free_locks(CRYPTO_num_locks());
}

#endif

