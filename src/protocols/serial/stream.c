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

#include "local.h"
#include "rfc2217.h"
#include "settings.h"
#include "stream.h"
#include "tcp.h"

#include <guacamole/client.h>
#include <guacamole/error.h>
#include <guacamole/mem.h>

#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/**
 * Writes the entire buffer given to the specified file descriptor, retrying
 * the write automatically if necessary.
 *
 * @param fd
 *     The file descriptor to write to.
 *
 * @param buffer
 *     The buffer to write.
 *
 * @param size
 *     The number of bytes from the buffer to write.
 *
 * @return
 *     The number of bytes written, or a value not equal to size if an error
 *     prevents all future writes.
 */
static int guac_serial_stream_write_all(int fd, const char* buffer, int size) {

    int remaining = size;
    while (remaining > 0) {

        int ret_val;
        GUAC_RETRY_EINTR(ret_val, write(fd, buffer, remaining));
        if (ret_val <= 0)
            return -1;

        remaining -= ret_val;
        buffer += ret_val;

    }

    return size;

}

guac_serial_stream* guac_serial_stream_open(guac_client* client,
        guac_serial_settings* settings) {

    guac_serial_stream* stream = guac_mem_zalloc(sizeof(guac_serial_stream));
    stream->client = client;
    stream->fd = -1;
    stream->telnet = NULL;
    stream->paste_delay = settings->paste_delay;
    stream->break_duration = settings->break_duration;
    pthread_mutex_init(&stream->write_lock, NULL);

    /* Dispatch to the appropriate backend */
    int result;
    if (settings->type == GUAC_SERIAL_TYPE_LOCAL)
        result = guac_serial_local_open(stream, client, settings);
    else if (settings->network_protocol == GUAC_SERIAL_NETWORK_PROTOCOL_RFC2217)
        result = guac_serial_rfc2217_open(stream, client, settings);
    else
        result = guac_serial_tcp_open(stream, client, settings);

    /* Clean up and return NULL on failure (backend has already aborted) */
    if (result != 0) {
        pthread_mutex_destroy(&stream->write_lock);
        guac_mem_free(stream);
        return NULL;
    }

    return stream;

}

int guac_serial_stream_write(guac_serial_stream* stream, const char* buffer,
        int length) {

    int result = length;

    pthread_mutex_lock(&stream->write_lock);

    /* Pace output byte-by-byte when a paste delay is configured */
    if (stream->paste_delay > 0) {

        for (int i = 0; i < length; i++) {

            if (stream->backend == GUAC_SERIAL_BACKEND_RFC2217)
                guac_serial_rfc2217_send(stream, buffer + i, 1);
            else if (guac_serial_stream_write_all(stream->fd, buffer + i, 1) != 1) {
                result = -1;
                break;
            }

            usleep((useconds_t) stream->paste_delay * 1000);

        }

    }

    /* Otherwise, write the entire buffer at once */
    else {

        if (stream->backend == GUAC_SERIAL_BACKEND_RFC2217)
            guac_serial_rfc2217_send(stream, buffer, length);
        else if (guac_serial_stream_write_all(stream->fd, buffer, length) != length)
            result = -1;

    }

    pthread_mutex_unlock(&stream->write_lock);
    return result;

}

int guac_serial_stream_send_break(guac_serial_stream* stream) {

    int result = 0;

    pthread_mutex_lock(&stream->write_lock);

    switch (stream->backend) {

        case GUAC_SERIAL_BACKEND_LOCAL:
#if defined(TIOCSBRK) && defined(TIOCCBRK)
            /* Hold the transmit line low for the configured duration */
            if (ioctl(stream->fd, TIOCSBRK) == 0) {
                if (stream->break_duration > 0)
                    usleep((useconds_t) stream->break_duration * 1000);
                ioctl(stream->fd, TIOCCBRK);
            }

            /* Fall back to tcsendbreak() if TIOCSBRK is unsupported */
            else
                tcsendbreak(stream->fd, 0);
#else
            tcsendbreak(stream->fd, 0);
#endif
            break;

        case GUAC_SERIAL_BACKEND_TCP:
            guac_client_log(stream->client, GUAC_LOG_WARNING, "Sending a serial "
                    "break is not supported on the raw TCP transport; use "
                    "rfc2217 instead.");
            break;

        case GUAC_SERIAL_BACKEND_RFC2217:
            guac_serial_rfc2217_send_break(stream);
            break;

    }

    pthread_mutex_unlock(&stream->write_lock);
    return result;

}

void guac_serial_stream_close(guac_serial_stream* stream) {

    /* Nothing to do if never opened */
    if (stream == NULL)
        return;

    /* Free the libtelnet session, if any */
    guac_serial_rfc2217_free(stream);

    /* Close the underlying file descriptor, if open */
    if (stream->fd != -1) {
        close(stream->fd);
        stream->fd = -1;
    }

    pthread_mutex_destroy(&stream->write_lock);
    guac_mem_free(stream);

}
