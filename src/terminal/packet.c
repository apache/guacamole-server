/*
 * Copyright (C) 2015 Glyptodon LLC
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

#include "common.h"
#include "packet.h"

#include <string.h>

int guac_terminal_packet_write(int fd, const void* data, int length) {

    guac_terminal_packet out;

    /* Do not attempt to write packets beyond maximum size */
    if (length > GUAC_TERMINAL_PACKET_SIZE)
        return -1;

    /* Calculate final packet length */
    int packet_length = sizeof(int) + length;

    /* Copy data into packet */
    out.length = length;
    memcpy(out.data, data, length);

    /* Write packet */
    return guac_terminal_write_all(fd, (const char*) &out, packet_length);

}

int guac_terminal_packet_read(int fd, void* data, int length) {

    int bytes;

    /* Read buffers MUST be at least GUAC_TERMINAL_PACKET_SIZE */
    if (length < GUAC_TERMINAL_PACKET_SIZE)
        return -1;

    /* Read length */
    if (guac_terminal_fill_buffer(fd, (char*) &bytes, sizeof(int)) < 0)
        return -1;

    /* Read body */
    if (guac_terminal_fill_buffer(fd, (char*) data, bytes) < 0)
        return -1;

    return bytes;

}

