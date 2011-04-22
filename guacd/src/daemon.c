
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

#ifdef __MINGW32__
#include <winsock2.h>
typedef int socklen_t;
typedef char* sockopt_p;
#else
#include <sys/socket.h>
#include <netinet/in.h>
typedef void* sockopt_p;
#endif

#include <errno.h>

#include <guacamole/client.h>
#include <guacamole/thread.h>
#include <guacamole/log.h>

/* Windows / MINGW32 handles closing sockets differently */ 
#ifdef __MINGW32__
#define CLOSE_SOCKET(socket) closesocket(socket)
#else
#define CLOSE_SOCKET(socket) close(socket)
#endif


/* Cross-platform strerror()/errno clone */
char error[65536];
char* lasterror() {
#ifdef __MINGW32__
    snprintf(error, sizeof(error)-1, "ERROR #%li", GetLastError());
    return error;
#else
    return strerror(errno);
#endif
}

typedef struct client_thread_data {

    int fd;

} client_thread_data;


void* start_client_thread(void* data) {

    guac_client* client;
    client_thread_data* thread_data = (client_thread_data*) data;

    guac_log_info("Spawning client");

    /* Load and start client */
    client = guac_get_client(thread_data->fd); 

    if (client == NULL) {
        guac_log_error("Client retrieval failed");
        free(data);
        return NULL;
    }

    guac_start_client(client);
    guac_free_client(client);

    /* Close socket */
    if (CLOSE_SOCKET(thread_data->fd) < 0) {
        guac_log_error("Error closing connection: %s", lasterror());
        free(data);
        return NULL;
    }

    guac_log_info("Client finished");
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

#ifdef __MINGW32__
    /* Structure for holding winsock version info */
    WSADATA wsadata;
#endif

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

#ifdef __MINGW32__
    /* If compiling for Windows, init winsock. */
    if (WSAStartup(MAKEWORD(1,1), &wsadata) == SOCKET_ERROR) {
        fprintf(stderr, "Error creating socket.");
        exit(EXIT_FAILURE);
    }
#endif

    /* Get socket */
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        fprintf(stderr, "Error opening socket: %s\n", lasterror());
        exit(EXIT_FAILURE);
    }

    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, (sockopt_p) &opt_on, sizeof(opt_on))) {
        fprintf(stderr, "Warning: Unable to set socket options for reuse: %s\n", lasterror());
    }

    /* Bind socket to address */
    if (bind(socket_fd, (struct sockaddr*) &server_addr,
                sizeof(server_addr)) < 0) {
        fprintf(stderr, "Error binding socket: %s\n", lasterror());
        exit(EXIT_FAILURE);
    } 

    /* Fork into background */
#ifdef HAVE_FORK
    daemon_pid = fork();

    /* If error, fail */
    if (daemon_pid == -1) {
        fprintf(stderr, "Error forking daemon process: %s\n", lasterror());
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
                fprintf(stderr, "WARNING: Could not write PID file: %s\n", lasterror());
                exit(EXIT_FAILURE);
            }

        }

        exit(EXIT_SUCCESS);
    }
#else
    daemon_pid = getpid();
    guac_log_info("fork() not defined at compile time.");
    guac_log_info("guacd running in foreground only.");
#endif

    /* Otherwise, this is the daemon */
    guac_log_info("Started, listening on port %i", listen_port);

#ifndef __MINGW32__
    /* Ignore SIGPIPE */
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        guac_log_error("Could not set handler for SIGPIPE to ignore. SIGPIPE may cause termination of the daemon.");
    }

    /* Ignore SIGCHLD (force automatic removal of children) */
    if (signal(SIGCHLD, SIG_IGN) == SIG_ERR) {
        guac_log_error("Could not set handler for SIGCHLD to ignore. Child processes may pile up in the process table.");
    }
#endif

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
            guac_log_error("Could not listen on socket: %s", lasterror());
            return 3;
        }

        /* Accept connection */
        client_addr_len = sizeof(client_addr);
        connected_socket_fd = accept(socket_fd, (struct sockaddr*) &client_addr, &client_addr_len);
        if (connected_socket_fd < 0) {
            guac_log_error("Could not accept client connection: %s", lasterror());
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
            guac_log_error("Error forking child process: %s\n", lasterror());

        /* If child, start client, and exit when finished */
        else if (child_pid == 0) {
            start_client_thread(data);
            return 0;
        }

#else

        if (guac_thread_create(&thread, start_client_thread, (void*) data))
            guac_log_error("Could not create client thread: %s", lasterror());

#endif

    }

    /* Close socket */
    if (CLOSE_SOCKET(socket_fd) < 0) {
        guac_log_error("Could not close socket: %s", lasterror());
        return 3;
    }

    return 0;

}

