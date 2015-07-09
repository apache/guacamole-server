/*
 * Copyright (C) 2015 Glyptodon LLC
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

#include "guac_ssh.h"

#include <guacamole/client.h>
#include <libssh2.h>

#ifdef LIBSSH2_USES_GCRYPT
#include <gcrypt.h>
#endif

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#ifdef LIBSSH2_USES_GCRYPT
GCRY_THREAD_OPTION_PTHREAD_IMPL;
#endif

/**
 * Array of mutexes, used by OpenSSL.
 */
static pthread_mutex_t* guac_common_ssh_openssl_locks;

/**
 * Called by OpenSSL when locking or unlocking the Nth mutex.
 */
static void guac_common_ssh_openssl_locking_callback(int mode, int n,
        const char* file, int line){

    /* Lock given mutex upon request */
    if (mode & CRYPTO_LOCK)
        pthread_mutex_lock(&(guac_common_ssh_openssl_locks[n]));

    /* Unlock given mutex upon request */
    else if (mode & CRYPTO_UNLOCK)
        pthread_mutex_unlock(&(guac_common_ssh_openssl_locks[n]));

}

/**
 * Called by OpenSSL when determining the current thread ID.
 */
static unsigned long guac_common_ssh_openssl_id_callback() {
    return (unsigned long) pthread_self();
}

/**
 * Creates the given number of mutexes, such that OpenSSL will have at least
 * this number of mutexes at its disposal.
 */
static void guac_common_ssh_openssl_init_locks(int count) {

    int i;

    /* Allocate required number of locks */
    guac_common_ssh_openssl_locks =
        malloc(sizeof(pthread_mutex_t) * CRYPTO_num_locks());

    /* Initialize each lock */
    for (i=0; i < count; i++)
        pthread_mutex_init(&(guac_common_ssh_openssl_locks[i]), NULL);

}

/**
 * Frees the given number of mutexes.
 */
static void guac_common_ssh_openssl_free_locks(int count) {

    int i;

    /* Free all locks */
    for (i=0; i < count; i++)
        pthread_mutex_destroy(&(guac_common_ssh_openssl_locks[i]));

}

int guac_common_ssh_init(guac_client* client) {

#ifdef LIBSSH2_USES_GCRYPT
    /* Init threadsafety in libgcrypt */
    gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
    if (!gcry_check_version(GCRYPT_VERSION)) {
        guac_client_log(client, GUAC_LOG_ERROR, "libgcrypt version mismatch.");
        return 1;
    }
#endif

    /* Init threadsafety in OpenSSL */
    guac_common_ssh_openssl_init_locks(CRYPTO_num_locks());
    CRYPTO_set_id_callback(guac_common_ssh_openssl_id_callback);
    CRYPTO_set_locking_callback(guac_common_ssh_openssl_locking_callback);

    /* Init OpenSSL */
    SSL_library_init();
    ERR_load_crypto_strings();

    /* Init libssh2 */
    libssh2_init(0);

    /* Success */
    return 0;

}

void guac_common_ssh_uninit() {
    guac_common_ssh_openssl_free_locks(CRYPTO_num_locks());
}

/**
 * Connects to the SSH server running at the given hostname and port, but does
 * not perform any authentication. Authentication must be immediately performed
 * after creation of the session for the session to become usable. If an error
 * occurs while connecting, the Guacamole client will automatically and fatally
 * abort.
 *
 * @param client
 *     The Guacamole client that will be using SSH.
 *
 * @param hostname
 *     The hostname of the SSH server to connect to.
 *
 * @param port
 *     The port to connect to on the given hostname.
 *
 * @param socket_fd
 *     A pointer to an integer in which the newly-allocated file descriptor for
 *     the SSH socket should be stored.
 *
 * @return
 *     A new SSH session if the connection succeeds, or NULL if the connection
 *     was not successful.
 */
static LIBSSH2_SESSION* guac_common_ssh_connect(guac_client* client,
        const char* hostname, const char* port, int* socket_fd) {

    int retval;

    int fd;
    struct addrinfo* addresses;
    struct addrinfo* current_address;

    char connected_address[1024];
    char connected_port[64];

    struct addrinfo hints = {
        .ai_family   = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP
    };

    /* Get socket */
    fd = socket(AF_INET, SOCK_STREAM, 0);

    /* Get addresses connection */
    if ((retval = getaddrinfo(hostname, port, &hints, &addresses))) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "Error parsing given address or port: %s",
                gai_strerror(retval));
        return NULL;
    }

    /* Attempt connection to each address until success */
    current_address = addresses;
    while (current_address != NULL) {

        /* Resolve hostname */
        if ((retval = getnameinfo(current_address->ai_addr,
                current_address->ai_addrlen,
                connected_address, sizeof(connected_address),
                connected_port, sizeof(connected_port),
                NI_NUMERICHOST | NI_NUMERICSERV)))
            guac_client_log(client, GUAC_LOG_DEBUG,
                    "Unable to resolve host: %s", gai_strerror(retval));

        /* Connect */
        if (connect(fd, current_address->ai_addr,
                        current_address->ai_addrlen) == 0) {

            guac_client_log(client, GUAC_LOG_DEBUG,
                    "Successfully connected to host %s, port %s",
                    connected_address, connected_port);

            /* Done if successful connect */
            break;

        }

        /* Otherwise log information regarding bind failure */
        else
            guac_client_log(client, GUAC_LOG_DEBUG, "Unable to connect to "
                    "host %s, port %s: %s",
                    connected_address, connected_port, strerror(errno));

        current_address = current_address->ai_next;

    }

    /* If unable to connect to anything, fail */
    if (current_address == NULL) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR,
                "Unable to connect to any addresses.");
        return NULL;
    }

    /* Free addrinfo */
    freeaddrinfo(addresses);

    /* Open SSH session */
    LIBSSH2_SESSION* session = libssh2_session_init_ex(NULL, NULL,
            NULL, client);
    if (session == NULL) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "Session allocation failed.");
        return NULL;
    }

    /* Perform handshake */
    if (libssh2_session_handshake(session, fd)) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR,
                "SSH handshake failed.");
        return NULL;
    }

    /* Save file descriptor */
    if (socket_fd != NULL)
        *socket_fd = fd;

    /* Return created session */
    return session;

}

LIBSSH2_SESSION* guac_common_ssh_connect_password(guac_client* client,
        const char* hostname, const char* port,
        const char* username, const char* password,
        int* socket_fd) {

    LIBSSH2_SESSION* session = guac_common_ssh_connect(client,
            hostname, port, socket_fd);

    /* STUB */

    return session;

}

LIBSSH2_SESSION* guac_common_ssh_connect_private_key(guac_client* client,
        const char* hostname, const char* port,
        const char* username, const char* private_key,
        const char* passphrase,
        int* socket_fd) {

    LIBSSH2_SESSION* session = guac_common_ssh_connect(client,
            hostname, port, socket_fd);

    /* STUB */

    return session;

}

