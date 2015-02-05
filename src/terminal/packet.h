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

#ifndef GUAC_TERMINAL_PACKET_H
#define GUAC_TERMINAL_PACKET_H

/**
 * The maximum size of a packet written or read by the
 * guac_terminal_packet_write() or guac_terminal_packet_read() functions.
 */
#define GUAC_TERMINAL_PACKET_SIZE 4096

/**
 * An arbitrary data packet with minimal framing.
 */
typedef struct guac_terminal_packet {

    /**
     * The number of bytes in the data portion of this packet.
     */
    int length;

    /**
     * Arbitrary data.
     */
    char data[GUAC_TERMINAL_PACKET_SIZE];

} guac_terminal_packet;

/**
 * Writes a single packet of data to the given file descriptor. The provided
 * length MUST be no greater than GUAC_TERMINAL_PACKET_SIZE. Zero-length
 * writes are legal and do result in a packet being written to the file
 * descriptor.
 *
 * @param fd
 *     The file descriptor to write to.
 *
 * @param data
 *     A buffer containing the data to write.
 *
 * @param length
 *     The number of bytes to write to the file descriptor.
 *
 * @return
 *     The number of bytes written on success, which may be zero if the data
 *     length is zero, or a negative value on error.
 */
int guac_terminal_packet_write(int fd, const void* data, int length);

/**
 * Reads a single packet of data from the given file descriptor. The provided
 * length MUST be at least GUAC_TERMINAL_PACKET_SIZE to ensure any packet
 * read will fit in the buffer. Zero-length reads are possible if a zero-length
 * packet was written.
 *
 * @param fd
 *     The file descriptor to read from.
 *
 * @param data
 *     The buffer to store data within.
 *
 * @param length
 *     The number of bytes available within the buffer.
 *
 * @return
 *     The number of bytes read on success, which may be zero if the read
 *     packet had a length of zero, or a negative value on error.
 */
int guac_terminal_packet_read(int fd, void* data, int length);

#endif

