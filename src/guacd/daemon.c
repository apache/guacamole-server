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

#include "connection.h"
#include "conf-args.h"
#include "conf-file.h"
#include "libguacd/user.h"
#include "log.h"
#include "proc-map.h"

#ifdef ENABLE_SSL
#include <openssl/ssl.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define GUACD_DEV_NULL "/dev/null"
#define GUACD_ROOT     "/"

/**
 * Redirects the given file descriptor to /dev/null. The given flags must match
 * the read/write flags of the file descriptor given (if the given file
 * descriptor was opened write-only, flags here must be O_WRONLY, etc.).
 *
 * @param fd
 *     The file descriptor to redirect to /dev/null.
 *
 * @param flags
 *     The flags to use when opening /dev/null as the target for redirection.
 *     These flags must match the flags of the file descriptor given.
 *
 * @return
 *     Zero on success, non-zero if redirecting the file descriptor fails.
 */
static int redirect_fd(int fd, int flags) {

    /* Attempt to open bit bucket */
    int new_fd = open(GUACD_DEV_NULL, flags);
    if (new_fd < 0)
        return 1;

    /* If descriptor is different, redirect old to new and close new */
    if (new_fd != fd) {
        dup2(new_fd, fd);
        close(new_fd);
    }

    return 0;

}

/**
 * Turns the current process into a daemon through a series of fork() calls.
 * The standard I/O file desriptors for STDIN, STDOUT, and STDERR will be
 * redirected to /dev/null, and the working directory is changed to root.
 * Execution within the caller of this function will terminate before this
 * function returns, while execution within the daemonized child process will
 * continue.
 *
 * @return
 *    Zero if the daemonization process succeeded and we are now in the
 *    daemonized child process, or non-zero if daemonization failed and we are
 *    still the original caller. This function does not return for the original
 *    caller if daemonization succeeds.
 */
static int daemonize() {

    pid_t pid;

    /* Fork once to ensure we aren't the process group leader */
    pid = fork();
    if (pid < 0) {
        guacd_log(GUAC_LOG_ERROR, "Could not fork() parent: %s", strerror(errno));
        return 1;
    }

    /* Exit if we are the parent */
    if (pid > 0) {
        guacd_log(GUAC_LOG_DEBUG, "Exiting and passing control to PID %i", pid);
        _exit(0);
    }

    /* Start a new session (if not already group leader) */
    setsid();

    /* Fork again so the session group leader exits */
    pid = fork();
    if (pid < 0) {
        guacd_log(GUAC_LOG_ERROR, "Could not fork() group leader: %s", strerror(errno));
        return 1;
    }

    /* Exit if we are the parent */
    if (pid > 0) {
        guacd_log(GUAC_LOG_DEBUG, "Exiting and passing control to PID %i", pid);
        _exit(0);
    }

    /* Change to root directory */
    if (chdir(GUACD_ROOT) < 0) {
        guacd_log(GUAC_LOG_ERROR, 
                "Unable to change working directory to "
                GUACD_ROOT);
        return 1;
    }

    /* Reopen the 3 stdxxx to /dev/null */

    if (redirect_fd(STDIN_FILENO, O_RDONLY)
    || redirect_fd(STDOUT_FILENO, O_WRONLY)
    || redirect_fd(STDERR_FILENO, O_WRONLY)) {

        guacd_log(GUAC_LOG_ERROR, 
                "Unable to redirect standard file descriptors to "
                GUACD_DEV_NULL);
        return 1;
    }

    /* Success */
    return 0;

}

#ifdef ENABLE_SSL
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
#endif

int main(int argc, char* argv[]) {

    /* Server */
    int socket_fd;
    struct addrinfo* addresses;
    struct addrinfo* current_address;
    char bound_address[1024];
    char bound_port[64];
    int opt_on = 1;

    struct addrinfo hints = {
        .ai_family   = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP
    };

    /* Client */
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    int connected_socket_fd;

#ifdef ENABLE_SSL
    SSL_CTX* ssl_context = NULL;
#endif

    guacd_proc_map* map = guacd_proc_map_alloc();

    /* General */
    int retval;

    /* Load configuration */
    guacd_config* config = guacd_conf_load();
    if (config == NULL || guacd_conf_parse_args(config, argc, argv))
       exit(EXIT_FAILURE);

    /* Init logging as early as possible */
    guacd_log_level = config->max_log_level;
    openlog(GUACD_LOG_NAME, LOG_PID, LOG_DAEMON);

    /* Log start */
    guacd_log(GUAC_LOG_INFO, "Guacamole proxy daemon (guacd) version " VERSION " started");

    /* Get addresses for binding */
    if ((retval = getaddrinfo(config->bind_host, config->bind_port,
                    &hints, &addresses))) {

        guacd_log(GUAC_LOG_ERROR, "Error parsing given address or port: %s",
                gai_strerror(retval));
        exit(EXIT_FAILURE);

    }

    /* Get socket */
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        guacd_log(GUAC_LOG_ERROR, "Error opening socket: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Allow socket reuse */
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR,
                (void*) &opt_on, sizeof(opt_on))) {
        guacd_log(GUAC_LOG_WARNING, "Unable to set socket options for reuse: %s",
                strerror(errno));
    }

    /* Attempt binding of each address until success */
    current_address = addresses;
    while (current_address != NULL) {

        int retval;

        /* Resolve hostname */
        if ((retval = getnameinfo(current_address->ai_addr,
                current_address->ai_addrlen,
                bound_address, sizeof(bound_address),
                bound_port, sizeof(bound_port),
                NI_NUMERICHOST | NI_NUMERICSERV)))
            guacd_log(GUAC_LOG_ERROR, "Unable to resolve host: %s",
                    gai_strerror(retval));

        /* Attempt to bind socket to address */
        if (bind(socket_fd,
                    current_address->ai_addr,
                    current_address->ai_addrlen) == 0) {

            guacd_log(GUAC_LOG_DEBUG, "Successfully bound socket to "
                    "host %s, port %s", bound_address, bound_port);

            /* Done if successful bind */
            break;

        }

        /* Otherwise log information regarding bind failure */
        else
            guacd_log(GUAC_LOG_DEBUG, "Unable to bind socket to "
                    "host %s, port %s: %s",
                    bound_address, bound_port, strerror(errno));

        current_address = current_address->ai_next;

    }

    /* If unable to bind to anything, fail */
    if (current_address == NULL) {
        guacd_log(GUAC_LOG_ERROR, "Unable to bind socket to any addresses.");
        exit(EXIT_FAILURE);
    }

#ifdef ENABLE_SSL
    /* Init SSL if enabled */
    if (config->key_file != NULL || config->cert_file != NULL) {

        guacd_log(GUAC_LOG_INFO, "Communication will require SSL/TLS.");

        /* Init threadsafety in OpenSSL */
        guacd_openssl_init_locks(CRYPTO_num_locks());
        CRYPTO_set_id_callback(guacd_openssl_id_callback);
        CRYPTO_set_locking_callback(guacd_openssl_locking_callback);

        /* Init SSL */
        SSL_library_init();
        SSL_load_error_strings();
        ssl_context = SSL_CTX_new(SSLv23_server_method());

        /* Load key */
        if (config->key_file != NULL) {
            guacd_log(GUAC_LOG_INFO, "Using PEM keyfile %s", config->key_file);
            if (!SSL_CTX_use_PrivateKey_file(ssl_context, config->key_file, SSL_FILETYPE_PEM)) {
                guacd_log(GUAC_LOG_ERROR, "Unable to load keyfile.");
                exit(EXIT_FAILURE);
            }
        }
        else
            guacd_log(GUAC_LOG_WARNING, "No PEM keyfile given - SSL/TLS may not work.");

        /* Load cert file if specified */
        if (config->cert_file != NULL) {
            guacd_log(GUAC_LOG_INFO, "Using certificate file %s", config->cert_file);
            if (!SSL_CTX_use_certificate_chain_file(ssl_context, config->cert_file)) {
                guacd_log(GUAC_LOG_ERROR, "Unable to load certificate.");
                exit(EXIT_FAILURE);
            }
        }
        else
            guacd_log(GUAC_LOG_WARNING, "No certificate file given - SSL/TLS may not work.");

    }
#endif

    /* Daemonize if requested */
    if (!config->foreground) {

        /* Attempt to daemonize process */
        if (daemonize()) {
            guacd_log(GUAC_LOG_ERROR, "Could not become a daemon.");
            exit(EXIT_FAILURE);
        }

    }

    /* Write PID file if requested */
    if (config->pidfile != NULL) {

        /* Attempt to open pidfile and write PID */
        FILE* pidf = fopen(config->pidfile, "w");
        if (pidf) {
            fprintf(pidf, "%d\n", getpid());
            fclose(pidf);
        }
        
        /* Fail if could not write PID file*/
        else {
            guacd_log(GUAC_LOG_ERROR, "Could not write PID file: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }

    }

    /* Ignore SIGPIPE */
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        guacd_log(GUAC_LOG_INFO, "Could not set handler for SIGPIPE to ignore. "
                "SIGPIPE may cause termination of the daemon.");
    }

    /* Ignore SIGCHLD (force automatic removal of children) */
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
        guacd_log(GUAC_LOG_INFO, "Could not set handler for SIGCHLD to ignore. "
                "Child processes may pile up in the process table.");
    }

    /* Log listening status */
    guacd_log(GUAC_LOG_INFO, "Listening on host %s, port %s", bound_address, bound_port);

    /* Free addresses */
    freeaddrinfo(addresses);

    /* Listen for connections */
    if (listen(socket_fd, 5) < 0) {
        guacd_log(GUAC_LOG_ERROR, "Could not listen on socket: %s", strerror(errno));
        return 3;
    }

    /* Daemon loop */
    for (;;) {

        pthread_t child_thread;

        /* Accept connection */
        client_addr_len = sizeof(client_addr);
        connected_socket_fd = accept(socket_fd,
                (struct sockaddr*) &client_addr, &client_addr_len);

        if (connected_socket_fd < 0) {
            guacd_log(GUAC_LOG_ERROR, "Could not accept client connection: %s", strerror(errno));
            continue;
        }

        /* Create parameters for connection thread */
        guacd_connection_thread_params* params = malloc(sizeof(guacd_connection_thread_params));
        if (params == NULL) {
            guacd_log(GUAC_LOG_ERROR, "Could not create connection thread: %s", strerror(errno));
            continue;
        }

        params->map = map;
        params->connected_socket_fd = connected_socket_fd;

#ifdef ENABLE_SSL
        params->ssl_context = ssl_context;
#endif

        /* Spawn thread to handle connection */
        pthread_create(&child_thread, NULL, guacd_connection_thread, params);
        pthread_detach(child_thread);

    }

    /* Close socket */
    if (close(socket_fd) < 0) {
        guacd_log(GUAC_LOG_ERROR, "Could not close socket: %s", strerror(errno));
        return 3;
    }

#ifdef ENABLE_SSL
    if (ssl_context != NULL) {
        guacd_openssl_free_locks(CRYPTO_num_locks());
        SSL_CTX_free(ssl_context);
    }
#endif

    return 0;

}

