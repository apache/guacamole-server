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

#include "client.h"
#include "sftp.h"
#include "ssh_key.h"
#include "terminal.h"

#ifdef ENABLE_SSH_AGENT
#include "ssh_agent.h"
#endif

#include <libssh2.h>
#include <libssh2_sftp.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <openssl/ssl.h>

#ifdef LIBSSH2_USES_GCRYPT
#include <gcrypt.h>
#endif

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>

void* ssh_input_thread(void* data) {

    guac_client* client = (guac_client*) data;
    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;

    char buffer[8192];
    int bytes_read;

    /* Write all data read */
    while ((bytes_read = guac_terminal_read_stdin(client_data->term, buffer, sizeof(buffer))) > 0) {
        pthread_mutex_lock(&(client_data->term_channel_lock));
        libssh2_channel_write(client_data->term_channel, buffer, bytes_read);
        pthread_mutex_unlock(&(client_data->term_channel_lock));
    }

    return NULL;

}

static int __sign_callback(LIBSSH2_SESSION* session,
        unsigned char** sig, size_t* sig_len,
        const unsigned char* data, size_t data_len, void **abstract) {

    ssh_key* key = (ssh_key*) abstract;
    int length;

    /* Allocate space for signature */
    *sig = malloc(4096);

    /* Sign with key */
    length = ssh_key_sign(key, (const char*) data, data_len, *sig);
    if (length < 0)
        return 1;

    *sig_len = length;
    return 0;
}

/**
 * Callback for the keyboard-interactive authentication method. Currently
 * suports just one prompt for the password.
 */
static void __kbd_callback(const char *name, int name_len,
                            const char *instruction, int instruction_len,
                            int num_prompts,
                            const LIBSSH2_USERAUTH_KBDINT_PROMPT *prompts,
                            LIBSSH2_USERAUTH_KBDINT_RESPONSE *responses,
                            void **abstract) {

    guac_client* client = (guac_client*) *abstract;
    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;

    if (num_prompts == 1) {
        responses[0].text = strdup(client_data->password);
        responses[0].length = strlen(client_data->password);
    }
    else
        guac_client_log(client, GUAC_LOG_WARNING,
                "Unsupported number of keyboard-interactive prompts: %i",
                num_prompts);

}

static LIBSSH2_SESSION* __guac_ssh_create_session(guac_client* client,
        int* socket_fd) {

    int retval;

    int fd;
    struct addrinfo* addresses;
    struct addrinfo* current_address;

    char connected_address[1024];
    char connected_port[64];
    char *user_authlist;

    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;

    struct addrinfo hints = {
        .ai_family   = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP
    };

    /* Get socket */
    fd = socket(AF_INET, SOCK_STREAM, 0);

    /* Get addresses connection */
    if ((retval = getaddrinfo(client_data->hostname, client_data->port,
                    &hints, &addresses))) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR, "Error parsing given address or port: %s",
                gai_strerror(retval));
        return NULL;

    }

    /* Attempt connection to each address until success */
    current_address = addresses;
    while (current_address != NULL) {

        int retval;

        /* Resolve hostname */
        if ((retval = getnameinfo(current_address->ai_addr,
                current_address->ai_addrlen,
                connected_address, sizeof(connected_address),
                connected_port, sizeof(connected_port),
                NI_NUMERICHOST | NI_NUMERICSERV)))
            guac_client_log(client, GUAC_LOG_DEBUG, "Unable to resolve host: %s", gai_strerror(retval));

        /* Connect */
        if (connect(fd, current_address->ai_addr,
                        current_address->ai_addrlen) == 0) {

            guac_client_log(client, GUAC_LOG_DEBUG, "Successfully connected to "
                    "host %s, port %s", connected_address, connected_port);

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
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR, "Unable to connect to any addresses.");
        return NULL;
    }

    /* Free addrinfo */
    freeaddrinfo(addresses);

    /* Open SSH session */
    LIBSSH2_SESSION* session = libssh2_session_init_ex(NULL, NULL,
            NULL, client);
    if (session == NULL) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR, "Session allocation failed.");
        return NULL;
    }

    /* Perform handshake */
    if (libssh2_session_handshake(session, fd)) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR, "SSH handshake failed.");
        return NULL;
    }

    /* Save file descriptor */
    if (socket_fd != NULL)
        *socket_fd = fd;

    /* Get list of suported authentication methods */
    user_authlist = libssh2_userauth_list(session, client_data->username, strlen(client_data->username));
    guac_client_log(client, GUAC_LOG_DEBUG, "Supported authentication methods: %s", user_authlist);

    /* Authenticate with key if available */
    if (client_data->key != NULL) {

        /* Check if public key auth is suported on the server */
        if (strstr(user_authlist, "publickey") == NULL) {
            guac_client_abort(client, GUAC_PROTOCOL_STATUS_CLIENT_UNAUTHORIZED,
                               "Public key authentication not suported");
            return NULL;
        }

        if (!libssh2_userauth_publickey(session, client_data->username,
                    (unsigned char*) client_data->key->public_key,
                    client_data->key->public_key_length,
                    __sign_callback, (void**) client_data->key))
            return session;
        else {
            char* error_message;
            libssh2_session_last_error(session, &error_message, NULL, 0);
            guac_client_abort(client, GUAC_PROTOCOL_STATUS_CLIENT_UNAUTHORIZED,
                    "Public key authentication failed: %s", error_message);
            return NULL;
        }
    }

    /* Authenticate with password */
    if (strstr(user_authlist, "password") != NULL) {
        guac_client_log(client, GUAC_LOG_DEBUG, "Using password authentication method");
        retval = libssh2_userauth_password(session, client_data->username, client_data->password);
    }
    else if (strstr(user_authlist, "keyboard-interactive") != NULL) {
        guac_client_log(client, GUAC_LOG_DEBUG, "Using keyboard-interactive authentication method");
        retval = libssh2_userauth_keyboard_interactive(session, client_data->username, &__kbd_callback);
    }
    else {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_CLIENT_BAD_TYPE, "No known authentication methods");
        return NULL;
    }

    if (retval == 0)
        return session;

    else {
        char* error_message;
        libssh2_session_last_error(session, &error_message, NULL, 0);
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_CLIENT_UNAUTHORIZED,
                "Password authentication failed: %s", error_message);
        return NULL;
    }

}

#ifdef LIBSSH2_USES_GCRYPT
GCRY_THREAD_OPTION_PTHREAD_IMPL;
#endif

/**
 * Array of mutexes, used by OpenSSL.
 */
static pthread_mutex_t* __openssl_locks;

/**
 * Called by OpenSSL when locking or unlocking the Nth mutex.
 */
static void __openssl_locking_callback(int mode, int n, const char* file, int line){
    if (mode & CRYPTO_LOCK)
        pthread_mutex_lock(&(__openssl_locks[n]));
    else if (mode & CRYPTO_UNLOCK)
        pthread_mutex_unlock(&(__openssl_locks[n]));
}

/**
 * Called by OpenSSL when determining the current thread ID.
 */
static unsigned long __openssl_id_callback() {
    return (unsigned long) pthread_self();
}

/**
 * Creates the given number of mutexes, such that OpenSSL will have at least
 * this number of mutexes at its disposal.
 */
static void __openssl_init_locks(int count) {

    int i;

    __openssl_locks = malloc(sizeof(pthread_mutex_t) * CRYPTO_num_locks());

    for (i=0; i<count; i++)
        pthread_mutex_init(&(__openssl_locks[i]), NULL);

}

/**
 * Frees the given number of mutexes.
 */
static void __openssl_free_locks(int count) {

    int i;

    for (i=0; i<count; i++)
        pthread_mutex_destroy(&(__openssl_locks[i]));

}

void* ssh_client_thread(void* data) {

    guac_client* client = (guac_client*) data;
    ssh_guac_client_data* client_data = (ssh_guac_client_data*) client->data;

    char name[1024];

    guac_socket* socket = client->socket;
    char buffer[8192];
    int bytes_read = -1234;

    int socket_fd;

    pthread_t input_thread;

#ifdef LIBSSH2_USES_GCRYPT
    /* Init threadsafety in libgcrypt */
    gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
    if (!gcry_check_version(GCRYPT_VERSION)) {
        guac_client_log(client, GUAC_LOG_ERROR, "libgcrypt version mismatch.");
        return NULL;
    }
#endif

    /* Init threadsafety in OpenSSL */
    __openssl_init_locks(CRYPTO_num_locks());
    CRYPTO_set_id_callback(__openssl_id_callback);
    CRYPTO_set_locking_callback(__openssl_locking_callback);

    SSL_library_init();
    libssh2_init(0);

    /* Get username */
    if (client_data->username[0] == 0)
        guac_terminal_prompt(client_data->term, "Login as: ",
                             client_data->username, sizeof(client_data->username), true);

    /* Send new name */
    snprintf(name, sizeof(name)-1, "%s@%s", client_data->username, client_data->hostname);
    guac_protocol_send_name(socket, name);

    /* If key specified, import */
    if (client_data->key_base64[0] != 0) {

        /* Attempt to read key without passphrase */
        client_data->key = ssh_key_alloc(client_data->key_base64,
                strlen(client_data->key_base64), "");

        /* On failure, attempt with passphrase */
        if (client_data->key == NULL) {

            /* Prompt for passphrase if missing */
            if (client_data->key_passphrase[0] == 0)
                guac_terminal_prompt(client_data->term, "Key passphrase: ",
                                     client_data->key_passphrase, sizeof(client_data->key_passphrase), false);

            /* Import key with passphrase */
            client_data->key = ssh_key_alloc(client_data->key_base64,
                    strlen(client_data->key_base64),
                    client_data->key_passphrase);

            /* If still failing, give up */
            if (client_data->key == NULL) {
                guac_client_log(client, GUAC_LOG_ERROR, "Auth key import failed.");
                return NULL;
            }

        } /* end decrypt key with passphrase */

        /* Success */
        guac_client_log(client, GUAC_LOG_INFO, "Auth key successfully imported.");

    } /* end if key given */

    /* Otherwise, get password if not provided */
    else if (client_data->password[0] == 0)
        guac_terminal_prompt(client_data->term, "Password: ",
                             client_data->password, sizeof(client_data->password), false);

    /* Clear screen */
    guac_terminal_printf(client_data->term, "\x1B[H\x1B[J");

    /* Open SSH session */
    client_data->session = __guac_ssh_create_session(client, &socket_fd);
    if (client_data->session == NULL) {
        /* Already aborted within __guac_ssh_create_session() */
        return NULL;
    }

    pthread_mutex_init(&client_data->term_channel_lock, NULL);

    /* Open channel for terminal */
    client_data->term_channel = libssh2_channel_open_session(client_data->session);
    if (client_data->term_channel == NULL) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR, "Unable to open terminal channel.");
        return NULL;
    }

#ifdef ENABLE_SSH_AGENT
    /* Start SSH agent forwarding, if enabled */
    if (client_data->enable_agent) {
        libssh2_session_callback_set(client_data->session,
                LIBSSH2_CALLBACK_AUTH_AGENT, (void*) ssh_auth_agent_callback);

        /* Request agent forwarding */
        if (libssh2_channel_request_auth_agent(client_data->term_channel))
            guac_client_log(client, GUAC_LOG_ERROR, "Agent forwarding request failed");
        else
            guac_client_log(client, GUAC_LOG_INFO, "Agent forwarding enabled.");
    }

    client_data->auth_agent = NULL;
#endif

    /* Start SFTP session as well, if enabled */
    if (client_data->enable_sftp) {

        /* Init handlers for Guacamole-specific console codes */
        client_data->term->upload_path_handler = guac_sftp_set_upload_path;
        client_data->term->file_download_handler = guac_sftp_download_file;

        /* Create SSH session specific for SFTP */
        guac_client_log(client, GUAC_LOG_DEBUG, "Reconnecting for SFTP...");
        client_data->sftp_ssh_session = __guac_ssh_create_session(client, NULL);
        if (client_data->sftp_ssh_session == NULL) {
            /* Already aborted within __guac_ssh_create_session() */
            return NULL;
        }

        /* Request SFTP */
        client_data->sftp_session = libssh2_sftp_init(client_data->sftp_ssh_session);
        if (client_data->sftp_session == NULL) {
            guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR, "Unable to start SFTP session.");
            return NULL;
        }

        /* Set file handler */
        client->file_handler = guac_sftp_file_handler;

        guac_client_log(client, GUAC_LOG_DEBUG, "SFTP session initialized");

    }

    /* Request PTY */
    if (libssh2_channel_request_pty_ex(client_data->term_channel, "linux", sizeof("linux")-1, NULL, 0,
            client_data->term->term_width, client_data->term->term_height, 0, 0)) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR, "Unable to allocate PTY.");
        return NULL;
    }

    /* Request shell */
    if (libssh2_channel_shell(client_data->term_channel)) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR, "Unable to associate shell with PTY.");
        return NULL;
    }

    /* Logged in */
    guac_client_log(client, GUAC_LOG_INFO, "SSH connection successful.");

    /* Start input thread */
    if (pthread_create(&(input_thread), NULL, ssh_input_thread, (void*) client)) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR, "Unable to start input thread");
        return NULL;
    }

    /* Set non-blocking */
    libssh2_session_set_blocking(client_data->session, 0);

    /* While data available, write to terminal */
    bytes_read = 0;
    for (;;) {

        /* Track total amount of data read */
        int total_read = 0;

        pthread_mutex_lock(&(client_data->term_channel_lock));

        /* Stop reading at EOF */
        if (libssh2_channel_eof(client_data->term_channel)) {
            pthread_mutex_unlock(&(client_data->term_channel_lock));
            break;
        }

        /* Read terminal data */
        bytes_read = libssh2_channel_read(client_data->term_channel,
                buffer, sizeof(buffer));

        pthread_mutex_unlock(&(client_data->term_channel_lock));

        /* Attempt to write data received. Exit on failure. */
        if (bytes_read > 0) {
            int written = guac_terminal_write_stdout(client_data->term, buffer, bytes_read);
            if (written < 0)
                break;

            total_read += bytes_read;
        }

        else if (bytes_read < 0 && bytes_read != LIBSSH2_ERROR_EAGAIN)
            break;

#ifdef ENABLE_SSH_AGENT
        /* If agent open, handle any agent packets */
        if (client_data->auth_agent != NULL) {
            bytes_read = ssh_auth_agent_read(client_data->auth_agent);
            if (bytes_read > 0)
                total_read += bytes_read;
            else if (bytes_read < 0 && bytes_read != LIBSSH2_ERROR_EAGAIN)
                client_data->auth_agent = NULL;
        }
#endif

        /* Wait for more data if reads turn up empty */
        if (total_read == 0) {
            fd_set fds;
            struct timeval timeout;

            FD_ZERO(&fds);
            FD_SET(socket_fd, &fds);

            /* Wait for one second */
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;

            if (select(socket_fd+1, &fds, NULL, NULL, &timeout) < 0)
                break;
        }

    }

    /* Kill client and Wait for input thread to die */
    guac_client_stop(client);
    pthread_join(input_thread, NULL);

    __openssl_free_locks(CRYPTO_num_locks());
    pthread_mutex_destroy(&client_data->term_channel_lock);

    guac_client_log(client, GUAC_LOG_INFO, "SSH connection ended.");
    return NULL;

}

