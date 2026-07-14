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

#ifndef GUAC_SERIAL_TCP_H
#define GUAC_SERIAL_TCP_H

#include "settings.h"
#include "stream.h"

#include <guacamole/client.h>

/**
 * Opens a raw TCP connection to the network serial server described by the
 * given settings (e.g. ser2net operating in raw mode) and stores the resulting
 * socket in the given stream. On failure, the client is aborted.
 *
 * @param stream
 *     The serial stream to populate with the opened socket.
 *
 * @param client
 *     The client for which the connection is being opened.
 *
 * @param settings
 *     The parsed connection settings describing the server to connect to.
 *
 * @return
 *     Zero on success, non-zero on failure.
 */
int guac_serial_tcp_open(guac_serial_stream* stream, guac_client* client,
        guac_serial_settings* settings);

#endif
