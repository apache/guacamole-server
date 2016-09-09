
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
#include "guac_client.h"
#include "guac_display.h"
#include "guac_input.h"
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
#include <guacamole/instruction.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>

#include "log.h"

/**
 * All arguments for the client (currently empty).
 */
static const char* GUAC_DRV_CLIENT_ARGS[] = { NULL };

guac_client* guac_drv_handle_connection(guac_socket* socket) {

    guac_client* client;
    guac_drv_client_data* client_data;
    guac_instruction* select;
    guac_instruction* size;
    guac_instruction* audio;
    guac_instruction* video;
    guac_instruction* connect;

    /* Get protocol from select instruction */
    select = guac_instruction_expect(
            socket, GUACD_USEC_TIMEOUT, "select");
    if (select == NULL) {

        /* Log error */
        guac_drv_log_guac_error(GUAC_LOG_ERROR, "Error reading \"select\"");

        /* Free resources */
        guac_socket_free(socket);
        return NULL;
    }

    /* Validate args to select */
    if (select->argc != 1) {

        /* Log error */
        guac_drv_log(GUAC_LOG_ERROR, "Bad number of arguments "
                "to \"select\" (%i)", select->argc);

        /* Free resources */
        guac_socket_free(socket);
        return NULL;
    }

    guac_drv_log(GUAC_LOG_INFO, "Protocol \"%s\" selected", select->argv[0]);
    if (strcmp(select->argv[0], "x11") != 0) {

        /* Log error */
        guac_drv_log_guac_error(GUAC_LOG_ERROR,
                "Requested protocol must be \"x11\".");

        /* Free resources */
        guac_socket_free(socket);
        return NULL;
    }

    guac_instruction_free(select);

    /* Send args response */
    if (guac_protocol_send_args(socket, GUAC_DRV_CLIENT_ARGS)
            || guac_socket_flush(socket)) {

        /* Log error */
        guac_drv_log_handshake_failure();
        guac_drv_log_guac_error(GUAC_LOG_DEBUG, "Error sending \"args\" to new user");

        guac_socket_free(socket);
        return NULL;
    }

    /* Get optimal screen size */
    size = guac_instruction_expect(socket, GUACD_USEC_TIMEOUT, "size");
    if (size == NULL) {

        /* Log error */
        guac_drv_log_handshake_failure();
        guac_drv_log_guac_error(GUAC_LOG_DEBUG, "Error reading \"size\"");

        /* Free resources */
        guac_socket_free(socket);
        return NULL;
    }

    /* Get supported audio formats */
    audio = guac_instruction_expect(socket, GUACD_USEC_TIMEOUT, "audio");
    if (audio == NULL) {

        /* Log error */
        guac_drv_log_handshake_failure();
        guac_drv_log_guac_error(GUAC_LOG_DEBUG, "Error reading \"audio\"");

        /* Free resources */
        guac_socket_free(socket);
        return NULL;
    }

    /* Get supported video formats */
    video = guac_instruction_expect(socket, GUACD_USEC_TIMEOUT, "video");
    if (video == NULL) {

        /* Log error */
        guac_drv_log_handshake_failure();
        guac_drv_log_guac_error(GUAC_LOG_DEBUG, "Error reading \"video\"");

        /* Free resources */
        guac_socket_free(socket);
        return NULL;
    }

    /* Get args from connect instruction */
    connect = guac_instruction_expect(socket, GUACD_USEC_TIMEOUT, "connect");
    if (connect == NULL) {

        /* Log error */
        guac_drv_log_handshake_failure();
        guac_drv_log_guac_error(GUAC_LOG_DEBUG, "Error reading \"connect\"");

        /* Free resources */
        guac_socket_free(socket);
        return NULL;

    }

    /* Get client */
    client = guac_client_alloc();
    client->socket = socket;
    client->log_handler = guac_drv_client_log;

    /* Parse optimal screen dimensions from size instruction */
    client->info.optimal_width  = atoi(size->argv[0]);
    client->info.optimal_height = atoi(size->argv[1]);

    /* Init data */
    client_data = client->data = malloc(sizeof(guac_drv_client_data));
    client_data->button_mask = 0;

    /* Start event thread */
    if (pthread_create(&(client_data->input_thread), NULL,
            guac_drv_client_input_thread, client)) {
        guac_drv_log(GUAC_LOG_ERROR, "Unable to start client event thread.");
        return NULL;
    }

    /* Free parsed instructions */
    guac_instruction_free(connect);
    guac_instruction_free(audio);
    guac_instruction_free(video);
    guac_instruction_free(size);

    return client;

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

    /* Arguments */
    char* listen_address = NULL; /* Default address of INADDR_ANY */
    char* listen_port = "4822";  /* Default port */

    /* General */
    int retval;

    /* Log start */
    guac_drv_log(GUAC_LOG_INFO, "Guacamole video driver daemon "
            "version " VERSION);

    /* Get addresses for binding */
    if ((retval = getaddrinfo(listen_address, listen_port,
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

        guac_socket* socket;
        guac_client* client;

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

        /* Open guac_socket */
        socket = guac_socket_open(connected_socket_fd);

        /* Init client */
        client = guac_drv_handle_connection(socket);
        client->mouse_handler = guac_drv_client_mouse_handler;
        client->free_handler  = guac_drv_client_free_handler;

        /* Insert client into list */
        guac_drv_display_add_client(display, client);

    }

    /* Close socket */
    if (close(socket_fd) < 0) {
        guac_drv_log(GUAC_LOG_ERROR, "Could not close socket: %s",
                strerror(errno));
        return NULL;
    }

    return NULL;

}

