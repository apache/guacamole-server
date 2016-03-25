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

#include "protocol.h"
#include "socket.h"
#include "unicode.h"

#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#define GUAC_SOCKET_NEST_BUFFER_SIZE 8192

typedef struct __guac_socket_nest_data {

    guac_socket* parent;
    char buffer[GUAC_SOCKET_NEST_BUFFER_SIZE];
    int index;

} __guac_socket_nest_data;

ssize_t __guac_socket_nest_write_handler(guac_socket* socket,
        const void* buf, size_t count) {

    __guac_socket_nest_data* data = (__guac_socket_nest_data*) socket->data;
    unsigned char* source = (unsigned char*) buf;

    /* Current location in destination buffer during copy */
    char* current = data->buffer;

    /* Number of bytes remaining in source buffer */
    int remaining = count;

    /* If we can't actually store that many bytes, reduce number of bytes
     * expected to be written */
    if (remaining > GUAC_SOCKET_NEST_BUFFER_SIZE)
        remaining = GUAC_SOCKET_NEST_BUFFER_SIZE;

    /* Current offset within destination buffer */
    int offset;

    /* Number of characters before start of next character */
    int skip = 0;

    /* Copy UTF-8 characters into buffer */
    for (offset = 0; offset < GUAC_SOCKET_NEST_BUFFER_SIZE; offset++) {

        /* Get next byte */
        unsigned char c = *source;
        remaining--;

        /* If skipping, then skip */
        if (skip > 0) skip--;

        /* Otherwise, determine next skip value, and increment length */
        else {

            /* Determine skip value (size in bytes of rest of character) */
            skip = guac_utf8_charsize(c) - 1;

            /* If not enough bytes to complete character, break */
            if (skip > remaining)
                break;

        }

        /* Store byte */
        *current = c;

        /* Advance to next character */
        source++;
        current++;

    }

    /* Append null-terminator */
    *current = 0;

    /* Send nest instruction containing read UTF-8 segment */
    guac_protocol_send_nest(data->parent, data->index, data->buffer);

    /* Return number of bytes actually written */
    return offset;

}

/**
 * Frees all implementation-specific data associated with the given socket, but
 * not the socket object itself.
 *
 * @param socket
 *     The guac_socket whose associated data should be freed.
 *
 * @return
 *     Zero if the data was successfully freed, non-zero otherwise. This
 *     implementation always succeeds, and will always return zero.
 */
static int __guac_socket_nest_free_handler(guac_socket* socket) {

    /* Free associated data */
    __guac_socket_nest_data* data = (__guac_socket_nest_data*) socket->data;
    free(data);

    return 0;

}

guac_socket* guac_socket_nest(guac_socket* parent, int index) {

    /* Allocate socket and associated data */
    guac_socket* socket = guac_socket_alloc();
    __guac_socket_nest_data* data = malloc(sizeof(__guac_socket_nest_data));

    /* Store file descriptor as socket data */
    data->parent = parent;
    socket->data = data;

    /* Set write and free handlers */
    socket->write_handler  = __guac_socket_nest_write_handler;
    socket->free_handler   = __guac_socket_nest_free_handler;

    return socket;

}

