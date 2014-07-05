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


#ifndef _GUACD_CLIENT_H
#define _GUACD_CLIENT_H

#include "config.h"

#include <guacamole/client.h>

/**
 * The time to allow between sync responses in milliseconds. If a sync
 * instruction is sent to the client and no response is received within this
 * timeframe, server messages will not be handled until a sync instruction is
 * received from the client.
 */
#define GUACD_SYNC_THRESHOLD 500

/**
 * The time to allow between server sync messages in milliseconds. A sync
 * message from the server will be sent every GUACD_SYNC_FREQUENCY milliseconds.
 * As this will induce a response from a client that is not malfunctioning,
 * this is used to detect when a client has died. This must be set to a
 * reasonable value to avoid clients being disconnected unnecessarily due
 * to timeout.
 */
#define GUACD_SYNC_FREQUENCY 5000

/**
 * The amount of time to wait after handling server messages. If a client
 * plugin has a message handler, and sends instructions when server messages
 * are being handled, there will be a pause of this many milliseconds before
 * the next call to the message handler.
 */
#define GUACD_MESSAGE_HANDLE_FREQUENCY 50

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

int guacd_client_start(guac_client* client);

#endif

