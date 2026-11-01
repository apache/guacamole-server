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

#ifndef GUAC_IPMI_CONTROL_H
#define GUAC_IPMI_CONTROL_H

#include <guacamole/user.h>

#include <stdbool.h>

/**
 * The name of the client-agnostic out-of-band control/status channel. A client
 * opens an inbound pipe stream with this name to send JSON commands, and the
 * server pushes JSON state/result/event messages back on outbound pipe streams
 * of the same name. This keeps chassis control and telemetry off the serial
 * console stream so any client (browser panel, native TUI) can render it
 * natively. The in-terminal Ctrl+] menu remains as a portable fallback.
 *
 * Inbound (client -> server) messages are JSON objects, each terminated by a
 * newline, written to the client-opened pipe stream. A single inbound stream
 * may carry any number of commands:
 *
 *     {"id":"<opaque>","type":"command","command":"power-cycle"}
 *
 * Outbound (server -> client) messages are sent one JSON object per pipe
 * stream, without a trailing newline; each outbound stream is ended right
 * after its single message:
 *
 *     {"type":"state","power":"on","health":"sol-connected"}
 *     {"type":"state","health":"sol-disconnected"}
 *     {"id":"<opaque>","type":"result","ok":true,"message":"..."}
 *     {"type":"sel","text":"..."}
 *     {"type":"sel","error":"..."}
 *
 * A "state" message omits "power" when the power state was not queried, in
 * which case the client should keep its last-known value.
 */
#define GUAC_IPMI_CONTROL_PIPE_NAME "ipmi-control"

/**
 * The maximum size, in bytes, of a single inbound control message (one line of
 * newline-delimited JSON), including its null terminator. Longer lines are
 * discarded in their entirety.
 */
#define GUAC_IPMI_CONTROL_BUFFER_LENGTH 4096

/**
 * Per-inbound-stream reassembly state for the control pipe.
 */
typedef struct guac_ipmi_control_stream {

    /**
     * Buffer accumulating the current (possibly partial) inbound JSON line.
     */
    char buffer[GUAC_IPMI_CONTROL_BUFFER_LENGTH];

    /**
     * The number of bytes currently held in the buffer.
     */
    int length;

    /**
     * Whether the current line has overflowed the buffer, in which case all
     * bytes up to and including the next newline are skipped rather than
     * buffered.
     */
    bool discarding;

} guac_ipmi_control_stream;

/**
 * Handler invoked by guac_ipmi_control_feed() for each complete inbound line.
 *
 * @param line
 *     The complete, null-terminated line, excluding its newline. The buffer
 *     is reused once the handler returns.
 *
 * @param data
 *     The arbitrary data passed to guac_ipmi_control_feed().
 */
typedef void guac_ipmi_control_line_handler(const char* line, void* data);

/**
 * Pushes the current control-channel state to all currently-connected users of
 * the given client. The chassis power state is not actively queried.
 *
 * @param client
 *     The client whose users should receive the state update.
 */
void guac_ipmi_control_broadcast_state(guac_client* client);

/**
 * Sets up an inbound "ipmi-control" pipe stream just opened by the given user,
 * installing the blob/end handlers that parse and dispatch control commands,
 * and pushing an initial state message to the user.
 *
 * @param user
 *     The user which opened the control pipe.
 *
 * @param stream
 *     The inbound pipe stream.
 */
void guac_ipmi_control_open(guac_user* user, guac_stream* stream);

/**
 * Appends the given bytes to the reassembly state of an inbound control pipe
 * stream, invoking the given handler for each complete, non-empty line as its
 * terminating newline arrives. Lines may span any number of calls. A line too
 * long to fit within GUAC_IPMI_CONTROL_BUFFER_LENGTH is discarded entirely, up
 * to and including its newline, so that no fragment of it is ever passed to
 * the handler.
 *
 * Exposed (rather than file-static) so the framing can be unit-tested.
 *
 * @param cs
 *     The reassembly state of the inbound stream.
 *
 * @param data
 *     The bytes received.
 *
 * @param length
 *     The number of bytes received.
 *
 * @param handler
 *     The handler to invoke for each complete line.
 *
 * @param handler_data
 *     Arbitrary data to pass to the handler.
 */
void guac_ipmi_control_feed(guac_ipmi_control_stream* cs, const char* data,
        int length, guac_ipmi_control_line_handler* handler,
        void* handler_data);

/**
 * Escapes the given string into the body of a JSON string (without the
 * surrounding quotes), writing at most dstlen-1 bytes plus a NUL terminator.
 * Control characters other than the standard JSON escapes are dropped.
 *
 * Exposed (rather than file-static) so the parser can be unit-tested.
 *
 * @param dst
 *     The buffer to which the escaped string should be written.
 *
 * @param dstlen
 *     The size of dst, in bytes.
 *
 * @param src
 *     The null-terminated string to escape.
 */
void guac_ipmi_control_json_escape(char* dst, int dstlen, const char* src);

/**
 * Extracts the value of the given key from a flat JSON object into out. The
 * key is matched only when used as an object key (followed by a colon), so an
 * identical token appearing as a value is not matched.
 *
 * Quoted string values are unescaped as they are copied. Bare scalars (numbers,
 * true, false, and null) are copied literally, as clients are free to send
 * them. Object and array values are not supported and yield no match.
 *
 * Exposed (rather than file-static) so the parser can be unit-tested.
 *
 * @param json
 *     The null-terminated flat JSON object to search.
 *
 * @param key
 *     The key whose value should be extracted.
 *
 * @param out
 *     The buffer to which the extracted value should be written.
 *
 * @param outlen
 *     The size of out, in bytes.
 *
 * @return
 *     Non-zero if the key was found and its value written to out, zero
 *     otherwise.
 */
int guac_ipmi_control_json_get(const char* json, const char* key,
        char* out, int outlen);

#endif
