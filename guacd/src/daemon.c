
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

#include <sys/socket.h>
#include <netinet/in.h>

#include <errno.h>
#include <syslog.h>

#include <guacamole/client.h>
#include <guacamole/error.h>

#include "client.h"

typedef struct client_thread_data {

    int fd;

} client_thread_data;


void* start_client_thread(void* data) {

    guac_client* client;
    guac_client_plugin* plugin;
    client_thread_data* thread_data = (client_thread_data*) data;
    guac_socket* socket;
    guac_instruction* select;
    guac_instruction* connect;

    /* Open guac_socket */
    socket = guac_socket_open(thread_data->fd);

    /* Get protocol from select instruction */
    select = guac_protocol_expect_instruction(
            socket, GUAC_USEC_TIMEOUT, "select");
    if (select == NULL) {

        /* Log error */
        syslog(LOG_ERR, "Error reading \"select\": %s",
                guac_status_string(guac_error));

        /* Free resources */
        guac_socket_close(socket);
        free(data);
        return NULL;
    }

    /* Validate args to select */
    if (select->argc != 1) {

        /* Log error */
        syslog(LOG_ERR, "Bad number of arguments to \"select\" (%i)",
                select->argc);

        /* Free resources */
        guac_socket_close(socket);
        free(data);
        return NULL;
    }

    syslog(LOG_INFO, "Protocol \"%s\" selected", select->argv[0]);

    /* Get plugin from protocol in select */
    plugin = guac_client_plugin_open(select->argv[0]);
    guac_instruction_free(select);

    if (plugin == NULL) {

        /* Log error */
        syslog(LOG_ERR, "Error loading client plugin: %s",
                guac_status_string(guac_error));

        /* Free resources */
        guac_socket_close(socket);
        free(data);
        return NULL;
    }

    /* Send args response */
    if (guac_protocol_send_args(socket, plugin->args)
            || guac_socket_flush(socket)) {

        /* Log error */
        syslog(LOG_ERR, "Error sending \"args\": %s",
                guac_status_string(guac_error));

        if (guac_client_plugin_close(plugin))
            syslog(LOG_ERR, "Error closing client plugin");

        guac_socket_close(socket);
        free(data);
        return NULL;
    }

    /* Get args from connect instruction */
    connect = guac_protocol_expect_instruction(
            socket, GUAC_USEC_TIMEOUT, "connect");
    if (connect == NULL) {

        /* Log error */
        syslog(LOG_ERR, "Error reading \"connect\": %s",
                guac_status_string(guac_error));

        if (guac_client_plugin_close(plugin))
            syslog(LOG_ERR, "Error closing client plugin");

        guac_socket_close(socket);
        free(data);
        return NULL;
    }

    /* Load and init client */
    client = guac_client_plugin_get_client(plugin, socket,
            connect->argc, connect->argv); 
    guac_instruction_free(select);

    if (client == NULL) {

        syslog(LOG_ERR, "Error instantiating client: %s",
                guac_status_string(guac_error));

        if (guac_client_plugin_close(plugin))
            syslog(LOG_ERR, "Error closing client plugin");

        guac_socket_close(socket);
        free(data);
        return NULL;
    }

    /* Start client threads */
    syslog(LOG_INFO, "Starting client");
    guac_start_client(client);

    /* Clean up */
    syslog(LOG_INFO, "Client finished");
    guac_client_free(client);
    if (guac_client_plugin_close(plugin))
        syslog(LOG_ERR, "Error closing client plugin");

    /* Close socket */
    guac_socket_close(socket);
    close(thread_data->fd);

    free(data);
    return NULL;

}

int main(int argc, char* argv[]) {

    /* Server */
    int socket_fd;
    struct sockaddr_in server_addr;
    int opt_on = 1;

    /* Client */
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    int connected_socket_fd;

    /* Arguments */
    int listen_port = 4822; /* Default port */
    int opt;

    char* pidfile = NULL;

    /* Daemon Process */
    pid_t daemon_pid;

    /* Parse arguments */
    while ((opt = getopt(argc, argv, "l:p:")) != -1) {
        if (opt == 'l') {
            listen_port = atoi(optarg);
            if (listen_port <= 0) {
                fprintf(stderr, "Invalid port: %s\n", optarg);
                exit(EXIT_FAILURE);
            }
        }
        else if (opt == 'p') {
            pidfile = strdup(optarg);
        }
        else {
            fprintf(stderr, "USAGE: %s [-l LISTENPORT] [-p PIDFILE]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    /* Get binding address */
    memset(&server_addr, 0, sizeof(server_addr)); /* Zero struct */
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(listen_port);

    /* Get socket */
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        fprintf(stderr, "Error opening socket: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (void*) &opt_on, sizeof(opt_on))) {
        fprintf(stderr, "Warning: Unable to set socket options for reuse: %s\n", strerror(errno));
    }

    /* Bind socket to address */
    if (bind(socket_fd, (struct sockaddr*) &server_addr,
                sizeof(server_addr)) < 0) {
        fprintf(stderr, "Error binding socket: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    } 

    /* Fork into background */
#ifdef HAVE_FORK
    daemon_pid = fork();

    /* If error, fail */
    if (daemon_pid == -1) {
        fprintf(stderr, "Error forking daemon process: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* If parent, write PID file and exit */
    else if (daemon_pid != 0) {

        if (pidfile != NULL) {

            /* Attempt to open pidfile and write PID */
            FILE* pidf = fopen(pidfile, "w");
            if (pidf) {
                fprintf(pidf, "%d\n", daemon_pid);
                fclose(pidf);
            }

            /* Warn on failure */
            else {
                fprintf(stderr, "WARNING: Could not write PID file: %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }

        }

        exit(EXIT_SUCCESS);
    }
#else
    daemon_pid = getpid();
    syslog(LOG_INFO, "fork() not defined at compile time.");
    syslog(LOG_INFO, "guacd running in foreground only.");
#endif

    /* Otherwise, this is the daemon */
    syslog(LOG_INFO, "Started, listening on port %i", listen_port);

    /* Ignore SIGPIPE */
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        syslog(LOG_ERR, "Could not set handler for SIGPIPE to ignore. SIGPIPE may cause termination of the daemon.");
    }

    /* Ignore SIGCHLD (force automatic removal of children) */
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
        syslog(LOG_ERR, "Could not set handler for SIGCHLD to ignore. Child processes may pile up in the process table.");
    }

    /* Daemon loop */
    for (;;) {

#ifdef HAVE_FORK
        pid_t child_pid;
#else
        guac_thread_t thread;
#endif
        client_thread_data* data;

        /* Listen for connections */
        if (listen(socket_fd, 5) < 0) {
            syslog(LOG_ERR, "Could not listen on socket: %s", strerror(errno));
            return 3;
        }

        /* Accept connection */
        client_addr_len = sizeof(client_addr);
        connected_socket_fd = accept(socket_fd, (struct sockaddr*) &client_addr, &client_addr_len);
        if (connected_socket_fd < 0) {
            syslog(LOG_ERR, "Could not accept client connection: %s", strerror(errno));
            return 3;
        }

        data = malloc(sizeof(client_thread_data));
        data->fd = connected_socket_fd;

        /* 
         * Once connection is accepted, send child into background, whether through
         * fork() or through creating a thread. If thead support is not present on
         * the platform, guacd will still work, but will only be able to handle one
         * connection at a time.
         */

#ifdef HAVE_FORK

        /*** FORK ***/

        /*
         * Note that we prefer fork() over threads for connection-handling
         * processes as they give each connection its own memory area, and
         * isolate the main daemon and other connections from errors in any
         * particular client plugin.
         */

        child_pid = fork();

        /* If error, log */
        if (child_pid == -1)
            syslog(LOG_ERR, "Error forking child process: %s\n", strerror(errno));

        /* If child, start client, and exit when finished */
        else if (child_pid == 0) {
            start_client_thread(data);
            return 0;
        }

        /* If parent, close reference to child's descriptor */
        else if (close(connected_socket_fd) < 0) {
            syslog(LOG_ERR, "Error closing daemon reference to child descriptor: %s", strerror(errno));
        }

#else

        if (guac_thread_create(&thread, start_client_thread, (void*) data))
            syslog(LOG_ERR, "Could not create client thread: %s", strerror(errno));

#endif

    }

    /* Close socket */
    if (close(socket_fd) < 0) {
        syslog(LOG_ERR, "Could not close socket: %s", strerror(errno));
        return 3;
    }

    return 0;

}

