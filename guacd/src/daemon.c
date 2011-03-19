
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
#include <sys/types.h>

#ifdef __MINGW32__
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#ifdef HAVE_LIBPTHREAD
#include <pthread.h>
#elif defined(__MINGW32__)
#include <windows.h>
#include <process.h>
#endif

#include <errno.h>

#include <guacamole/client.h>
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
    snprintf(error, sizeof(error)-1, "ERROR #%i", GetLastError());
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

    GUAC_LOG_INFO("Spawning client");

    /* Load and start client */
    client = guac_get_client(thread_data->fd); 

    if (client == NULL) {
        GUAC_LOG_ERROR("Client retrieval failed");
        return NULL;
    }

    guac_start_client(client);

    guac_free_client(client);

    /* Close socket */
    if (CLOSE_SOCKET(thread_data->fd) < 0) {
        GUAC_LOG_ERROR("Error closing connection: %s", lasterror());
        free(data);
        return NULL;
    }

    GUAC_LOG_INFO("Client finished");
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
    unsigned int client_addr_len;
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

    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt_on, sizeof(opt_on))) {
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
    GUAC_LOG_INFO("fork() not defined at compile time.");
    GUAC_LOG_INFO("guacd running in foreground only.");
#endif

    /* Otherwise, this is the daemon */
    GUAC_LOG_INFO("Started, listening on port %i", listen_port);

    /* Daemon loop */
    for (;;) {

#ifdef HAVE_LIBPTHREAD
        pthread_t thread;
#endif
        client_thread_data* data;

        /* Listen for connections */
        if (listen(socket_fd, 5) < 0) {
            GUAC_LOG_ERROR("Could not listen on socket: %s", lasterror());
            return 3;
        }

        /* Accept connection */
        client_addr_len = sizeof(client_addr);
        connected_socket_fd = accept(socket_fd, (struct sockaddr*) &client_addr, &client_addr_len);
        if (connected_socket_fd < 0) {
            GUAC_LOG_ERROR("Could not accept client connection: %s", lasterror());
            return 3;
        }

        data = malloc(sizeof(client_thread_data));
        data->fd = connected_socket_fd;

#ifdef HAVE_LIBPTHREAD
        if (pthread_create(&thread, NULL, start_client_thread, (void*) data)) {
            GUAC_LOG_ERROR("Could not create client thread: %s", lasterror());
            return 3;
        }

        pthread_detach(thread);
#elif __MINGW32__
        if (_beginthread(start_client_thread, 0, (void*) data) == -1L) { 
            GUAC_LOG_ERROR("Could not create client thread: %s", lasterror());
            return 3;
        }
#else
#warning THREAD SUPPORT NOT FOUND! guacd will only be able to handle one connection at a time.
        GUAC_LOG_INFO("Thread support not present at compile time.");
        GUAC_LOG_INFO("guacd handling one connection at a time.");
        start_client_thread(data);
#endif

    }

    /* Close socket */
    if (CLOSE_SOCKET(socket_fd) < 0) {
        GUAC_LOG_ERROR("Could not close socket: %s", lasterror());
        return 3;
    }

    return 0;

}

