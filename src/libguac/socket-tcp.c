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
#include "guacamole/error.h"
#include "guacamole/socket.h"

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

int guac_socket_tcp_connect(const char* hostname, const char* port) {

    int retval;

    int fd = EBADFD;
    struct addrinfo* addresses;
    struct addrinfo* current_address;

    char connected_address[1024];
    char connected_port[64];

    struct addrinfo hints = {
        .ai_family   = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP
    };

    /* Get addresses for requested hostname and port. */
    if ((retval = getaddrinfo(hostname, port, &hints, &addresses))) {
        guac_error = GUAC_STATUS_INVALID_ARGUMENT;
        guac_error_message = "Error parsing address or port.";
        return retval;
    }

    /* Attempt connection to each address until success */
    current_address = addresses;
    while (current_address != NULL) {

        /* Resolve hostname */
        if ((retval = getnameinfo(current_address->ai_addr,
                current_address->ai_addrlen,
                connected_address, sizeof(connected_address),
                connected_port, sizeof(connected_port),
                NI_NUMERICHOST | NI_NUMERICSERV))) {

            guac_error = GUAC_STATUS_INVALID_ARGUMENT;
            guac_error_message = "Error resolving host.";
            continue;
        }

        /* Get socket */
        fd = socket(current_address->ai_family, SOCK_STREAM, 0);
        if (fd < 0) {
            freeaddrinfo(addresses);
            return fd;
        }

        /* Connect */
        if (connect(fd, current_address->ai_addr,
                        current_address->ai_addrlen) == 0) {

            /* Done if successful connect */
            break;

        }

        close(fd);
        current_address = current_address->ai_next;

    }

    /* Free addrinfo */
    freeaddrinfo(addresses);

    /* If unable to connect to anything, set error status. */
    if (current_address == NULL) {
        guac_error = GUAC_STATUS_REFUSED;
        guac_error_message = "Unable to connect to remote host.";
    }

    /* Return the fd, or the error message if the socket connection failed. */
    return fd;

}