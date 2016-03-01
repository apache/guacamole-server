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

#include "config.h"

#include "error.h"
#include "socket.h"

#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef __MINGW32__
#include <winsock2.h>
#else
#include <sys/select.h>
#endif

/**
 * Data associated with an open socket which writes to a file descriptor.
 */
typedef struct guac_socket_fd_data {

    /**
     * The associated file descriptor;
     */
    int fd;

    /**
     * The number of bytes currently in the main write buffer.
     */
    int written;

    /**
     * The main write buffer. Bytes written go here before being flushed
     * to the open file descriptor.
     */
    char out_buf[GUAC_SOCKET_OUTPUT_BUFFER_SIZE];

    /**
     * Lock which is acquired when an instruction is being written, and
     * released when the instruction is finished being written.
     */
    pthread_mutex_t socket_lock;

    /**
     * Lock which protects access to the internal buffer of this socket,
     * guaranteeing atomicity of writes and flushes.
     */
    pthread_mutex_t buffer_lock;

} guac_socket_fd_data;

/**
 * Writes the entire contents of the given buffer to the file descriptor
 * associated with the given socket, retrying as necessary until the whole
 * buffer is written, and aborting if an error occurs.
 *
 * @param socket
 *     The guac_socket associated with the file descriptor to which the given
 *     buffer should be written.
 *
 * @param buf
 *     The buffer of data to write to the given guac_socket.
 *
 * @param count
 *     The number of bytes within the given buffer.
 *
 * @return
 *     The number of bytes written, which will be exactly the size of the given
 *     buffer, or a negative value if an error occurs.
 */
ssize_t guac_socket_fd_write(guac_socket* socket,
        const void* buf, size_t count) {

    guac_socket_fd_data* data = (guac_socket_fd_data*) socket->data;
    const char* buffer = buf;

    /* Write until completely written */
    while (count > 0) {

        int retval;

#ifdef __MINGW32__
        /* MINGW32 WINSOCK only works with send() */
        retval = send(data->fd, buf, count, 0);
#else
        /* Use write() for all other platforms */
        retval = write(data->fd, buf, count);
#endif

        /* Record errors in guac_error */
        if (retval < 0) {
            guac_error = GUAC_STATUS_SEE_ERRNO;
            guac_error_message = "Error writing data to socket";
            return retval;
        }

        /* Advance buffer as data retval */
        buffer += retval;
        count  -= retval;

    }

    return 0;

}

static ssize_t guac_socket_fd_read_handler(guac_socket* socket,
        void* buf, size_t count) {

    guac_socket_fd_data* data = (guac_socket_fd_data*) socket->data;

    /* Read from socket */
    int retval = read(data->fd, buf, count);

    /* Record errors in guac_error */
    if (retval < 0) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Error reading data from socket";
    }

    return retval;

}

/**
 * Flushes the contents of the output buffer of the given socket immediately,
 * without first locking access to the output buffer. This function must ONLY
 * be called if the buffer lock has already been acquired.
 *
 * @param socket
 *     The guac_socket to flush.
 *
 * @return
 *     Zero if the flush operation was successful, non-zero otherwise.
 */
static ssize_t guac_socket_fd_flush(guac_socket* socket) {

    guac_socket_fd_data* data = (guac_socket_fd_data*) socket->data;

    /* Flush remaining bytes in buffer */
    if (data->written > 0) {

        /* Write ALL bytes in buffer immediately */
        if (guac_socket_fd_write(socket, data->out_buf, data->written))
            return 1;

        data->written = 0;
    }

    return 0;

}

static ssize_t guac_socket_fd_flush_handler(guac_socket* socket) {

    int retval;
    guac_socket_fd_data* data = (guac_socket_fd_data*) socket->data;

    /* Acquire exclusive access to buffer */
    pthread_mutex_lock(&(data->buffer_lock));

    /* Flush contents of buffer */
    retval = guac_socket_fd_flush(socket);

    /* Relinquish exclusive access to buffer */
    pthread_mutex_unlock(&(data->buffer_lock));

    return retval;

}

/**
 * Writes the contents of the buffer to the output buffer of the given socket,
 * flushing the output buffer as necessary, without first locking access to the
 * output buffer. This function must ONLY be called if the buffer lock has
 * already been acquired.
 *
 * @param socket
 *     The guac_socket to write the given buffer to.
 *
 * @param buf
 *     The buffer to write to the given socket.
 *
 * @param count
 *     The number of bytes in the given buffer.
 *
 * @return
 *     The number of bytes written, or a negative value if an error occurs
 *     during write.
 */
static ssize_t guac_socket_fd_write_buffered(guac_socket* socket,
        const void* buf, size_t count) {

    size_t original_count = count;
    const char* current = buf;
    guac_socket_fd_data* data = (guac_socket_fd_data*) socket->data;

    /* Append to buffer, flush if necessary */
    while (count > 0) {

        int chunk_size;
        int remaining = sizeof(data->out_buf) - data->written;

        /* If no space left in buffer, flush and retry */
        if (remaining == 0) {

            /* Abort if error occurs during flush */
            if (guac_socket_fd_flush(socket))
                return -1;

            /* Retry buffer append */
            continue;

        }

        /* Calculate size of chunk to be written to buffer */
        chunk_size = count;
        if (chunk_size > remaining)
            chunk_size = remaining;

        /* Update output buffer */
        memcpy(data->out_buf + data->written, current, chunk_size);
        data->written += chunk_size;

        /* Update provided buffer */
        current += chunk_size;
        count   -= chunk_size;

    }

    /* All bytes have been written, possibly some to the internal buffer */
    return original_count;

}

static ssize_t guac_socket_fd_write_handler(guac_socket* socket,
        const void* buf, size_t count) {

    int retval;
    guac_socket_fd_data* data = (guac_socket_fd_data*) socket->data;
    
    /* Acquire exclusive access to buffer */
    pthread_mutex_lock(&(data->buffer_lock));

    /* Write provided data to buffer */
    retval = guac_socket_fd_write_buffered(socket, buf, count);

    /* Relinquish exclusive access to buffer */
    pthread_mutex_unlock(&(data->buffer_lock));

    return retval;

}

static int guac_socket_fd_select_handler(guac_socket* socket,
        int usec_timeout) {

    guac_socket_fd_data* data = (guac_socket_fd_data*) socket->data;

    fd_set fds;
    struct timeval timeout;
    int retval;

    /* Initialize fd_set with single underlying file descriptor */
    FD_ZERO(&fds);
    FD_SET(data->fd, &fds);

    /* No timeout if usec_timeout is negative */
    if (usec_timeout < 0)
        retval = select(data->fd + 1, &fds, NULL, NULL, NULL); 

    /* Handle timeout if specified */
    else {
        timeout.tv_sec = usec_timeout/1000000;
        timeout.tv_usec = usec_timeout%1000000;
        retval = select(data->fd + 1, &fds, NULL, NULL, &timeout);
    }

    /* Properly set guac_error */
    if (retval <  0) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Error while waiting for data on socket";
    }

    if (retval == 0) {
        guac_error = GUAC_STATUS_TIMEOUT;
        guac_error_message = "Timeout while waiting for data on socket";
    }

    return retval;

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
static int guac_socket_fd_free_handler(guac_socket* socket) {

    guac_socket_fd_data* data = (guac_socket_fd_data*) socket->data;

    /* Destroy locks */
    pthread_mutex_destroy(&(data->socket_lock));
    pthread_mutex_destroy(&(data->buffer_lock));

    /* Close file descriptor */
    close(data->fd);

    free(data);
    return 0;

}

/**
 * Acquires exclusive access to the given socket.
 *
 * @param socket
 *     The guac_socket to which exclusive access is required.
 */
static void guac_socket_fd_lock_handler(guac_socket* socket) {

    guac_socket_fd_data* data = (guac_socket_fd_data*) socket->data;

    /* Acquire exclusive access to socket */
    pthread_mutex_lock(&(data->socket_lock));

}

/**
 * Relinquishes exclusive access to the given socket.
 *
 * @param socket
 *     The guac_socket to which exclusive access is no longer required.
 */
static void guac_socket_fd_unlock_handler(guac_socket* socket) {

    guac_socket_fd_data* data = (guac_socket_fd_data*) socket->data;

    /* Relinquish exclusive access to socket */
    pthread_mutex_unlock(&(data->socket_lock));

}

guac_socket* guac_socket_open(int fd) {

    pthread_mutexattr_t lock_attributes;

    /* Allocate socket and associated data */
    guac_socket* socket = guac_socket_alloc();
    guac_socket_fd_data* data = malloc(sizeof(guac_socket_fd_data));

    /* Store file descriptor as socket data */
    data->fd = fd;
    data->written = 0;
    socket->data = data;

    pthread_mutexattr_init(&lock_attributes);
    pthread_mutexattr_setpshared(&lock_attributes, PTHREAD_PROCESS_SHARED);

    /* Init locks */
    pthread_mutex_init(&(data->socket_lock), &lock_attributes);
    pthread_mutex_init(&(data->buffer_lock), &lock_attributes);
    
    /* Set read/write handlers */
    socket->read_handler   = guac_socket_fd_read_handler;
    socket->write_handler  = guac_socket_fd_write_handler;
    socket->select_handler = guac_socket_fd_select_handler;
    socket->lock_handler   = guac_socket_fd_lock_handler;
    socket->unlock_handler = guac_socket_fd_unlock_handler;
    socket->flush_handler  = guac_socket_fd_flush_handler;
    socket->free_handler   = guac_socket_fd_free_handler;

    return socket;

}

