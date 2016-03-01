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

#ifndef _GUAC_SOCKET_FNTYPES_H
#define _GUAC_SOCKET_FNTYPES_H

/**
 * Function type definitions related to the guac_socket object.
 *
 * @file socket-fntypes.h
 */

#include "socket-types.h"

#include <unistd.h>

/**
 * Generic read handler for socket read operations, modeled after the standard
 * POSIX read() function. When set within a guac_socket, a handler of this type
 * will be called when data needs to be read into the socket.
 *
 * @param socket The guac_socket being read from.
 * @param buf The arbitrary buffer we must populate with data.
 * @param count The maximum number of bytes to read into the buffer.
 * @return The number of bytes read, or -1 if an error occurs.
 */
typedef ssize_t guac_socket_read_handler(guac_socket* socket,
        void* buf, size_t count);

/**
 * Generic write handler for socket write operations, modeled after the standard
 * POSIX write() function. When set within a guac_socket, a handler of this type
 * will be called when data needs to be written to the socket.
 *
 * @param socket The guac_socket being written to.
 * @param buf The arbitrary buffer containing data to be written.
 * @param count The maximum number of bytes to written to the buffer.
 * @return The number of bytes written, or -1 if an error occurs.
 */
typedef ssize_t guac_socket_write_handler(guac_socket* socket,
        const void* buf, size_t count);

/**
 * Generic handler for socket select operations, similar to the POSIX select()
 * function. When guac_socket_select() is called on a guac_socket, its
 * guac_socket_select_handler will be invoked, if defined.
 *
 * @param socket The guac_socket being selected.
 * @param usec_timeout The maximum number of microseconds to wait for data, or
 *                     -1 to potentially wait forever.
 * @return Positive on success, zero if the timeout elapsed and no data is
 *         available, negative on error.
 */
typedef int guac_socket_select_handler(guac_socket* socket, int usec_timeout);

/**
 * Generic flush handler for socket flush operations. This function is not
 * modeled after any POSIX function. When set within a guac_socket, a handler
 * of this type will be called when guac_socket_flush() is called.
 *
 * @param socket
 *     The guac_socket being flushed.
 *
 * @return
 *     Zero on success, or non-zero if an error occurs during flush.
 */
typedef ssize_t guac_socket_flush_handler(guac_socket* socket);

/**
 * When set within a guac_socket, a handler of this type will be called
 * whenever exclusive access to the guac_socket is required, such as when
 * guac_socket_instruction_begin() is called.
 *
 * @param socket
 *     The guac_socket to which exclusive access is required.
 */
typedef void guac_socket_lock_handler(guac_socket* socket);

/**
 * When set within a guac_socket, a handler of this type will be called
 * whenever exclusive access to the guac_socket is no longer required, such as
 * when guac_socket_instruction_end() is called.
 *
 * @param socket
 *     The guac_socket to which exclusive access is no longer required.
 */
typedef void guac_socket_unlock_handler(guac_socket* socket);

/**
 * Generic handler for the closing of a socket, modeled after the standard
 * POSIX close() function. When set within a guac_socket, a handler of this type
 * will be called when the socket is closed.
 *
 * @param socket The guac_socket being closed.
 * @return Zero on success, or -1 if an error occurs.
 */
typedef int guac_socket_free_handler(guac_socket* socket);

#endif

