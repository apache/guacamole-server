/*
 * Copyright (C) 2014 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef GUACD_CONNECTION_H
#define GUACD_CONNECTION_H

#include "config.h"

#include "proc-map.h"

#ifdef ENABLE_SSL
#include <openssl/ssl.h>
#endif

/**
 * Parameters required by each connection thread.
 */
typedef struct guacd_connection_thread_params {

    /**
     * The shared map of all connected clients.
     */
    guacd_proc_map* map;

#ifdef ENABLE_SSL
    /**
     * SSL context for encrypted connections to guacd. If SSL is not active,
     * this will be NULL.
     */
    SSL_CTX* ssl_context;
#endif

    /**
     * The file descriptor associated with the newly-accepted connection.
     */
    int connected_socket_fd;

} guacd_connection_thread_params;

/**
 * Handles an inbound connection to guacd, allowing guacd to continue listening
 * for other connections. It is expected that this thread will operate
 * detached. The creating process need not join on the resulting thread.
 */
void* guacd_connection_thread(void* data);

/**
 * Parameters required by the per-connection I/O transfer thread.
 */
typedef struct guacd_connection_io_thread_params {

    /**
     * The guac_parser which may contain buffered, but unparsed, data from the
     * original guac_socket which must be transferred to the
     * connection-specific process.
     */
    guac_parser* parser;

    /**
     * The guac_socket which is directly handling I/O from a user's connection
     * to guacd.
     */
    guac_socket* socket;

    /**
     * The file descriptor which is being handled by a guac_socket within the
     * connection-specific process.
     */
    int fd;

} guacd_connection_io_thread_params;

/**
 * Transfers data back and forth between the guacd-side guac_socket and the
 * file descriptor used by the process-side guac_socket.
 */
void* guacd_connection_io_thread(void* data);

#endif

