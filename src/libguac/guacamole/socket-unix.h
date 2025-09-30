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

#ifndef __GUAC_SOCKET_UNIX_H
#define __GUAC_SOCKET_UNIX_H

#include "config.h"

#include <stddef.h>

/**
 * Given a path to a UNIX socket, attempt to connect to that socket, returning
 * the open socket if the connection succeeds, or a negative value if it fails.
 * If it fails the errno variable will be set.
 *
 * @param path
 *     The path to the UNIX socket. If the path begins with a slash, it will
 *     be interpreted as an absolute path to the socket on the system running
 *     guacd. If the path begins with anything but a slash, it will be a path
 *     relative to the working directory from which guacd was started.
 *
 * @return
 *     A valid socket if the connection succeeds, or a negative integer if it
 *     fails.
 */
int guac_socket_unix_connect(const char* path);

#endif // __GUAC_SOCKET_UNIX_H