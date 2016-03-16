/*
 * Copyright (C) 2016 Glyptodon LLC
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

#include "socket.h"

#include <stdlib.h>

/**
 * Data specific to the tee implementation of guac_socket.
 */
typedef struct guac_socket_tee_data {

    /**
     * The guac_socket to which all socket operations should be delegated.
     */
    guac_socket* primary;

    /**
     * The guac_socket to which all write and flush operations should be
     * duplicated.
     */
    guac_socket* secondary;

} guac_socket_tee_data;

/**
 * Callback function which reads only from the primary socket.
 *
 * @param socket
 *     The tee socket to read from.
 *
 * @param buf
 *     The buffer to read data into.
 *
 * @param count
 *     The maximum number of bytes to read into the given buffer.
 *
 * @return
 *     The value returned by guac_socket_read() when invoked on the primary
 *     socket with the given parameters.
 */
static ssize_t __guac_socket_tee_read_handler(guac_socket* socket,
        void* buf, size_t count) {

    guac_socket_tee_data* data = (guac_socket_tee_data*) socket->data;

    /* Delegate read to wrapped socket */
    return guac_socket_read(data->primary, buf, count);

}

/**
 * Callback function which writes the given data to both underlying sockets,
 * returning only the result from the primary socket.
 *
 * @param socket
 *     The tee socket to write through.
 *
 * @param buf
 *     The buffer of data to write.
 *
 * @param count
 *     The number of bytes in the buffer to be written.
 *
 * @return
 *     The number of bytes written if the write was successful, or -1 if an
 *     error occurs.
 */
static ssize_t __guac_socket_tee_write_handler(guac_socket* socket,
        const void* buf, size_t count) {

    guac_socket_tee_data* data = (guac_socket_tee_data*) socket->data;

    /* Write to secondary socket (ignoring result) */
    guac_socket_write(data->secondary, buf, count);

    /* Delegate write to wrapped socket */
    if (guac_socket_write(data->primary, buf, count))
        return -1;

    /* All data written successfully */
    return count;

}

/**
 * Callback function which flushes both underlying sockets, returning only the
 * result from the primary socket.
 *
 * @param socket
 *     The tee socket to flush.
 *
 * @return
 *     The value returned by guac_socket_flush() when invoked on the primary
 *     socket.
 */
static ssize_t __guac_socket_tee_flush_handler(guac_socket* socket) {

    guac_socket_tee_data* data = (guac_socket_tee_data*) socket->data;

    /* Flush secondary socket (ignoring result) */
    guac_socket_flush(data->secondary);

    /* Delegate flush to wrapped socket */
    return guac_socket_flush(data->primary);

}

/**
 * Callback function which delegates the lock operation to the primary
 * socket alone.
 *
 * @param socket
 *     The tee socket on which guac_socket_instruction_begin() was invoked.
 */
static void __guac_socket_tee_lock_handler(guac_socket* socket) {

    guac_socket_tee_data* data = (guac_socket_tee_data*) socket->data;

    /* Delegate lock to wrapped socket */
    guac_socket_instruction_begin(data->primary);

}

/**
 * Callback function which delegates the unlock operation to the primary
 * socket alone.
 *
 * @param socket
 *     The tee socket on which guac_socket_instruction_end() was invoked.
 */
static void __guac_socket_tee_unlock_handler(guac_socket* socket) {

    guac_socket_tee_data* data = (guac_socket_tee_data*) socket->data;

    /* Delegate unlock to wrapped socket */
    guac_socket_instruction_end(data->primary);

}

/**
 * Callback function which delegates the select operation to the primary
 * socket alone.
 *
 * @param socket
 *     The tee socket on which guac_socket_select() was invoked.
 *
 * @param usec_timeout
 *     The timeout to specify when invoking guac_socket_select() on the
 *     primary socket.
 *
 * @return
 *     The value returned by guac_socket_select() when invoked with the
 *     given parameters on the primary socket.
 */
static int __guac_socket_tee_select_handler(guac_socket* socket,
        int usec_timeout) {

    guac_socket_tee_data* data = (guac_socket_tee_data*) socket->data;

    /* Delegate select to wrapped socket */
    return guac_socket_select(data->primary, usec_timeout);

}

/**
 * Callback function which frees all underlying data associated with the
 * given tee socket, including both primary and secondary sockets.
 *
 * @param socket
 *     The tee socket being freed.
 *
 * @return
 *     Always zero.
 */
static int __guac_socket_tee_free_handler(guac_socket* socket) {

    guac_socket_tee_data* data = (guac_socket_tee_data*) socket->data;

    /* Free underlying sockets */
    guac_socket_free(data->primary);
    guac_socket_free(data->secondary);

    /* Freeing the tee socket always succeeds */
    free(data);
    return 0;

}

guac_socket* guac_socket_tee(guac_socket* primary, guac_socket* secondary) {

    /* Set up socket to split outout into a file */
    guac_socket_tee_data* data = malloc(sizeof(guac_socket_tee_data));
    data->primary = primary;
    data->secondary = secondary;

    /* Associate tee-specific data with new socket */
    guac_socket* socket = guac_socket_alloc();
    socket->data = data;

    /* Assign handlers */
    socket->read_handler   = __guac_socket_tee_read_handler;
    socket->write_handler  = __guac_socket_tee_write_handler;
    socket->select_handler = __guac_socket_tee_select_handler;
    socket->flush_handler  = __guac_socket_tee_flush_handler;
    socket->lock_handler   = __guac_socket_tee_lock_handler;
    socket->unlock_handler = __guac_socket_tee_unlock_handler;
    socket->free_handler   = __guac_socket_tee_free_handler;

    return socket;

}

