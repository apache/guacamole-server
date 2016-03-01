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

#ifndef _GUAC_CLIENT_CONSTANTS_H
#define _GUAC_CLIENT_CONSTANTS_H

/**
 * Constants related to the Guacamole client structure, guac_client.
 *
 * @file client-constants.h
 */

/**
 * The maximum number of inbound or outbound streams supported by any one
 * guac_client.
 */
#define GUAC_CLIENT_MAX_STREAMS 64

/**
 * The index of a closed stream.
 */
#define GUAC_CLIENT_CLOSED_STREAM_INDEX -1

/**
 * The character prefix which identifies a client ID.
 */
#define GUAC_CLIENT_ID_PREFIX '$'

/**
 * The flag set in the mouse button mask when the left mouse button is down.
 */
#define GUAC_CLIENT_MOUSE_LEFT 0x01

/**
 * The flag set in the mouse button mask when the middle mouse button is down.
 */
#define GUAC_CLIENT_MOUSE_MIDDLE 0x02

/**
 * The flag set in the mouse button mask when the right mouse button is down.
 */
#define GUAC_CLIENT_MOUSE_RIGHT 0x04

/**
 * The flag set in the mouse button mask when the mouse scrollwheel is scrolled
 * up. Note that mouse scrollwheels are actually sets of two buttons. One
 * button is pressed and released for an upward scroll, and the other is
 * pressed and released for a downward scroll. Some mice may actually implement
 * these as separate buttons, not a wheel.
 */
#define GUAC_CLIENT_MOUSE_SCROLL_UP 0x08

/**
 * The flag set in the mouse button mask when the mouse scrollwheel is scrolled
 * down. Note that mouse scrollwheels are actually sets of two buttons. One
 * button is pressed and released for an upward scroll, and the other is
 * pressed and released for a downward scroll. Some mice may actually implement
 * these as separate buttons, not a wheel.
 */
#define GUAC_CLIENT_MOUSE_SCROLL_DOWN 0x10

/**
 * The minimum number of buffers to create before allowing free'd buffers to
 * be reclaimed. In the case a protocol rapidly creates, uses, and destroys
 * buffers, this can prevent unnecessary reuse of the same buffer (which
 * would make draw operations unnecessarily synchronous).
 */
#define GUAC_BUFFER_POOL_INITIAL_SIZE 1024

#endif

