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

#ifndef __GUAC_SOCKET_TCP_H
#define __GUAC_SOCKET_TCP_H

#include "config.h"

#include <stddef.h>

/**
 * Given a hostname or IP address and port, attempt to connect to that
 * system, returning an open socket if the connection succeeds, or a negative
 * value if it fails. If it fails the errno variable will be set.
 *
 * @param hostname
 *     The hostname or IP address to which to attempt connections.
 *
 * @param port
 *     The TCP port to which to attempt to connect.
 *
 * @param timeout
 *     The number of seconds to try the TCP connection before timing out.
 *
 * @return
 *     A valid socket if the connection succeeds, or a negative integer if it
 *     fails.
 */
int guac_socket_tcp_connect(const char* hostname, const char* port, const int timeout);

#endif // __GUAC_SOCKET_TCP_H