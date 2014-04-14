
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is guacd.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * David PHAM-VAN <d.pham-van@ulteo.com> Ulteo SAS - http://www.ulteo.com
 * Alex Bligh <alex@alex.org.uk>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>

#include <errno.h>
#include <syslog.h>
#include <libgen.h>

#ifdef ENABLE_SSL
#include <openssl/ssl.h>
#include "socket-ssl.h"
#endif

#include <guacamole/client.h>
#include <guacamole/error.h>
#include <guacamole/instruction.h>
#include <guacamole/plugin.h>
#include <guacamole/protocol.h>

#include "client.h"
#include "log.h"

#define GUACD_DEV_NULL "/dev/null"
#define GUACD_ROOT     "/"

void guacd_handle_connection(guac_socket* socket) {

    guac_client* client;
    guac_client_plugin* plugin;
    guac_instruction* select;
    guac_instruction* size;
    guac_instruction* audio;
    guac_instruction* video;
    guac_instruction* connect;
    int init_result;

    /* Reset guac_error */
    guac_error = GUAC_STATUS_SUCCESS;
    guac_error_message = NULL;

    /* Get protocol from select instruction */
    select = guac_instruction_expect(
            socket, GUACD_USEC_TIMEOUT, "select");
    if (select == NULL) {

        /* Log error */
        guacd_log_guac_error("Error reading \"select\"");

        /* Free resources */
        guac_socket_free(socket);
        return;
    }

    /* Validate args to select */
    if (select->argc != 1) {

        /* Log error */
        guacd_log_error("Bad number of arguments to \"select\" (%i)",
                select->argc);

        /* Free resources */
        guac_socket_free(socket);
        return;
    }

    guacd_log_info("Protocol \"%s\" selected", select->argv[0]);

    /* Get plugin from protocol in select */
    plugin = guac_client_plugin_open(select->argv[0]);
    guac_instruction_free(select);

    if (plugin == NULL) {

        /* Log error */
        guacd_log_guac_error("Error loading client plugin");

        /* Free resources */
        guac_socket_free(socket);
        return;
    }

    /* Send args response */
    if (guac_protocol_send_args(socket, plugin->args)
            || guac_socket_flush(socket)) {

        /* Log error */
        guacd_log_guac_error("Error sending \"args\"");

        if (guac_client_plugin_close(plugin))
            guacd_log_guac_error("Error closing client plugin");

        guac_socket_free(socket);
        return;
    }

    /* Get optimal screen size */
    size = guac_instruction_expect(
            socket, GUACD_USEC_TIMEOUT, "size");
    if (size == NULL) {

        /* Log error */
        guacd_log_guac_error("Error reading \"size\"");

        /* Free resources */
        guac_socket_free(socket);
        return;
    }

    /* Get supported audio formats */
    audio = guac_instruction_expect(
            socket, GUACD_USEC_TIMEOUT, "audio");
    if (audio == NULL) {

        /* Log error */
        guacd_log_guac_error("Error reading \"audio\"");

        /* Free resources */
        guac_socket_free(socket);
        return;
    }

    /* Get supported video formats */
    video = guac_instruction_expect(
            socket, GUACD_USEC_TIMEOUT, "video");
    if (video == NULL) {

        /* Log error */
        guacd_log_guac_error("Error reading \"video\"");

        /* Free resources */
        guac_socket_free(socket);
        return;
    }

    /* Get args from connect instruction */
    connect = guac_instruction_expect(
            socket, GUACD_USEC_TIMEOUT, "connect");
    if (connect == NULL) {

        /* Log error */
        guacd_log_guac_error("Error reading \"connect\"");

        if (guac_client_plugin_close(plugin))
            guacd_log_guac_error("Error closing client plugin");

        guac_socket_free(socket);
        return;
    }

    /* Get client */
    client = guac_client_alloc();
    client->socket = socket;
    client->log_info_handler = guacd_client_log_info;
    client->log_error_handler = guacd_client_log_error;

    /* Parse optimal screen dimensions from size instruction */
    client->info.optimal_width  = atoi(size->argv[0]);
    client->info.optimal_height = atoi(size->argv[1]);

    /* Store audio mimetypes */
    client->info.audio_mimetypes = malloc(sizeof(char*) * (audio->argc+1));
    memcpy(client->info.audio_mimetypes, audio->argv,
            sizeof(char*) * audio->argc);
    client->info.audio_mimetypes[audio->argc] = NULL;

    /* Store video mimetypes */
    client->info.video_mimetypes = malloc(sizeof(char*) * (video->argc+1));
    memcpy(client->info.video_mimetypes, video->argv,
            sizeof(char*) * video->argc);
    client->info.video_mimetypes[video->argc] = NULL;

    /* Init client */
    init_result = guac_client_plugin_init_client(plugin,
                client, connect->argc, connect->argv);

    guac_instruction_free(connect);

    /* If client could not be started, free everything and fail */
    if (init_result) {

        guac_client_free(client);

        guacd_log_guac_error("Error instantiating client");

        if (guac_client_plugin_close(plugin))
            guacd_log_guac_error("Error closing client plugin");

        guac_socket_free(socket);
        return;
    }

    /* Start client threads */
    guacd_log_info("Starting client");
    if (guacd_client_start(client))
        guacd_log_error("Client finished abnormally");
    else
        guacd_log_info("Client finished normally");

    /* Free mimetype lists */
    free(client->info.audio_mimetypes);
    free(client->info.video_mimetypes);

    /* Free remaining instructions */
    guac_instruction_free(audio);
    guac_instruction_free(video);
    guac_instruction_free(size);

    /* Clean up */
    guac_client_free(client);
    if (guac_client_plugin_close(plugin))
        guacd_log_error("Error closing client plugin");

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
        guacd_log_error("Could not fork() parent: %s", strerror(errno));
        return 1;
    }

    /* Exit if we are the parent */
    if (pid > 0) {
        guacd_log_info("Exiting and passing control to PID %i", pid);
        _exit(0);
    }

    /* Start a new session (if not already group leader) */
    setsid();

    /* Fork again so the session group leader exits */
    pid = fork();
    if (pid < 0) {
        guacd_log_error("Could not fork() group leader: %s", strerror(errno));
        return 1;
    }

    /* Exit if we are the parent */
    if (pid > 0) {
        guacd_log_info("Exiting and passing control to PID %i", pid);
        _exit(0);
    }

    /* Change to root directory */
    if (chdir(GUACD_ROOT) < 0) {
        guacd_log_error(
                "Unable to change working directory to "
                GUACD_ROOT);
        return 1;
    }

    /* Reopen the 3 stdxxx to /dev/null */

    if (redirect_fd(STDIN_FILENO, O_RDONLY)
    || redirect_fd(STDOUT_FILENO, O_WRONLY)
    || redirect_fd(STDERR_FILENO, O_WRONLY)) {

        guacd_log_error(
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

    /* Arguments */
    char* listen_address = NULL; /* Default address of INADDR_ANY */
    char* listen_port = "4822";  /* Default port */
    char* pidfile = NULL;
    int opt;
    int foreground = 0;

#ifdef ENABLE_SSL
    /* SSL */
    char* cert_file = NULL;
    char* key_file = NULL;
    SSL_CTX* ssl_context = NULL;
#endif

    /* General */
    int retval;

    /* Parse arguments */
    while ((opt = getopt(argc, argv, "l:b:p:C:K:f")) != -1) {
        if (opt == 'l') {
            listen_port = strdup(optarg);
        }
        else if (opt == 'b') {
            listen_address = strdup(optarg);
        }
        else if (opt == 'f') {
            foreground = 1;
        }
        else if (opt == 'p') {
            pidfile = strdup(optarg);
        }
#ifdef ENABLE_SSL
        else if (opt == 'C') {
            cert_file = strdup(optarg);
        }
        else if (opt == 'K') {
            key_file = strdup(optarg);
        }
#else
        else if (opt == 'C' || opt == 'K') {
            fprintf(stderr,
                    "This guacd does not have SSL/TLS support compiled in.\n\n"

                    "If you wish to enable support for the -%c option, please install libssl and\n"
                    "recompile guacd.\n",
                    opt);
            exit(EXIT_FAILURE);
        }
#endif
        else {

            fprintf(stderr, "USAGE: %s"
                    " [-l LISTENPORT]"
                    " [-b LISTENADDRESS]"
                    " [-p PIDFILE]"
#ifdef ENABLE_SSL
                    " [-C CERTIFICATE_FILE]"
                    " [-K PEM_FILE]"
#endif
                    " [-f]\n", argv[0]);

            exit(EXIT_FAILURE);
        }
    }

    /* Set up logging prefix */
    strncpy(log_prefix, basename(argv[0]), sizeof(log_prefix));

    /* Open log as early as we can */
    openlog(NULL, LOG_PID, LOG_DAEMON);

    /* Log start */
    guacd_log_info("Guacamole proxy daemon (guacd) version " VERSION);

    /* Get addresses for binding */
    if ((retval = getaddrinfo(listen_address, listen_port,
                    &hints, &addresses))) {

        guacd_log_error("Error parsing given address or port: %s",
                gai_strerror(retval));
        exit(EXIT_FAILURE);

    }

    /* Get socket */
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        guacd_log_error("Error opening socket: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* Allow socket reuse */
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR,
                (void*) &opt_on, sizeof(opt_on))) {
        guacd_log_info("Unable to set socket options for reuse: %s",
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
            guacd_log_error("Unable to resolve host: %s",
                    gai_strerror(retval));

        /* Attempt to bind socket to address */
        if (bind(socket_fd,
                    current_address->ai_addr,
                    current_address->ai_addrlen) == 0) {

            guacd_log_info("Successfully bound socket to "
                    "host %s, port %s", bound_address, bound_port);

            /* Done if successful bind */
            break;

        }

        /* Otherwise log information regarding bind failure */
        else
            guacd_log_info("Unable to bind socket to "
                    "host %s, port %s: %s",
                    bound_address, bound_port, strerror(errno));

        current_address = current_address->ai_next;

    }

    /* If unable to bind to anything, fail */
    if (current_address == NULL) {
        guacd_log_error("Unable to bind socket to any addresses.");
        exit(EXIT_FAILURE);
    }

#ifdef ENABLE_SSL
    /* Init SSL if enabled */
    if (key_file != NULL || cert_file != NULL) {

        /* Init SSL */
        guacd_log_info("Communication will require SSL/TLS.");
        SSL_library_init();
        SSL_load_error_strings();
        ssl_context = SSL_CTX_new(SSLv23_server_method());

        /* Load key */
        if (key_file != NULL) {
            guacd_log_info("Using PEM keyfile %s", key_file);
            if (!SSL_CTX_use_PrivateKey_file(ssl_context, key_file, SSL_FILETYPE_PEM)) {
                guacd_log_error("Unable to load keyfile.");
                exit(EXIT_FAILURE);
            }
        }
        else
            guacd_log_info("No PEM keyfile given - SSL/TLS may not work.");

        /* Load cert file if specified */
        if (cert_file != NULL) {
            guacd_log_info("Using certificate file %s", cert_file);
            if (!SSL_CTX_use_certificate_file(ssl_context, cert_file, SSL_FILETYPE_PEM)) {
                guacd_log_error("Unable to load certificate.");
                exit(EXIT_FAILURE);
            }
        }
        else
            guacd_log_info("No certificate file given - SSL/TLS may not work.");

    }
#endif

    /* Daemonize if requested */
    if (!foreground) {

        /* Attempt to daemonize process */
        if (daemonize()) {
            guacd_log_error("Could not become a daemon.");
            exit(EXIT_FAILURE);
        }

    }

    /* Write PID file if requested */
    if (pidfile != NULL) {

        /* Attempt to open pidfile and write PID */
        FILE* pidf = fopen(pidfile, "w");
        if (pidf) {
            fprintf(pidf, "%d\n", getpid());
            fclose(pidf);
        }
        
        /* Fail if could not write PID file*/
        else {
            guacd_log_error("Could not write PID file: %s", strerror(errno));
            exit(EXIT_FAILURE);
        }

    }

    /* Ignore SIGPIPE */
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        guacd_log_info("Could not set handler for SIGPIPE to ignore. "
                "SIGPIPE may cause termination of the daemon.");
    }

    /* Ignore SIGCHLD (force automatic removal of children) */
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
        guacd_log_info("Could not set handler for SIGCHLD to ignore. "
                "Child processes may pile up in the process table.");
    }

    /* Log listening status */
    guacd_log_info("Listening on host %s, port %s", bound_address, bound_port);

    /* Free addresses */
    freeaddrinfo(addresses);

    /* Daemon loop */
    for (;;) {

        pid_t child_pid;

        /* Listen for connections */
        if (listen(socket_fd, 5) < 0) {
            guacd_log_error("Could not listen on socket: %s", strerror(errno));
            return 3;
        }

        /* Accept connection */
        client_addr_len = sizeof(client_addr);
        connected_socket_fd = accept(socket_fd,
                (struct sockaddr*) &client_addr, &client_addr_len);

        if (connected_socket_fd < 0) {
            guacd_log_error("Could not accept client connection: %s",
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
            guacd_log_error("Error forking child process: %s", strerror(errno));

        /* If child, start client, and exit when finished */
        else if (child_pid == 0) {

            guac_socket* socket;

#ifdef ENABLE_SSL

            /* If SSL chosen, use it */
            if (ssl_context != NULL) {
                socket = guac_socket_open_secure(ssl_context, connected_socket_fd);
                if (socket == NULL) {
                    guacd_log_guac_error("Error opening secure connection");
                    return 0;
                }
            }
            else
                socket = guac_socket_open(connected_socket_fd);
#else
            /* Open guac_socket */
            socket = guac_socket_open(connected_socket_fd);
#endif

            guacd_handle_connection(socket);
            close(connected_socket_fd);
            return 0;
        }

        /* If parent, close reference to child's descriptor */
        else if (close(connected_socket_fd) < 0) {
            guacd_log_error("Error closing daemon reference to "
                    "child descriptor: %s", strerror(errno));
        }

    }

    /* Close socket */
    if (close(socket_fd) < 0) {
        guacd_log_error("Could not close socket: %s", strerror(errno));
        return 3;
    }

    return 0;

}

