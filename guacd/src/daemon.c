
/*
 *  Guacamole - Clientless Remote Desktop
 *  Copyright (C) 2010  Michael Jumper
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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

#ifdef __HAVE_PTHREAD_H__
#include <pthread.h>
#endif

#include <errno.h>

#include <guacamole/client.h>
#include <guacamole/guaclog.h>

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
    if (close(thread_data->fd) < 0) {
        GUAC_LOG_ERROR("Error closing connection: %s", strerror(errno));
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

    /* Daemon Process */
    pid_t daemon_pid;

    /* Parse arguments */
    while ((opt = getopt(argc, argv, "l:")) != -1) {
        if (opt == 'l') {
            listen_port = atoi(optarg);
            if (listen_port <= 0) {
                fprintf(stderr, "Invalid port: %s\n", optarg);
                exit(EXIT_FAILURE);
            }
        }
        else {
            fprintf(stderr, "USAGE: %s [-l LISTENPORT]\n", argv[0]);
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

    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt_on, sizeof(opt_on))) {
        fprintf(stderr, "Warning: Unable to set socket options for reuse: %s\n", strerror(errno));
    }

    /* Bind socket to address */
    if (bind(socket_fd, (struct sockaddr*) &server_addr,
                sizeof(server_addr)) < 0) {
        fprintf(stderr, "Error binding socket: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    } 

    /* Fork into background */
#ifdef fork
    daemon_pid = fork();

    /* If error, fail */
    if (daemon_pid == -1) {
        fprintf(stderr, "Error forking daemon process: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* If parent, exit */
    else if (daemon_pid != 0) {
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

#ifdef pthread_t
        pthread_t thread;
#endif
        client_thread_data* data;

        /* Listen for connections */
        if (listen(socket_fd, 5) < 0) {
            GUAC_LOG_ERROR("Could not listen on socket: %s", strerror(errno));
            return 3;
        }

        /* Accept connection */
        client_addr_len = sizeof(client_addr);
        connected_socket_fd = accept(socket_fd, (struct sockaddr*) &client_addr, &client_addr_len);
        if (connected_socket_fd < 0) {
            GUAC_LOG_ERROR("Could not accept client connection: %s", strerror(errno));
            return 3;
        }

        data = malloc(sizeof(client_thread_data));
        data->fd = connected_socket_fd;

#ifdef pthread_t
        if (pthread_create(&thread, NULL, start_client_thread, (void*) data)) {
            GUAC_LOG_ERROR("Could not create client thread: %s", strerror(errno));
            return 3;
        }
#else
        GUAC_LOG_INFO("POSIX threads support not present at compile time.");
        GUAC_LOG_INFO("guacd handling one connection at a time.");
        start_client_thread(data);
#endif

    }

    /* Close socket */
    if (close(socket_fd) < 0) {
        GUAC_LOG_ERROR("Could not close socket: %s", strerror(errno));
        return 3;
    }

    return 0;

}

