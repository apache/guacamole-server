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

#ifndef GUAC_SERIAL_RFC2217_H
#define GUAC_SERIAL_RFC2217_H

#include "settings.h"
#include "stream.h"

#include <guacamole/client.h>

/*
 * This header intentionally does not depend on libtelnet. All libtelnet usage
 * is confined to rfc2217.c and compiled only when ENABLE_SERIAL_RFC2217 is
 * defined (i.e. libtelnet is available). When it is not, the functions below
 * are still defined, but fail or become no-ops, so the rest of the plugin
 * links and runs (with the RFC2217 transport unavailable).
 */

/**
 * Opens an RFC2217 (telnet COM-PORT-OPTION) connection to the network serial
 * server described by the given settings, negotiates the configured serial line
 * parameters, and stores the resulting socket and libtelnet session in the
 * given stream. On failure, the client is aborted.
 *
 * If RFC2217 support was not compiled in, the client is aborted and -1 is
 * returned.
 *
 * @param stream
 *     The serial stream to populate with the opened socket and session.
 *
 * @param client
 *     The client for which the connection is being opened.
 *
 * @param settings
 *     The parsed connection settings describing the server to connect to and
 *     the serial line parameters to negotiate.
 *
 * @return
 *     Zero on success, non-zero on failure.
 */
int guac_serial_rfc2217_open(guac_serial_stream* stream, guac_client* client,
        guac_serial_settings* settings);

/**
 * Sends the given buffer to the remote end of an RFC2217 connection, applying
 * telnet IAC framing. This must only be called on a stream whose backend is
 * GUAC_SERIAL_BACKEND_RFC2217.
 *
 * @param stream
 *     The RFC2217 serial stream to write to.
 *
 * @param buffer
 *     The buffer of data to send.
 *
 * @param length
 *     The number of bytes from the buffer to send.
 */
void guac_serial_rfc2217_send(guac_serial_stream* stream, const char* buffer,
        int length);

/**
 * Feeds the given buffer of bytes received from the remote end into the
 * RFC2217 libtelnet parser, which will de-frame the data and write terminal
 * output. This must only be called on a stream whose backend is
 * GUAC_SERIAL_BACKEND_RFC2217.
 *
 * @param stream
 *     The RFC2217 serial stream that received the data.
 *
 * @param buffer
 *     The buffer of received data.
 *
 * @param length
 *     The number of bytes within the buffer.
 */
void guac_serial_rfc2217_recv(guac_serial_stream* stream, const char* buffer,
        int length);

/**
 * Sends a serial break to the remote end of an RFC2217 connection by holding
 * the transmit line low for the stream's configured break duration using
 * COM-PORT-OPTION SET-CONTROL subnegotiations. This must only be called on a
 * stream whose backend is GUAC_SERIAL_BACKEND_RFC2217.
 *
 * @param stream
 *     The RFC2217 serial stream on which to send the break.
 *
 * @return
 *     Zero on success, non-zero on error.
 */
int guac_serial_rfc2217_send_break(guac_serial_stream* stream);

/**
 * Frees the libtelnet session associated with the given stream, if any. Safe
 * to call regardless of backend; has no effect if no session is present.
 *
 * @param stream
 *     The serial stream whose libtelnet session should be freed.
 */
void guac_serial_rfc2217_free(guac_serial_stream* stream);

#endif
