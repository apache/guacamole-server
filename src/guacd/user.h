/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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

