/*
 * Copyright (C) 2014 Glyptodon LLC
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

#ifndef GUAC_TELNET__GUAC_HANDLERS_H
#define GUAC_TELNET__GUAC_HANDLERS_H

#include "config.h"

#include <guacamole/client.h>

/**
 * Generic handler for sending outbound messages. Required by libguac and
 * called periodically by guacd when the client is ready for more graphical
 * updates.
 */
int guac_telnet_client_handle_messages(guac_client* client);

/**
 * Handler for key events. Required by libguac and called whenever key events
 * are received.
 */
int guac_telnet_client_key_handler(guac_client* client, int keysym, int pressed);

/**
 * Handler for mouse events. Required by libguac and called whenever mouse
 * events are received.
 */
int guac_telnet_client_mouse_handler(guac_client* client, int x, int y, int mask);

/**
 * Handler for size events. Required by libguac and called whenever the remote
 * display (window) is resized.
 */
int guac_telnet_client_size_handler(guac_client* client, int width, int height);

/**
 * Free handler. Required by libguac and called when the guac_client is
 * disconnected and must be cleaned up.
 */
int guac_telnet_client_free_handler(guac_client* client);

#endif

