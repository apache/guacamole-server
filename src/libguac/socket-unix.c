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
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

int guac_socket_unix_connect(const char* path) {

    /* Attempt to open a socket. */
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        guac_error = GUAC_STATUS_INVALID_ARGUMENT;
        guac_error_message = "Unable to acquire a UNIX socket.";
        close(fd);
        return -1;
    }

    /* Set up the socket with the path. */
    struct sockaddr_un socket_addr = {
        .sun_family = AF_UNIX
    };
    strncpy(socket_addr.sun_path, path, sizeof(socket_addr.sun_path) - 1);

    /* Attempt to connect via the open socket. */
    if (connect(fd, (const struct sockaddr *) &socket_addr, sizeof(struct sockaddr_un))) {
        guac_error = GUAC_STATUS_INVALID_ARGUMENT;
        guac_error_message = "Unable to connect to the UNIX socket.";
        close(fd);
        return -1;
    }

    /* Return the open socket. */
    return fd;

}