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


#ifndef LIBGUACD_USER_H
#define LIBGUACD_USER_H

#include <guacamole/parser.h>
#include <guacamole/socket.h>
#include <guacamole/user.h>

/**
 * The number of milliseconds to wait for messages in any phase before
 * timing out and closing the connection with an error.
 */
#define GUACD_TIMEOUT      15000

/**
 * The number of microseconds to wait for messages in any phase before
 * timing out and closing the conncetion with an error. This is always
 * equal to GUACD_TIMEOUT * 1000.
 */
#define GUACD_USEC_TIMEOUT (GUACD_TIMEOUT*1000)

/**
 * The maximum number of concurrent connections to a single instance
 * of guacd.
 */
#define GUACD_CLIENT_MAX_CONNECTIONS 65536

/**
 * Handles the initial handshake of a user and all subsequent I/O. This
 * function blocks until the user disconnects.
 *
 * @param user
 *     The user whose handshake and entire Guacamole protocol exchange should
 *     be handled.
 *
 * @return
 *     Zero if the user's Guacamole connection was successfully handled and
 *     the user has disconnected, or non-zero if an error prevented the user's
 *     connection from being handled properly.
 */
int guacd_handle_user(guac_user* user);

#endif
