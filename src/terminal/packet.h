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

