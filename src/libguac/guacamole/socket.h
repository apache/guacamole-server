/*
 * Copyright (C) 2015 Glyptodon LLC
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

#ifndef _GUAC_SOCKET_H
#define _GUAC_SOCKET_H

/**
 * Defines the guac_socket object and functionss for using and manipulating it.
 *
 * @file socket.h
 */

#include "socket-constants.h"
#include "socket-fntypes.h"
#include "socket-types.h"
#include "timestamp-types.h"

#include <pthread.h>
#include <stdint.h>
#include <unistd.h>

struct guac_socket {

    /**
     * Arbitrary socket-specific data.
     */
    void* data;

    /**
     * Handler which will be called when data needs to be read from the socket.
     */
    guac_socket_read_handler* read_handler;

    /**
     * Handler which will be called whenever data is written to this socket.
     */
    guac_socket_write_handler* write_handler;

    /**
     * Handler which will be called whenever this socket needs to be flushed.
     */
    guac_socket_flush_handler* flush_handler;

    /**
     * Handler which will be called whenever a socket needs to be acquired for
     * exclusive access, such as when an instruction is about to be written.
     */
    guac_socket_lock_handler* lock_handler;

    /**
     * Handler which will be called whenever exclusive access to a socket is
     * being released, such as when an instruction has finished being written.
     */
    guac_socket_unlock_handler* unlock_handler;

    /**
     * Handler which will be called whenever guac_socket_select() is invoked
     * on this socket.
     */
    guac_socket_select_handler* select_handler;

    /**
     * Handler which will be called when the socket is free'd (closed).
     */
    guac_socket_free_handler* free_handler;

    /**
     * The current state of this guac_socket.
     */
    guac_socket_state state;

    /**
     * The timestamp associated with the time the last block of data was
     * written to this guac_socket.
     */
    guac_timestamp last_write_timestamp;

    /**
     * The number of bytes present in the base64 "ready" buffer.
     */
    int __ready;

    /**
     * The base64 "ready" buffer. Once this buffer is filled, base64 data is
     * flushed to the main write buffer.
     */
    int __ready_buf[3];

    /**
     * Whether automatic keep-alive is enabled.
     */
    int __keep_alive_enabled;

    /**
     * The keep-alive thread.
     */
    pthread_t __keep_alive_thread;

};

/**
 * Allocates a new, completely blank guac_socket. This guac_socket will do
 * absolutely nothing when used unless its handlers are defined.
 *
 * @returns A newly-allocated guac_socket, or NULL if the guac_socket could
 *          not be allocated.
 */
guac_socket* guac_socket_alloc();

/**
 * Frees the given guac_socket and all associated resources.
 *
 * @param socket The guac_socket to free.
 */
void guac_socket_free(guac_socket* socket);

/**
 * Declares that the given socket must automatically send a keep-alive ping
 * to ensure neither side of the socket times out while the socket is open.
 * This ping will take the form of a "nop" instruction.
 *
 * @param socket
 *     The guac_socket to declare as requiring an automatic keep-alive ping.
 */
void guac_socket_require_keep_alive(guac_socket* socket);

/**
 * Marks the beginning of a Guacamole protocol instruction.
 *
 * @param socket
 *     The guac_socket beginning an instruction.
 */
void guac_socket_instruction_begin(guac_socket* socket);

/**
 * Marks the end of a Guacamole protocol instruction.
 *
 * @param socket
 *     The guac_socket ending an instruction.
 */
void guac_socket_instruction_end(guac_socket* socket);

/**
 * Allocates and initializes a new guac_socket object with the given open
 * file descriptor. The file descriptor will be automatically closed when
 * the allocated guac_socket is freed.
 *
 * If an error occurs while allocating the guac_socket object, NULL is returned,
 * and guac_error is set appropriately.
 *
 * @param fd An open file descriptor that this guac_socket object should manage.
 * @return A newly allocated guac_socket object associated with the given
 *         file descriptor, or NULL if an error occurs while allocating
 *         the guac_socket object.
 */
guac_socket* guac_socket_open(int fd);

/**
 * Allocates and initializes a new guac_socket which writes all data via
 * nest instructions to the given existing, open guac_socket. Freeing the
 * returned guac_socket has no effect on the underlying, nested guac_socket.
 *
 * If an error occurs while allocating the guac_socket object, NULL is returned,
 * and guac_error is set appropriately.
 *
 * @param parent The guac_socket this new guac_socket should write nest
 *               instructions to.
 * @param index The stream index to use for the written nest instructions.
 * @return A newly allocated guac_socket object associated with the given
 *         guac_socket and stream index, or NULL if an error occurs while
 *         allocating the guac_socket object.
 */
guac_socket* guac_socket_nest(guac_socket* parent, int index);

/**
 * Writes the given unsigned int to the given guac_socket object. The data
 * written may be buffered until the buffer is flushed automatically or
 * manually.
 *
 * If an error occurs while writing, a non-zero value is returned, and
 * guac_error is set appropriately.
 *
 * @param socket The guac_socket object to write to.
 * @param i The unsigned int to write.
 * @return Zero on success, or non-zero if an error occurs while writing.
 */
ssize_t guac_socket_write_int(guac_socket* socket, int64_t i);

/**
 * Writes the given string to the given guac_socket object. The data
 * written may be buffered until the buffer is flushed automatically or
 * manually.
 *
 * If an error occurs while writing, a non-zero value is returned, and
 * guac_error is set appropriately.
 *
 * @param socket The guac_socket object to write to.
 * @param str The string to write.
 * @return Zero on success, or non-zero if an error occurs while writing.
*/
ssize_t guac_socket_write_string(guac_socket* socket, const char* str);

/**
 * Writes the given binary data to the given guac_socket object as base64-
 * encoded data. The data written may be buffered until the buffer is flushed
 * automatically or manually. Beware that, because base64 data is buffered
 * on top of the write buffer already used, a call to guac_socket_flush_base64()
 * MUST be made before non-base64 writes (or writes of an independent block of
 * base64 data) can be made.
 *
 * If an error occurs while writing, a non-zero value is returned, and
 * guac_error is set appropriately.
 *
 * @param socket The guac_socket object to write to.
 * @param buf A buffer containing the data to write.
 * @param count The number of bytes to write.
 * @return Zero on success, or non-zero if an error occurs while writing.
 */
ssize_t guac_socket_write_base64(guac_socket* socket, const void* buf, size_t count);

/**
 * Writes the given data to the specified socket. The data written may be
 * buffered until the buffer is flushed automatically or manually.
 *
 * If an error occurs while writing, a non-zero value is returned, and
 * guac_error is set appropriately.
 *
 * @param socket The guac_socket object to write to.
 * @param buf A buffer containing the data to write.
 * @param count The number of bytes to write.
 * @return Zero on success, or non-zero if an error occurs while writing.
 */
ssize_t guac_socket_write(guac_socket* socket, const void* buf, size_t count);

/**
 * Attempts to read data from the socket, filling up to the specified number
 * of bytes in the given buffer.
 *
 * If an error occurs while writing, a non-zero value is returned, and
 * guac_error is set appropriately.
 *
 * @param socket The guac_socket to read from.
 * @param buf The buffer to read bytes into.
 * @param count The maximum number of bytes to read.
 * @return The number of bytes read, or non-zero if an error occurs while
 *         reading.
 */
ssize_t guac_socket_read(guac_socket* socket, void* buf, size_t count);

/**
 * Flushes the base64 buffer, writing padding characters as necessary.
 *
 * If an error occurs while writing, a non-zero value is returned, and
 * guac_error is set appropriately.
 *
 * @param socket The guac_socket object to flush
 * @return Zero on success, or non-zero if an error occurs during flush.
 */
ssize_t guac_socket_flush_base64(guac_socket* socket);

/**
 * Flushes the write buffer.
 *
 * If an error occurs while writing, a non-zero value is returned, and
 * guac_error is set appropriately.
 *
 * @param socket The guac_socket object to flush
 * @return Zero on success, or non-zero if an error occurs during flush.
 */
ssize_t guac_socket_flush(guac_socket* socket);

/**
 * Waits for input to be available on the given guac_socket object until the
 * specified timeout elapses.
 *
 * If an error occurs while waiting, a negative value is returned, and
 * guac_error is set appropriately.
 *
 * If a timeout occurs while waiting, zero value is returned, and
 * guac_error is set to GUAC_STATUS_INPUT_TIMEOUT.
 *
 * @param socket The guac_socket object to wait for.
 * @param usec_timeout The maximum number of microseconds to wait for data, or
 *                     -1 to potentially wait forever.
 * @return Positive on success, zero if the timeout elapsed and no data is
 *         available, negative on error.
 */
int guac_socket_select(guac_socket* socket, int usec_timeout);

#endif

