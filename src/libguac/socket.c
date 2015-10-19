/*
 * Copyright (C) 2013 Glyptodon LLC
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

#include "config.h"

#include "error.h"
#include "protocol.h"
#include "socket.h"
#include "timestamp.h"

#include <inttypes.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
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

    pthread_mutexattr_t lock_attributes;
    guac_socket* socket = malloc(sizeof(guac_socket));

    /* If no memory available, return with error */
    if (socket == NULL) {
        guac_error = GUAC_STATUS_NO_MEMORY;
        guac_error_message = "Could not allocate memory for socket";
        return NULL;
    }

    socket->__ready = 0;
    socket->__written = 0;
    socket->data = NULL;
    socket->state = GUAC_SOCKET_OPEN;
    socket->last_write_timestamp = guac_timestamp_current();

    /* Init members */
    socket->__instructionbuf_unparsed_start = socket->__instructionbuf;
    socket->__instructionbuf_unparsed_end = socket->__instructionbuf;

    /* Default to unsafe threading */
    socket->__threadsafe_instructions = 0;

    /* No keep alive ping by default */
    socket->__keep_alive_enabled = 0;

    pthread_mutexattr_init(&lock_attributes);
    pthread_mutexattr_setpshared(&lock_attributes, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&(socket->__instruction_write_lock), &lock_attributes);
    pthread_mutex_init(&(socket->__buffer_lock),            &lock_attributes);
    
    /* No handlers yet */
    socket->read_handler   = NULL;
    socket->write_handler  = NULL;
    socket->select_handler = NULL;
    socket->free_handler   = NULL;

    return socket;

}

void guac_socket_require_threadsafe(guac_socket* socket) {
    socket->__threadsafe_instructions = 1;
}

void guac_socket_require_keep_alive(guac_socket* socket) {

    /* Keep-alive thread requires a threadsafe socket */
    guac_socket_require_threadsafe(socket);

    /* Start keep-alive thread */
    socket->__keep_alive_enabled = 1;
    pthread_create(&(socket->__keep_alive_thread), NULL,
                __guac_socket_keep_alive_thread, (void*) socket);

}

void guac_socket_instruction_begin(guac_socket* socket) {

    /* Lock writes if threadsafety enabled */
    if (socket->__threadsafe_instructions)
        pthread_mutex_lock(&(socket->__instruction_write_lock));

}

void guac_socket_instruction_end(guac_socket* socket) {

    /* Unlock writes if threadsafety enabled */
    if (socket->__threadsafe_instructions)
        pthread_mutex_unlock(&(socket->__instruction_write_lock));

}

void guac_socket_update_buffer_begin(guac_socket* socket) {

    /* Lock if threadsafety enabled */
    if (socket->__threadsafe_instructions)
        pthread_mutex_lock(&(socket->__buffer_lock));

}

void guac_socket_update_buffer_end(guac_socket* socket) {

    /* Unlock if threadsafety enabled */
    if (socket->__threadsafe_instructions)
        pthread_mutex_unlock(&(socket->__buffer_lock));

}

void guac_socket_free(guac_socket* socket) {

    /* Call free handler if defined */
    if (socket->free_handler)
        socket->free_handler(socket);

    guac_socket_flush(socket);

    /* Mark as closed */
    socket->state = GUAC_SOCKET_CLOSED;

    /* Wait for keep-alive, if enabled */
    if (socket->__keep_alive_enabled)
        pthread_join(socket->__keep_alive_thread, NULL);

    pthread_mutex_destroy(&(socket->__instruction_write_lock));
    free(socket);
}

ssize_t guac_socket_write_int(guac_socket* socket, int64_t i) {

    char buffer[128];
    snprintf(buffer, sizeof(buffer), "%"PRIi64, i);
    return guac_socket_write_string(socket, buffer);

}

ssize_t guac_socket_write_string(guac_socket* socket, const char* str) {

    char* __out_buf = socket->__out_buf;

    guac_socket_update_buffer_begin(socket);

    for (; *str != '\0'; str++) {

        __out_buf[socket->__written++] = *str; 

        /* Flush when necessary, return on error. Note that we must flush within 4 bytes of boundary because
         * __guac_socket_write_base64_triplet ALWAYS writes four bytes, and would otherwise potentially overflow
         * the buffer. */
        if (socket->__written > GUAC_SOCKET_OUTPUT_BUFFER_SIZE - 4) {

            if (guac_socket_write(socket, __out_buf, socket->__written)) {
                guac_socket_update_buffer_end(socket);
                return 1;
            }

            socket->__written = 0;

        }

    }

    guac_socket_update_buffer_end(socket);
    return 0;

}

ssize_t __guac_socket_write_base64_triplet(guac_socket* socket, int a, int b, int c) {

    char* __out_buf = socket->__out_buf;

    /* Byte 1 */
    __out_buf[socket->__written++] = __guac_socket_BASE64_CHARACTERS[(a & 0xFC) >> 2]; /* [AAAAAA]AABBBB BBBBCC CCCCCC */

    if (b >= 0) {
        __out_buf[socket->__written++] = __guac_socket_BASE64_CHARACTERS[((a & 0x03) << 4) | ((b & 0xF0) >> 4)]; /* AAAAAA[AABBBB]BBBBCC CCCCCC */

        if (c >= 0) {
            __out_buf[socket->__written++] = __guac_socket_BASE64_CHARACTERS[((b & 0x0F) << 2) | ((c & 0xC0) >> 6)]; /* AAAAAA AABBBB[BBBBCC]CCCCCC */
            __out_buf[socket->__written++] = __guac_socket_BASE64_CHARACTERS[c & 0x3F]; /* AAAAAA AABBBB BBBBCC[CCCCCC] */
        }
        else { 
            __out_buf[socket->__written++] = __guac_socket_BASE64_CHARACTERS[((b & 0x0F) << 2)]; /* AAAAAA AABBBB[BBBB--]------ */
            __out_buf[socket->__written++] = '='; /* AAAAAA AABBBB BBBB--[------] */
        }
    }
    else {
        __out_buf[socket->__written++] = __guac_socket_BASE64_CHARACTERS[((a & 0x03) << 4)]; /* AAAAAA[AA----]------ ------ */
        __out_buf[socket->__written++] = '='; /* AAAAAA AA----[------]------ */
        __out_buf[socket->__written++] = '='; /* AAAAAA AA---- ------[------] */
    }

    /* At this point, 4 bytes have been socket->__written */

    /* Flush when necessary, return on error */
    if (socket->__written > GUAC_SOCKET_OUTPUT_BUFFER_SIZE - 4) {

        if (guac_socket_write(socket, __out_buf, socket->__written))
            return -1;

        socket->__written = 0;
    }

    if (b < 0)
        return 1;

    if (c < 0)
        return 2;

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

    guac_socket_update_buffer_begin(socket);
    while (char_buf < end) {

        retval = __guac_socket_write_base64_byte(socket, *(char_buf++));
        if (retval < 0) {
            guac_socket_update_buffer_end(socket);
            return retval;
        }

    }

    guac_socket_update_buffer_end(socket);
    return 0;

}

ssize_t guac_socket_flush(guac_socket* socket) {

    /* Flush remaining bytes in buffer */
    guac_socket_update_buffer_begin(socket);
    if (socket->__written > 0) {

        if (guac_socket_write(socket, socket->__out_buf, socket->__written)) {
            guac_socket_update_buffer_end(socket);
            return 1;
        }

        socket->__written = 0;
    }

    guac_socket_update_buffer_end(socket);
    return 0;

}

ssize_t guac_socket_flush_base64(guac_socket* socket) {

    int retval;

    /* Flush triplet to output buffer */
    guac_socket_update_buffer_begin(socket);
    while (socket->__ready > 0) {

        retval = __guac_socket_write_base64_byte(socket, -1);
        if (retval < 0) {
            guac_socket_update_buffer_end(socket);
            return retval;
        }

    }

    guac_socket_update_buffer_end(socket);
    return 0;

}

