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
#include <guacamole/object.h>
#include <libssh2.h>

#ifdef LIBSSH2_USES_GCRYPT
#include <gcrypt.h>
#endif

#include <openssl/err.h>
#include <openssl/ssl.h>

#include <pthread.h>

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

LIBSSH2_SESSION* guac_common_ssh_connect_password(const char* hostname,
        int port, const char* username, const char* password) {

    /* STUB */
    return NULL;

}

LIBSSH2_SESSION* guac_common_ssh_connect_private_key(const char* hostname,
        int port, const char* username, const char* private_key,
        const char* passphrase) {

    /* STUB */
    return NULL;

}

guac_object* guac_common_ssh_create_sftp_filesystem(guac_client* client,
         const char* name, LIBSSH2_SESSION* session) {

    /* STUB */
    return NULL;

}

void guac_common_ssh_destroy_sftp_filesystem(guac_client* client,
        guac_object* filesystem) {
    /* STUB */
}

