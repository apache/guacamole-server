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
#include "guacamole/protocol.h"
#include "guacamole/socket.h"
#include "guacamole/timestamp.h"

#include <inttypes.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

char __guac_socket_BASE64_CHARACTERS[64] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
    'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
    'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's',
    't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', '+', '/'
};

static void* __guac_socket_keep_alive_thread(void* data) {

    /* Calculate sleep interval */
    struct timespec interval;
    interval.tv_sec  =  GUAC_SOCKET_KEEP_ALIVE_INTERVAL / 1000;
    interval.tv_nsec = (GUAC_SOCKET_KEEP_ALIVE_INTERVAL % 1000) * 1000000L;

    /* Socket keep-alive loop */
    guac_socket* socket = (guac_socket*) data;
    while (socket->state == GUAC_SOCKET_OPEN) {

        /* Send NOP keep-alive if it's been a while since the last output */
        guac_timestamp timestamp = guac_timestamp_current();
        if (timestamp - socket->last_write_timestamp >
                GUAC_SOCKET_KEEP_ALIVE_INTERVAL) {

            /* Send NOP */
            if (guac_protocol_send_nop(socket)
                || guac_socket_flush(socket))
                break;

        }

        /* Sleep until next keep-alive check */
        nanosleep(&interval, NULL);

    }

    return NULL;

}

static ssize_t __guac_socket_write(guac_socket* socket,
        const void* buf, size_t count) {

    /* Update timestamp of last write */
    socket->last_write_timestamp = guac_timestamp_current();

    /* If handler defined, call it. */
    if (socket->write_handler)
        return socket->write_handler(socket, buf, count);

    /* Otherwise, pretend everything was written. */
    return count;

}

ssize_t guac_socket_write(guac_socket* socket,
        const void* buf, size_t count) {

    const char* buffer = buf;

    /* Write until completely written */
    while (count > 0) {

        /* Attempt to write, return on error */
        int written = __guac_socket_write(socket, buffer, count);
        if (written == -1)
            return 1;

        /* Advance buffer as data written */
        buffer += written;
        count  -= written;

    }

    return 0;

}

ssize_t guac_socket_read(guac_socket* socket, void* buf, size_t count) {

    /* If handler defined, call it. */
    if (socket->read_handler)
        return socket->read_handler(socket, buf, count);

    /* Otherwise, pretend nothing was read. */
    return 0;

}

int guac_socket_select(guac_socket* socket, int usec_timeout) {

    /* Call select handler if defined */
    if (socket->select_handler)
        return socket->select_handler(socket, usec_timeout);

    /* Otherwise, assume ready. */
    return 1;

}

guac_socket* guac_socket_alloc() {

    guac_socket* socket = malloc(sizeof(guac_socket));

    /* If no memory available, return with error */
    if (socket == NULL) {
        guac_error = GUAC_STATUS_NO_MEMORY;
        guac_error_message = "Could not allocate memory for socket";
        return NULL;
    }

    socket->__ready = 0;
    socket->data = NULL;
    socket->state = GUAC_SOCKET_OPEN;
    socket->last_write_timestamp = guac_timestamp_current();

    /* No keep alive ping by default */
    socket->__keep_alive_enabled = 0;

    /* No handlers yet */
    socket->read_handler   = NULL;
    socket->write_handler  = NULL;
    socket->select_handler = NULL;
    socket->free_handler   = NULL;
    socket->flush_handler  = NULL;
    socket->lock_handler   = NULL;
    socket->unlock_handler = NULL;

    return socket;

}

void guac_socket_require_keep_alive(guac_socket* socket) {

    /* Start keep-alive thread */
    socket->__keep_alive_enabled = 1;
    pthread_create(&(socket->__keep_alive_thread), NULL,
                __guac_socket_keep_alive_thread, (void*) socket);

}

void guac_socket_instruction_begin(guac_socket* socket) {

    /* Call instruction begin handler if defined */
    if (socket->lock_handler)
        socket->lock_handler(socket);

}

void guac_socket_instruction_end(guac_socket* socket) {

    /* Call instruction end handler if defined */
    if (socket->unlock_handler)
        socket->unlock_handler(socket);

}

void guac_socket_free(guac_socket* socket) {

    guac_socket_flush(socket);

    /* Call free handler if defined */
    if (socket->free_handler)
        socket->free_handler(socket);

    /* Mark as closed */
    socket->state = GUAC_SOCKET_CLOSED;

    /* Wait for keep-alive, if enabled */
    if (socket->__keep_alive_enabled)
        pthread_join(socket->__keep_alive_thread, NULL);

    free(socket);
}

ssize_t guac_socket_write_int(guac_socket* socket, int64_t i) {

    char buffer[128];
    int length;

    /* Write provided integer as a string */
    length = snprintf(buffer, sizeof(buffer), "%"PRIi64, i);
    return guac_socket_write(socket, buffer, length);

}

ssize_t guac_socket_write_string(guac_socket* socket, const char* str) {

    /* Write contents of string */
    if (guac_socket_write(socket, str, strlen(str)))
        return 1;

    return 0;

}

ssize_t __guac_socket_write_base64_triplet(guac_socket* socket,
        int a, int b, int c) {

    char output[4];

    /* Byte 0:[AAAAAA] AABBBB BBBBCC CCCCCC */
    output[0] = __guac_socket_BASE64_CHARACTERS[(a & 0xFC) >> 2];

    if (b >= 0) {

        /* Byte 1: AAAAAA [AABBBB] BBBBCC CCCCCC */
        output[1] = __guac_socket_BASE64_CHARACTERS[((a & 0x03) << 4) | ((b & 0xF0) >> 4)];

        /* 
         * Bytes 2 and 3, zero characters of padding:
         *
         * AAAAAA  AABBBB [BBBBCC] CCCCCC
         * AAAAAA  AABBBB  BBBBCC [CCCCCC]
         */
        if (c >= 0) {
            output[2] = __guac_socket_BASE64_CHARACTERS[((b & 0x0F) << 2) | ((c & 0xC0) >> 6)];
            output[3] = __guac_socket_BASE64_CHARACTERS[c & 0x3F];
        }

        /* 
         * Bytes 2 and 3, one character of padding:
         *
         * AAAAAA  AABBBB [BBBB--] ------
         * AAAAAA  AABBBB  BBBB-- [------]
         */
        else { 
            output[2] = __guac_socket_BASE64_CHARACTERS[((b & 0x0F) << 2)];
            output[3] = '=';
        }
    }

    /* 
     * Bytes 1, 2, and 3, two characters of padding:
     *
     * AAAAAA [AA----] ------  ------
     * AAAAAA  AA---- [------] ------
     * AAAAAA  AA----  ------ [------]
     */
    else {
        output[1] = __guac_socket_BASE64_CHARACTERS[((a & 0x03) << 4)];
        output[2] = '=';
        output[3] = '=';
    }

    /* At this point, 4 base64 bytes have been written */
    if (guac_socket_write(socket, output, 4))
        return -1;

    /* If no second byte was provided, only one byte was written */
    if (b < 0)
        return 1;

    /* If no third byte was provided, only two bytes were written */
    if (c < 0)
        return 2;

    /* Otherwise, three bytes were written */
    return 3;

}

ssize_t __guac_socket_write_base64_byte(guac_socket* socket, int buf) {

    int* __ready_buf = socket->__ready_buf;

    int retval;

    __ready_buf[socket->__ready++] = buf;

    /* Flush triplet */
    if (socket->__ready == 3) {
        retval = __guac_socket_write_base64_triplet(socket, __ready_buf[0], __ready_buf[1], __ready_buf[2]);
        if (retval < 0)
            return retval;

        socket->__ready = 0;
    }

    return 1;
}

ssize_t guac_socket_write_base64(guac_socket* socket, const void* buf, size_t count) {

    int retval;

    const unsigned char* char_buf = (const unsigned char*) buf;
    const unsigned char* end = char_buf + count;

    while (char_buf < end) {

        retval = __guac_socket_write_base64_byte(socket, *(char_buf++));
        if (retval < 0)
            return retval;

    }

    return 0;

}

ssize_t guac_socket_flush(guac_socket* socket) {

    /* If handler defined, call it. */
    if (socket->flush_handler)
        return socket->flush_handler(socket);

    /* Otherwise, do nothing */
    return 0;

}

ssize_t guac_socket_flush_base64(guac_socket* socket) {

    int retval;

    /* Flush triplet to output buffer */
    while (socket->__ready > 0) {

        retval = __guac_socket_write_base64_byte(socket, -1);
        if (retval < 0)
            return retval;

    }

    return 0;

}

