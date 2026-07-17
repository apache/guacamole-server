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
#include <guacamole/error.h>
#include <guacamole/protocol.h>
#include <guacamole/tcp.h>

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/**
 * Binds and listens on the address/port described by the given settings and
 * accepts a single inbound connection from the device (reverse mode). The wait
 * for the inbound connection honors both the configured listen timeout and
 * client shutdown. On success the accepted socket is returned; TCP_NODELAY is
 * left to the caller (guac_serial_net_open_fd).
 *
 * @param stream
 *     The serial stream whose open_status is set on failure.
 *
 * @param client
 *     The client for which the connection is being opened.
 *
 * @param settings
 *     The parsed connection settings describing the address/port to listen on.
 *
 * @return
 *     The accepted file descriptor on success, or -1 on failure with
 *     stream->open_status set and the reason logged.
 */
static int guac_serial_reverse_accept(guac_serial_stream* stream,
        guac_client* client, guac_serial_settings* settings) {

    /* Default to loopback if no bind address was configured */
    const char* bind_addr = (settings->bind_address && settings->bind_address[0])
            ? settings->bind_address : "127.0.0.1";

    /* Resolve the local address to bind and listen on */
    struct addrinfo hints = {
        .ai_family   = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_flags    = AI_PASSIVE
    };

    struct addrinfo* addrs;
    int err = getaddrinfo(bind_addr, settings->port, &hints, &addrs);
    if (err != 0) {
        guac_client_log(client, GUAC_LOG_ERROR, "Unable to resolve bind "
                "address \"%s\" for reverse serial connection: %s", bind_addr,
                gai_strerror(err));
        stream->open_status = GUAC_PROTOCOL_STATUS_SERVER_ERROR;
        return -1;
    }

    /* Bind to the first address for which a socket can be created and bound */
    int listen_fd = -1;
    for (struct addrinfo* addr = addrs; addr != NULL; addr = addr->ai_next) {

        listen_fd = socket(addr->ai_family,
                addr->ai_socktype | SOCK_CLOEXEC, addr->ai_protocol);
        if (listen_fd < 0)
            continue;

        /* Allow re-listening after a reconnect despite lingering TIME_WAIT */
        int reuse = 1;
        setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        if (bind(listen_fd, addr->ai_addr, addr->ai_addrlen) == 0)
            break;

        close(listen_fd);
        listen_fd = -1;

    }

    freeaddrinfo(addrs);

    if (listen_fd < 0) {
        guac_client_log(client, GUAC_LOG_ERROR, "Unable to bind %s:%s for "
                "reverse serial connection: %s", bind_addr, settings->port,
                strerror(errno));
        stream->open_status = GUAC_PROTOCOL_STATUS_SERVER_ERROR;
        return -1;
    }

    /* Listen for a single inbound connection */
    if (listen(listen_fd, 1) != 0) {
        guac_client_log(client, GUAC_LOG_ERROR, "Unable to listen on %s:%s for "
                "reverse serial connection: %s", bind_addr, settings->port,
                strerror(errno));
        close(listen_fd);
        stream->open_status = GUAC_PROTOCOL_STATUS_SERVER_ERROR;
        return -1;
    }

    guac_client_log(client, GUAC_LOG_INFO, "Listening on %s:%s for an inbound "
            "serial connection...", bind_addr, settings->port);

    /* Wait for one inbound connection, honoring both the listen timeout and
     * client shutdown. Poll in 1000ms slices so teardown stays responsive. */
    int timeout = settings->listen_timeout;   /* seconds; <= 0 waits forever */
    int waited_ms = 0;
    while (client->state == GUAC_CLIENT_RUNNING) {

        struct pollfd pfd = { .fd = listen_fd, .events = POLLIN };
        int pr = poll(&pfd, 1, 1000);
        if (pr < 0) {
            if (errno == EINTR)
                continue;
            guac_client_log(client, GUAC_LOG_WARNING, "Error while waiting for "
                    "an inbound serial connection on %s:%s: %s", bind_addr,
                    settings->port, strerror(errno));
            break;
        }

        /* An inbound connection is ready to accept */
        if (pr > 0 && (pfd.revents & POLLIN))
            break;

        /* Otherwise a slice elapsed; enforce the listen timeout if configured */
        if (timeout > 0) {
            waited_ms += 1000;
            if (waited_ms >= timeout * 1000) {
                guac_client_log(client, GUAC_LOG_ERROR, "Timed out after %ds "
                        "waiting for an inbound serial connection on %s:%s",
                        timeout, bind_addr, settings->port);
                close(listen_fd);
                stream->open_status = GUAC_PROTOCOL_STATUS_UPSTREAM_TIMEOUT;
                return -1;
            }
        }

    }

    /* Abort if the client is shutting down rather than accepting */
    if (client->state != GUAC_CLIENT_RUNNING) {
        close(listen_fd);
        stream->open_status = GUAC_PROTOCOL_STATUS_SERVER_ERROR;
        return -1;
    }

    /* Accept the single inbound connection and stop listening */
    struct sockaddr_storage peer;
    socklen_t plen = sizeof(peer);
    int fd;
    GUAC_RETRY_EINTR(fd, accept(listen_fd, (struct sockaddr*) &peer, &plen));
    close(listen_fd);

    if (fd < 0) {
        guac_client_log(client, GUAC_LOG_ERROR, "Failed to accept inbound "
                "serial connection on %s:%s: %s", bind_addr, settings->port,
                strerror(errno));
        stream->open_status = GUAC_PROTOCOL_STATUS_SERVER_ERROR;
        return -1;
    }

    /* Log the peer which connected */
    char host[NI_MAXHOST] = "?";
    char serv[NI_MAXSERV] = "?";
    getnameinfo((struct sockaddr*) &peer, plen, host, sizeof(host),
            serv, sizeof(serv), NI_NUMERICHOST | NI_NUMERICSERV);
    guac_client_log(client, GUAC_LOG_INFO, "Accepted inbound serial connection "
            "from %s:%s", host, serv);

    /* Keep an idle inbound link alive so a silent device drop is detected */
    int keepalive = 1;
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));

    return fd;

}

int guac_serial_net_open_fd(guac_serial_stream* stream, guac_client* client,
        guac_serial_settings* settings) {

    int fd;

    /* In reverse mode, accept an inbound connection rather than dialing out.
     * guac_serial_reverse_accept() already logs and sets open_status. */
    if (settings->reverse_connect) {
        fd = guac_serial_reverse_accept(stream, client, settings);
        if (fd < 0)
            return -1;
    }

    /* Otherwise dial out to the remote serial server */
    else {
        fd = guac_tcp_connect(settings->hostname, settings->port,
                settings->timeout);
        if (fd < 0) {

            int err = errno;

            /* Distinguish an actively-refused or reset endpoint from other
             * failures (e.g. name resolution, timeout) */
            if (err == ECONNREFUSED) {
                guac_client_log(client, GUAC_LOG_ERROR, "ser2net endpoint "
                        "\"%s:%s\" refused the connection: %s",
                        settings->hostname, settings->port, strerror(err));
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
    }

    /* Disable Nagle's algorithm so keystrokes are sent without delay */
    int flag = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

    return fd;

}

int guac_serial_tcp_open(guac_serial_stream* stream, guac_client* client,
        guac_serial_settings* settings) {

    int fd = guac_serial_net_open_fd(stream, client, settings);
    if (fd < 0)
        return -1;

    stream->backend = GUAC_SERIAL_BACKEND_TCP;
    stream->fd = fd;
    return 0;

}
