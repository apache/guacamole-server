
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
#include "daemon.h"
#include "display.h"
#include "input.h"
#include "list.h"

#include <xorg-server.h>
#include <xf86.h>
#include <xf86str.h>

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

#include <guacamole/client.h>
#include <guacamole/error.h>
#include <guacamole/parser.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/user.h>

#include "log.h"

/**
 * Parameters used by the connection thread created for each new user.
 */
typedef struct guac_drv_connection_thread_params {

    /**
     * The guac_client representing the connection being joined by the new
     * user.
     */
    guac_client* client;

    /**
     * The file descriptor of the socket of the inbound connection of the
     * joining user.
     */
    int fd;

} guac_drv_connection_thread_params;

/**
 * Connection thread which is created for each user joining the current X11
 * session. The thread takes care of the entire Guacamole protocol handshake
 * (except for the initial "select").
 *
 * @param data
 *     An instance of guac_drv_connection_thread_params describing the
 *     guac_client being joined, and the file descriptor of the new user's
 *     connection.
 *
 * @return
 *     Always NULL.
 */
static void* guac_drv_connection_thread(void* data) {

    guac_drv_connection_thread_params* params =
        (guac_drv_connection_thread_params*) data;

    guac_client* client = params->client;
    int fd = params->fd;

    /* Open guac_socket */
    guac_socket* socket = guac_socket_open(fd);
    if (socket == NULL)
        return NULL;

    /* Create parser for new connection */
    guac_parser* parser = guac_parser_alloc();

    /* Reset guac_error */
    guac_error = GUAC_STATUS_SUCCESS;
    guac_error_message = NULL;

    /* Get protocol from select instruction */
    if (guac_parser_expect(parser, socket, GUACD_USEC_TIMEOUT, "select")) {

        /* Log error */
        guac_drv_log_handshake_failure();
        guac_drv_log_guac_error(GUAC_LOG_DEBUG, "Error reading \"select\"");

        goto handshake_failed;
    }

    /* Validate args to select */
    if (parser->argc != 1) {

        /* Log error */
        guac_drv_log_handshake_failure();
        guac_drv_log(GUAC_LOG_ERROR, "Bad number of arguments "
                "to \"select\" (%i)", parser->argc);

        goto handshake_failed;
    }

    const char* identifier = parser->argv[0];

    /* Accept connections for this driver only */
    if (strcmp(identifier, "xorg") == 0)
        guac_drv_log(GUAC_LOG_INFO, "X.Org video driver selected", identifier);

    /* Allow the overall connection to be joined (there is only one) */
    else if (strcmp(identifier, client->connection_id) == 0)
        guac_drv_log(GUAC_LOG_INFO, "Connection \"%s\" selected", identifier);

    /* Fail all other connection attempts */
    else {
        guac_drv_log(GUAC_LOG_ERROR, "Unknown protocol or "
                "connection ID: \"%s\".", identifier);
        goto handshake_failed;
    }

    /* Init user */
    guac_user* user = guac_user_alloc();
    user->client = client;
    user->socket = socket;

    /* Handle entire user connection, free user once complete */
    guac_user_handle_connection(user, GUACD_USEC_TIMEOUT);
    guac_user_free(user);

    /* Fall through to free remaining resources */

handshake_failed:
    guac_parser_free(parser);
    guac_socket_free(socket);

    free(data);
    return NULL;

}

void* guac_drv_listen_thread(void* arg) {

    /* Get guac_drv_display */
    guac_drv_display* display = (guac_drv_display*) arg;

    /* Server */
    int socket_fd;
    struct addrinfo* addresses;
    struct addrinfo* current_address;
    char bound_address[1024];
    char bound_port[64];
    int opt_on = 1;

    struct addrinfo hints = {
        .ai_flags    = AI_PASSIVE,
        .ai_family   = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP
    };

    /* Client */
    struct sockaddr_in client_addr;
    socklen_t client_addr_len;
    int connected_socket_fd;

    /* General */
    int retval;

    /* Log start */
    guac_drv_log(GUAC_LOG_INFO, "Guacamole video driver daemon "
            "version " VERSION);

    /* Get addresses for binding */
    if ((retval = getaddrinfo(display->listen_address, display->listen_port,
                    &hints, &addresses))) {

        guac_drv_log(GUAC_LOG_ERROR, "Error parsing given address or port: %s",
                gai_strerror(retval));
        return NULL;

    }

    /* Get socket */
    socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_fd < 0) {
        guac_drv_log(GUAC_LOG_ERROR, "Error opening socket: %s",
                strerror(errno));
        return NULL;
    }

    /* Allow socket reuse */
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR,
                (void*) &opt_on, sizeof(opt_on))) {
        guac_drv_log(GUAC_LOG_INFO, "Unable to set socket options for "
                "reuse: %s", strerror(errno));
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
            guac_drv_log(GUAC_LOG_ERROR, "Unable to resolve host: %s",
                    gai_strerror(retval));

        /* Attempt to bind socket to address */
        if (bind(socket_fd,
                    current_address->ai_addr,
                    current_address->ai_addrlen) == 0) {

            guac_drv_log(GUAC_LOG_INFO, "Successfully bound socket to "
                    "host %s, port %s", bound_address, bound_port);

            /* Done if successful bind */
            break;

        }

        /* Otherwise log information regarding bind failure */
        else
            guac_drv_log(GUAC_LOG_INFO, "Unable to bind socket to "
                    "host %s, port %s: %s",
                    bound_address, bound_port, strerror(errno));

        current_address = current_address->ai_next;

    }

    /* If unable to bind to anything, fail */
    if (current_address == NULL) {
        guac_drv_log(GUAC_LOG_ERROR, "Unable to bind socket to any addresses.");
        return NULL;
    }

    /* Log listening status */
    guac_drv_log(GUAC_LOG_INFO, "Listening on host %s, port %s",
            bound_address, bound_port);

    /* Free addresses */
    freeaddrinfo(addresses);

    /* Daemon loop */
    for (;;) {

        /* Listen for connections */
        if (listen(socket_fd, 5) < 0) {
            guac_drv_log(GUAC_LOG_ERROR, "Could not listen on socket: %s",
                    strerror(errno));
            return NULL;
        }

        /* Accept connection */
        client_addr_len = sizeof(client_addr);
        connected_socket_fd = accept(socket_fd,
                (struct sockaddr*) &client_addr, &client_addr_len);

        if (connected_socket_fd < 0) {

            /* Try again if interrupted */
            if (errno == EINTR)
                continue;

            guac_drv_log(GUAC_LOG_ERROR, "Could not accept client "
                    "connection: %s", strerror(errno));
            return NULL;
        }

        /* Handle Guacamole protocol over new connection */
        guac_drv_connection_thread_params* params =
            malloc(sizeof(guac_drv_connection_thread_params));

        params->client = display->client;
        params->fd = connected_socket_fd;

        /* Start connection thread */
        pthread_t connection_thread;
        if (!pthread_create(&connection_thread, NULL,
                    guac_drv_connection_thread, params))
            pthread_detach(connection_thread);

        /* Log thread creation failures */
        else {
            guac_drv_log(GUAC_LOG_ERROR,
                    "Could not start connection thread: %s", strerror(errno));
            free(params);
        }

    }

    /* Close socket */
    if (close(socket_fd) < 0) {
        guac_drv_log(GUAC_LOG_ERROR, "Could not close socket: %s",
                strerror(errno));
        return NULL;
    }

    return NULL;

}

