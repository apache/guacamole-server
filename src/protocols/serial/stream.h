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
    GUAC_SERIAL_BACKEND_RFC2217,

    /**
     * A base RFC854 Telnet connection to a remote serial server, using Telnet
     * IAC framing without the RFC2217 COM-PORT-OPTION.
     */
    GUAC_SERIAL_BACKEND_TELNET

} guac_serial_backend;

/**
 * A modem control line which may be asserted or de-asserted on the serial
 * stream.
 */
typedef enum guac_serial_control_line {

    /**
     * The Data Terminal Ready (DTR) line.
     */
    GUAC_SERIAL_CONTROL_LINE_DTR,

    /**
     * The Request To Send (RTS) line.
     */
    GUAC_SERIAL_CONTROL_LINE_RTS

} guac_serial_control_line;

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

    /**
     * The guac_protocol_status describing the most recent failed open attempt.
     * Used by the initial connect path to abort with an appropriate status.
     */
    int open_status;

} guac_serial_stream;

/**
 * Allocates a serial byte stream for the given client, without yet opening any
 * transport. The returned stream has no open file descriptor (fd is -1) until
 * guac_serial_stream_reopen() succeeds. This allows a single stream object to
 * persist across reconnects for the lifetime of the session.
 *
 * @param client
 *     The client for which the stream is being allocated.
 *
 * @param settings
 *     The parsed connection settings describing the transport to use.
 *
 * @return
 *     A newly-allocated serial stream which must be freed with
 *     guac_serial_stream_close().
 */
guac_serial_stream* guac_serial_stream_alloc(guac_client* client,
        guac_serial_settings* settings);

/**
 * (Re)opens the transport for the given serial stream, closing any currently-
 * open file descriptor and libtelnet session first, then dispatching to the
 * local, TCP, or RFC2217 backend as appropriate. The stream's fd is swapped
 * under its write lock so that a concurrent writer never touches a stale
 * descriptor. On failure, the stream's open_status is set and the fd is left
 * at -1; the client is NOT aborted, so the caller may retry.
 *
 * @param stream
 *     The serial stream to (re)open.
 *
 * @param client
 *     The client for which the transport is being opened.
 *
 * @param settings
 *     The parsed connection settings describing the transport to use.
 *
 * @return
 *     Zero on success, non-zero on failure.
 */
int guac_serial_stream_reopen(guac_serial_stream* stream, guac_client* client,
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
 * Asserts or de-asserts a modem control line (DTR or RTS) on the serial stream.
 * For local transports the line is toggled via TIOCMBIS/TIOCMBIC; for RFC2217
 * transports the corresponding COM-PORT-OPTION SET-CONTROL subnegotiation is
 * sent. Raw TCP transports do not support control-line signalling; a warning
 * is logged and the call is a no-op.
 *
 * @param stream
 *     The serial stream on which to toggle the control line.
 *
 * @param line
 *     The control line to toggle (DTR or RTS).
 *
 * @param state
 *     true to assert (raise) the line, false to de-assert (lower) it.
 *
 * @return
 *     Zero on success, non-zero on error.
 */
int guac_serial_stream_set_line(guac_serial_stream* stream,
        guac_serial_control_line line, bool state);

/**
 * Closes the given serial stream, releasing all associated resources. This
 * function is idempotent and safe to call with a NULL stream.
 *
 * @param stream
 *     The serial stream to close.
 */
void guac_serial_stream_close(guac_serial_stream* stream);

#endif
