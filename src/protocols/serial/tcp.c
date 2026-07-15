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

#include "settings.h"
#include "stream.h"
#include "tcp.h"

#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/tcp.h>

#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>

int guac_serial_tcp_open(guac_serial_stream* stream, guac_client* client,
        guac_serial_settings* settings) {

    /* Connect to the remote serial server */
    int fd = guac_tcp_connect(settings->hostname, settings->port,
            settings->timeout);
    if (fd < 0) {

        int err = errno;

        /* Distinguish an actively-refused or reset endpoint from other
         * failures (e.g. name resolution, timeout) */
        if (err == ECONNREFUSED) {
            guac_client_log(client, GUAC_LOG_ERROR, "ser2net endpoint "
                    "\"%s:%s\" refused the connection: %s", settings->hostname,
                    settings->port, strerror(err));
            stream->open_status = GUAC_PROTOCOL_STATUS_UPSTREAM_UNAVAILABLE;
        }
        else if (err == ECONNRESET) {
            guac_client_log(client, GUAC_LOG_ERROR, "Connection to ser2net "
                    "endpoint \"%s:%s\" was reset: %s", settings->hostname,
                    settings->port, strerror(err));
            stream->open_status = GUAC_PROTOCOL_STATUS_UPSTREAM_UNAVAILABLE;
        }
        else {
            guac_client_log(client, GUAC_LOG_ERROR, "Unable to connect to "
                    "serial server \"%s\" port \"%s\".", settings->hostname,
                    settings->port);
            stream->open_status = GUAC_PROTOCOL_STATUS_UPSTREAM_NOT_FOUND;
        }

        return -1;
    }

    /* Disable Nagle's algorithm so keystrokes are sent without delay */
    int flag = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

    stream->backend = GUAC_SERIAL_BACKEND_TCP;
    stream->fd = fd;
    return 0;

}
