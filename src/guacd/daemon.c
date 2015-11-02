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
#include "client-map.h"
#include "conf-args.h"
#include "conf-file.h"
#include "log.h"

#include <guacamole/client.h>
#include <guacamole/error.h>
#include <guacamole/instruction.h>
#include <guacamole/plugin.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>

#ifdef ENABLE_SSL
#include <openssl/ssl.h>
#include "socket-ssl.h"
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
 * Logs a reasonable explanatory message regarding handshake failure based on
 * the current value of guac_error.
 */
static void guacd_log_handshake_failure() {

    if (guac_error == GUAC_STATUS_CLOSED)
        guacd_log(GUAC_LOG_INFO,
                "Guacamole connection closed during handshake");
    else if (guac_error == GUAC_STATUS_PROTOCOL_ERROR)
        guacd_log(GUAC_LOG_ERROR,
                "Guacamole protocol violation. Perhaps the version of "
                "guacamole-client is incompatible with this version of "
                "guacd?");
    else
        guacd_log(GUAC_LOG_WARNING,
                "Guacamole handshake failed: %s",
                guac_status_string(guac_error));

}

/**
 * Copies the given array of mimetypes (strings) into a newly-allocated NULL-
 * terminated array of strings. Both the array and the strings within the array
 * are newly-allocated and must be later freed via guacd_free_mimetypes().
 *
 * @param mimetypes
 *     The array of mimetypes to copy.
 *
 * @param count
 *     The number of mimetypes in the given array.
 *
 * @return
 *     A newly-allocated, NULL-terminated array containing newly-allocated
 *     copies of each of the mimetypes provided in the original mimetypes
 *     array.
 */
static char** guacd_copy_mimetypes(char** mimetypes, int count) {

    int i;

    /* Allocate sufficient space for NULL-terminated array of mimetypes */
    char** mimetypes_copy = malloc(sizeof(char*) * (count+1));

    /* Copy each provided mimetype */
    for (i = 0; i < count; i++)
        mimetypes_copy[i] = strdup(mimetypes[i]);

    /* Terminate with NULL */
    mimetypes_copy[count] = NULL;

    return mimetypes_copy;

}

/**
 * Frees the given array of mimetypes, including the space allocated to each
 * mimetype string within the array. The provided array of mimetypes MUST have
 * been allocated with guacd_copy_mimetypes().
 *
 * @param mimetypes
 *     The NULL-terminated array of mimetypes to free. This array MUST have
 *     been previously allocated with guacd_copy_mimetypes().
 */
static void guacd_free_mimetypes(char** mimetypes) {

    char** current_mimetype = mimetypes;

    /* Free all strings within NULL-terminated mimetype array */
    while (*current_mimetype != NULL) {
        free(*current_mimetype);
        current_mimetype++;
    }

    /* Free the array itself, now that its contents have been freed */
    free(mimetypes);

}

/**
 * Creates a new guac_client for the connection on the given socket, adding
 * it to the client map based on its ID.
 */
static void guacd_handle_connection(guacd_client_map* map, guac_socket* socket) {

    guac_client* client;
    guac_client_plugin* plugin;
    guac_instruction* select;
    guac_instruction* size;
    guac_instruction* audio;
    guac_instruction* video;
    guac_instruction* image;
    guac_instruction* connect;
    int init_result;

    /* Reset guac_error */
    guac_error = GUAC_STATUS_SUCCESS;
    guac_error_message = NULL;

    /* Get protocol from select instruction */
    select = guac_instruction_expect(socket, GUACD_USEC_TIMEOUT, "select");
    if (select == NULL) {

        /* Log error */
        guacd_log_handshake_failure();
        guacd_log_guac_error(GUAC_LOG_DEBUG,
                "Error reading \"select\"");

        /* Free resources */
        guac_socket_free(socket);
        return;

    }

    /* Validate args to select */
    if (select->argc != 1) {

        /* Log error */
        guacd_log_handshake_failure();
        guacd_log(GUAC_LOG_ERROR, "Bad number of arguments to \"select\" (%i)",
                select->argc);

        /* Free resources */
        guac_socket_free(socket);
        return;
    }

    guacd_log(GUAC_LOG_INFO, "Protocol \"%s\" selected", select->argv[0]);

    /* Get plugin from protocol in select */
    plugin = guac_client_plugin_open(select->argv[0]);
    guac_instruction_free(select);

    if (plugin == NULL) {

        /* Log error */
        if (guac_error == GUAC_STATUS_NOT_FOUND)
            guacd_log(GUAC_LOG_WARNING,
                    "Support for selected protocol is not installed");
        else
            guacd_log_guac_error(GUAC_LOG_ERROR,
                    "Unable to load client plugin");

        /* Free resources */
        guac_socket_free(socket);
        return;
    }

    /* Send args response */
    if (guac_protocol_send_args(socket, plugin->args)
            || guac_socket_flush(socket)) {

        /* Log error */
        guacd_log_handshake_failure();
        guacd_log_guac_error(GUAC_LOG_DEBUG, "Error sending \"args\"");

        if (guac_client_plugin_close(plugin))
            guacd_log_guac_error(GUAC_LOG_WARNING,
                    "Unable to close client plugin");

        guac_socket_free(socket);
        return;
    }

    /* Get client */
    client = guac_client_alloc();
    if (client == NULL) {
        guacd_log_guac_error(GUAC_LOG_ERROR, "Unable to create client");
        guac_socket_free(socket);
        return;
    }

    /* Get optimal screen size */
    size = guac_instruction_expect(
            socket, GUACD_USEC_TIMEOUT, "size");
    if (size == NULL) {

        /* Log error */
        guacd_log_handshake_failure();
        guacd_log_guac_error(GUAC_LOG_DEBUG, "Error reading \"size\"");

        /* Free resources */
        guac_client_free(client);
        guac_socket_free(socket);
        return;
    }

    /* Parse optimal screen dimensions from size instruction */
    client->info.optimal_width  = atoi(size->argv[0]);
    client->info.optimal_height = atoi(size->argv[1]);

    /* If DPI given, set the client resolution */
    if (size->argc >= 3)
        client->info.optimal_resolution = atoi(size->argv[2]);

    /* Otherwise, use a safe default for rough backwards compatibility */
    else
        client->info.optimal_resolution = 96;

    guac_instruction_free(size);

    /* Get supported audio formats */
    audio = guac_instruction_expect(
            socket, GUACD_USEC_TIMEOUT, "audio");
    if (audio == NULL) {

        /* Log error */
        guacd_log_handshake_failure();
        guacd_log_guac_error(GUAC_LOG_DEBUG, "Error reading \"audio\"");

        /* Free resources */
        guac_client_free(client);
        guac_socket_free(socket);
        return;
    }

    /* Store audio mimetypes */
    client->info.audio_mimetypes =
        guacd_copy_mimetypes(audio->argv, audio->argc);

    guac_instruction_free(audio);

    /* Get supported video formats */
    video = guac_instruction_expect(
            socket, GUACD_USEC_TIMEOUT, "video");
    if (video == NULL) {

        /* Log error */
        guacd_log_handshake_failure();
        guacd_log_guac_error(GUAC_LOG_DEBUG, "Error reading \"video\"");

        /* Free resources */
        guac_client_free(client);
        guac_socket_free(socket);
        return;
    }

    /* Store video mimetypes */
    client->info.video_mimetypes =
        guacd_copy_mimetypes(video->argv, video->argc);

    guac_instruction_free(video);

    /* Get supported image formats */
    image = guac_instruction_expect(
            socket, GUACD_USEC_TIMEOUT, "image");
    if (image == NULL) {

        /* Log error */
        guacd_log_handshake_failure();
        guacd_log_guac_error(GUAC_LOG_DEBUG, "Error reading \"image\"");

        /* Free resources */
        guac_client_free(client);
        guac_socket_free(socket);
        return;
    }

    /* Store image mimetypes */
    client->info.image_mimetypes =
        guacd_copy_mimetypes(image->argv, image->argc);

    guac_instruction_free(image);

    /* Get args from connect instruction */
    connect = guac_instruction_expect(
            socket, GUACD_USEC_TIMEOUT, "connect");
    if (connect == NULL) {

        /* Log error */
        guacd_log_handshake_failure();
        guacd_log_guac_error(GUAC_LOG_DEBUG, "Error reading \"connect\"");

        if (guac_client_plugin_close(plugin))
            guacd_log_guac_error(GUAC_LOG_WARNING,
                    "Unable to close client plugin");

        guac_client_free(client);
        guac_socket_free(socket);
        return;
    }

    client->socket = socket;
    client->log_handler = guacd_client_log;

    /* Store client */
    if (guacd_client_map_add(map, client))
        guacd_log(GUAC_LOG_ERROR, "Unable to add client. Internal client storage has failed");

    /* Send connection ID */
    guacd_log(GUAC_LOG_INFO, "Connection ID is \"%s\"", client->connection_id);
    guac_protocol_send_ready(socket, client->connection_id);

    /* Init client */
    init_result = guac_client_plugin_init_client(plugin,
                client, connect->argc, connect->argv);

    guac_instruction_free(connect);

    /* If client could not be started, free everything and fail */
    if (init_result) {

        guac_client_free(client);

        guacd_log_guac_error(GUAC_LOG_INFO, "Connection did not succeed");

        if (guac_client_plugin_close(plugin))
            guacd_log_guac_error(GUAC_LOG_WARNING,
                    "Unable to close client plugin");

        guac_socket_free(socket);
        return;
    }

    /* Start client threads */
    guacd_log(GUAC_LOG_INFO, "Starting client");
    if (guacd_client_start(client))
        guacd_log(GUAC_LOG_WARNING, "Client finished abnormally");
    else
        guacd_log(GUAC_LOG_INFO, "Client disconnected");

    /* Remove client */
    if (guacd_client_map_remove(map, client->connection_id) == NULL)
        guacd_log(GUAC_LOG_ERROR, "Unable to remove client. Internal client storage has failed");

    /* Free mimetype lists */
    guacd_free_mimetypes(client->info.audio_mimetypes);
    guacd_free_mimetypes(client->info.video_mimetypes);
    guacd_free_mimetypes(client->info.image_mimetypes);

    /* Clean up */
    guac_client_free(client);
    if (guac_client_plugin_close(plugin))
        guacd_log_guac_error(GUAC_LOG_WARNING,
                "Unable to close client plugin");

    /* Close socket */
    guac_socket_free(socket);

}

int redirect_fd(int fd, int flags) {

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

int daemonize() {

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

    guacd_client_map* map = guacd_client_map_alloc();

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

        /* Init SSL */
        guacd_log(GUAC_LOG_INFO, "Communication will require SSL/TLS.");
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

        pid_t child_pid;

        /* Accept connection */
        client_addr_len = sizeof(client_addr);
        connected_socket_fd = accept(socket_fd,
                (struct sockaddr*) &client_addr, &client_addr_len);

        if (connected_socket_fd < 0) {
            guacd_log(GUAC_LOG_ERROR, "Could not accept client connection: %s",
                    strerror(errno));
            return 3;
        }

        /* 
         * Once connection is accepted, send child into background.
         *
         * Note that we prefer fork() over threads for connection-handling
         * processes as they give each connection its own memory area, and
         * isolate the main daemon and other connections from errors in any
         * particular client plugin.
         */

        child_pid = fork();

        /* If error, log */
        if (child_pid == -1)
            guacd_log(GUAC_LOG_ERROR, "Error forking child process: %s", strerror(errno));

        /* If child, start client, and exit when finished */
        else if (child_pid == 0) {

            guac_socket* socket;

#ifdef ENABLE_SSL

            /* If SSL chosen, use it */
            if (ssl_context != NULL) {
                socket = guac_socket_open_secure(ssl_context, connected_socket_fd);
                if (socket == NULL) {
                    guacd_log_guac_error(GUAC_LOG_ERROR,
                            "Unable to set up SSL/TLS");
                    return 0;
                }
            }
            else
                socket = guac_socket_open(connected_socket_fd);
#else
            /* Open guac_socket */
            socket = guac_socket_open(connected_socket_fd);
#endif

            guacd_handle_connection(map, socket);
            close(connected_socket_fd);
            return 0;
        }

        /* If parent, close reference to child's descriptor */
        else if (close(connected_socket_fd) < 0) {
            guacd_log(GUAC_LOG_ERROR, "Error closing daemon reference to "
                    "child descriptor: %s", strerror(errno));
        }

    }

    /* Close socket */
    if (close(socket_fd) < 0) {
        guacd_log(GUAC_LOG_ERROR, "Could not close socket: %s", strerror(errno));
        return 3;
    }

    return 0;

}

