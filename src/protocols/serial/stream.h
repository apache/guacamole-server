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

#ifndef GUAC_SERIAL_STREAM_H
#define GUAC_SERIAL_STREAM_H

#include "settings.h"

#include <guacamole/client.h>

#include <pthread.h>

/**
 * The transport backend providing the serial byte stream.
 */
typedef enum guac_serial_backend {

    /**
     * A local serial device opened directly via a device node.
     */
    GUAC_SERIAL_BACKEND_LOCAL,

    /**
     * A raw TCP connection to a remote serial server (e.g. ser2net in raw
     * mode).
     */
    GUAC_SERIAL_BACKEND_TCP,

    /**
     * An RFC2217 (telnet COM-PORT-OPTION) connection to a remote serial server.
     */
    GUAC_SERIAL_BACKEND_RFC2217

} guac_serial_backend;

/**
 * A transport-neutral, bidirectional serial byte stream. All reads are
 * performed directly on the file descriptor by the main worker thread; writes
 * and break signalling are dispatched to the appropriate backend and
 * serialized with an internal lock.
 */
typedef struct guac_serial_stream {

    /**
     * The transport backend providing this stream.
     */
    guac_serial_backend backend;

    /**
     * The device file descriptor (local) or socket file descriptor (tcp,
     * rfc2217), or -1 if not open.
     */
    int fd;

    /**
     * The libtelnet session (telnet_t*) used for IAC framing when the backend
     * is GUAC_SERIAL_BACKEND_RFC2217, or NULL otherwise. Declared as an opaque
     * pointer so this header does not depend on libtelnet, which is required
     * only by the RFC2217 backend (see rfc2217.c). Only the RFC2217 backend
     * dereferences this value.
     */
    void* telnet;

    /**
     * Lock serializing writes and break signalling on this stream.
     */
    pthread_mutex_t write_lock;

    /**
     * The delay, in milliseconds, inserted between each byte written to the
     * serial line. Zero disables pacing. Copied from settings.
     */
    int paste_delay;

    /**
     * The duration, in milliseconds, that the transmit line is held low when
     * sending a serial break. Copied from settings.
     */
    int break_duration;

    /**
     * The client associated with this stream, used for logging.
     */
    guac_client* client;

} guac_serial_stream;

/**
 * Opens a serial byte stream using the transport described by the given
 * settings, dispatching to the local, TCP, or RFC2217 backend as appropriate.
 * On failure, the client is aborted and NULL is returned.
 *
 * @param client
 *     The client for which the stream is being opened.
 *
 * @param settings
 *     The parsed connection settings describing the transport to use.
 *
 * @return
 *     A newly-allocated serial stream which must be freed with
 *     guac_serial_stream_close(), or NULL if the connection could not be
 *     established.
 */
guac_serial_stream* guac_serial_stream_open(guac_client* client,
        guac_serial_settings* settings);

/**
 * Writes the given buffer to the serial stream, blocking until the entire
 * buffer has been written or an unrecoverable error occurs. If a non-zero
 * paste delay is configured, bytes are written one at a time with the
 * configured delay inserted between each.
 *
 * @param stream
 *     The serial stream to write to.
 *
 * @param buffer
 *     The buffer of data to write.
 *
 * @param length
 *     The number of bytes from the buffer to write.
 *
 * @return
 *     The number of bytes written on success, or a negative value if an error
 *     prevents all future writes.
 */
int guac_serial_stream_write(guac_serial_stream* stream, const char* buffer,
        int length);

/**
 * Sends a serial break on the serial stream. For local and RFC2217 transports,
 * the transmit line is held low for the configured break duration. Raw TCP
 * transports do not support break signalling; a warning is logged and the call
 * is a no-op.
 *
 * @param stream
 *     The serial stream on which to send the break.
 *
 * @return
 *     Zero on success, non-zero on error.
 */
int guac_serial_stream_send_break(guac_serial_stream* stream);

/**
 * Closes the given serial stream, releasing all associated resources. This
 * function is idempotent and safe to call with a NULL stream.
 *
 * @param stream
 *     The serial stream to close.
 */
void guac_serial_stream_close(guac_serial_stream* stream);

#endif
