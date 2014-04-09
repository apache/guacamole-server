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

#ifndef _GUAC_SOCKET_TYPES_H
#define _GUAC_SOCKET_TYPES_H

/**
 * Type definitions related to the guac_socket object.
 *
 * @file socket-types.h
 */

/**
 * The core I/O object of Guacamole. guac_socket provides buffered input and
 * output as well as convenience methods for efficiently writing base64 data.
 */
typedef struct guac_socket guac_socket;

/**
 * Possible current states of a guac_socket.
 */
typedef enum guac_socket_state {

    /**
     * The socket is open and can be written to / read from.
     */
    GUAC_SOCKET_OPEN,

    /**
     * The socket is closed. Reads and writes will fail.
     */
    GUAC_SOCKET_CLOSED

} guac_socket_state;

#endif

