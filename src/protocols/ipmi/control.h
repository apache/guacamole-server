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

/**
 * The name of the client-agnostic out-of-band control/status channel. A client
 * opens an inbound pipe stream with this name to send JSON commands, and the
 * server pushes JSON state/result/event messages back on outbound pipe streams
 * of the same name. This keeps chassis control and telemetry off the serial
 * console stream so any client (browser panel, native TUI) can render it
 * natively. The in-terminal Ctrl+] menu remains as a portable fallback.
 *
 * Messages are newline-delimited JSON objects. Inbound (client -> server):
 *
 *     {"id":"<opaque>","type":"command","command":"power-cycle"}
 *
 * Outbound (server -> client):
 *
 *     {"type":"state","power":"on","identify":false,"health":"sol-connected"}
 *     {"id":"<opaque>","type":"result","ok":true,"message":"..."}
 *     {"type":"sel","total":66,"text":"..."}
 */
#define GUAC_IPMI_CONTROL_PIPE_NAME "ipmi-control"

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

#endif
