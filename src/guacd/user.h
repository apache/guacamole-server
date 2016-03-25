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


#ifndef _GUACD_USER_H
#define _GUACD_USER_H

#include "config.h"

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
 * Parameters required by the user input thread.
 */
typedef struct guacd_user_input_thread_params {

    /**
     * The parser which will be used throughout the user's session.
     */
    guac_parser* parser;

    /**
     * A reference to the connected user.
     */
    guac_user* user;

} guacd_user_input_thread_params;

/**
 * Starts the input/output threads of a new user. This function will block
 * until the user disconnects. If an error prevents the input/output threads
 * from starting, guac_user_stop() will be invoked on the given user.
 *
 * @param parser
 *     The guac_parser to use to handle all input from the given user.
 *
 * @param user
 *     The user whose associated I/O transfer threads should be started.
 *
 * @return
 *     Zero if the I/O threads started successfully and user has disconnected,
 *     or non-zero if the I/O threads could not be started.
 */
int guacd_user_start(guac_parser* parser, guac_user* user);

/**
 * The thread which handles all user input, calling event handlers for received
 * instructions.
 *
 * @param data
 *     A pointer to a guacd_user_input_thread_params structure describing the
 *     user whose input is being handled and the guac_parser with which to
 *     handle it.
 *
 * @return
 *     Always NULL.
 */
void* guacd_user_input_thread(void* data);

#endif

