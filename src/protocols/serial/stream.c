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
#include <guacamole/protocol.h>

#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/**
 * The granularity, in milliseconds, of the interruptible sleep used to pace
 * byte-by-byte writes. Shorter slices make teardown more responsive during a
 * long paced paste; longer slices reduce wakeups.
 */
#define GUAC_SERIAL_PACING_SLICE 20

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

/**
 * Sleeps for the given number of milliseconds, returning early if the client
 * stops running. The wait is broken into short slices so that teardown is not
 * delayed by a long paced write.
 *
 * @param stream
 *     The serial stream whose client is checked for shutdown.
 *
 * @param milliseconds
 *     The number of milliseconds to sleep.
 */
static void guac_serial_stream_interruptible_sleep(guac_serial_stream* stream,
        int milliseconds) {

    while (milliseconds > 0 && stream->client->state == GUAC_CLIENT_RUNNING) {
        int slice = milliseconds < GUAC_SERIAL_PACING_SLICE
                ? milliseconds : GUAC_SERIAL_PACING_SLICE;
        usleep((useconds_t) slice * 1000);
        milliseconds -= slice;
    }

}

guac_serial_stream* guac_serial_stream_alloc(guac_client* client,
        guac_serial_settings* settings) {

    guac_serial_stream* stream = guac_mem_zalloc(sizeof(guac_serial_stream));
    stream->client = client;
    stream->fd = -1;
    stream->telnet = NULL;
    stream->paste_delay = settings->paste_delay;
    stream->break_duration = settings->break_duration;
    stream->open_status = GUAC_PROTOCOL_STATUS_SERVER_ERROR;
    pthread_mutex_init(&stream->write_lock, NULL);

    return stream;

}

int guac_serial_stream_reopen(guac_serial_stream* stream, guac_client* client,
        guac_serial_settings* settings) {

    pthread_mutex_lock(&stream->write_lock);

    /* Tear down any existing transport so the fd/telnet are never stale */
    guac_serial_rfc2217_free(stream);
    if (stream->fd != -1) {
        close(stream->fd);
        stream->fd = -1;
    }

    /* Dispatch to the appropriate backend. Backends set stream->fd on success,
     * or set stream->open_status and return non-zero on failure without
     * aborting the client (so the caller may retry). */
    int result;
    if (settings->type == GUAC_SERIAL_TYPE_LOCAL)
        result = guac_serial_local_open(stream, client, settings);
    else if (settings->network_protocol == GUAC_SERIAL_NETWORK_PROTOCOL_RFC2217)
        result = guac_serial_rfc2217_open(stream, client, settings);
    else if (settings->network_protocol == GUAC_SERIAL_NETWORK_PROTOCOL_TELNET)
        result = guac_serial_telnet_open(stream, client, settings);
    else
        result = guac_serial_tcp_open(stream, client, settings);

    /* Ensure the fd is left invalid on failure */
    if (result != 0)
        stream->fd = -1;

    pthread_mutex_unlock(&stream->write_lock);
    return result;

}

int guac_serial_stream_write(guac_serial_stream* stream, const char* buffer,
        int length) {

    pthread_mutex_lock(&stream->write_lock);

    /* Drop input while the transport is down (e.g. mid-reconnect) rather than
     * blocking or writing to a stale descriptor */
    if (stream->fd < 0) {
        pthread_mutex_unlock(&stream->write_lock);
        return length;
    }

    /* Pace output byte-by-byte when a paste delay is configured */
    if (stream->paste_delay > 0) {

        for (int i = 0; i < length; i++) {

            /* Bail promptly if the session is tearing down, so a long paste
             * cannot hold up teardown */
            if (stream->client->state != GUAC_CLIENT_RUNNING)
                break;

            if (stream->backend == GUAC_SERIAL_BACKEND_RFC2217
                    || stream->backend == GUAC_SERIAL_BACKEND_TELNET)
                guac_serial_rfc2217_send(stream, buffer + i, 1);
            else if (guac_serial_stream_write_all(stream->fd, buffer + i, 1) != 1)
                /* The descriptor went bad; the read loop will detect the drop
                 * and reconnect, so simply stop writing */
                break;

            guac_serial_stream_interruptible_sleep(stream, stream->paste_delay);

        }

    }

    /* Otherwise, write the entire buffer at once */
    else {

        if (stream->backend == GUAC_SERIAL_BACKEND_RFC2217
                || stream->backend == GUAC_SERIAL_BACKEND_TELNET)
            guac_serial_rfc2217_send(stream, buffer, length);
        else
            guac_serial_stream_write_all(stream->fd, buffer, length);

    }

    pthread_mutex_unlock(&stream->write_lock);
    return length;

}

int guac_serial_stream_send_break(guac_serial_stream* stream) {

    int result = 0;

    pthread_mutex_lock(&stream->write_lock);

    /* Ignore if the transport is currently down */
    if (stream->fd < 0) {
        pthread_mutex_unlock(&stream->write_lock);
        return 0;
    }

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

        case GUAC_SERIAL_BACKEND_TELNET:
            guac_serial_telnet_send_break(stream);
            break;

    }

    pthread_mutex_unlock(&stream->write_lock);
    return result;

}

int guac_serial_stream_set_line(guac_serial_stream* stream,
        guac_serial_control_line line, bool state) {

    int result = 0;
    const char* line_name = (line == GUAC_SERIAL_CONTROL_LINE_DTR)
            ? "DTR" : "RTS";

    pthread_mutex_lock(&stream->write_lock);

    /* Ignore if the transport is currently down */
    if (stream->fd < 0) {
        pthread_mutex_unlock(&stream->write_lock);
        return 0;
    }

    switch (stream->backend) {

        case GUAC_SERIAL_BACKEND_LOCAL: {
            int flag = (line == GUAC_SERIAL_CONTROL_LINE_DTR)
                    ? TIOCM_DTR : TIOCM_RTS;
            if (ioctl(stream->fd, state ? TIOCMBIS : TIOCMBIC, &flag) != 0) {
                guac_client_log(stream->client, GUAC_LOG_WARNING, "Unable to "
                        "set %s %s: %s", line_name, state ? "on" : "off",
                        strerror(errno));
                result = -1;
            }
            break;
        }

        case GUAC_SERIAL_BACKEND_TCP:
            guac_client_log(stream->client, GUAC_LOG_WARNING, "Control-line "
                    "toggles (DTR/RTS) require the rfc2217 transport; ignoring "
                    "on raw TCP transport.");
            break;

        case GUAC_SERIAL_BACKEND_RFC2217: {
            /* RFC2217 SET-CONTROL values: DTR ON=8, DTR OFF=9, RTS ON=11,
             * RTS OFF=12 */
            int value;
            if (line == GUAC_SERIAL_CONTROL_LINE_DTR)
                value = state ? 8 : 9;
            else
                value = state ? 11 : 12;
            guac_serial_rfc2217_set_control(stream, value);
            break;
        }

        case GUAC_SERIAL_BACKEND_TELNET:
            guac_client_log(stream->client, GUAC_LOG_WARNING, "Control-line "
                    "toggles (DTR/RTS) require the rfc2217 transport; ignoring "
                    "on telnet transport.");
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
