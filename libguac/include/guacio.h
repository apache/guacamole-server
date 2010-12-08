
/*
 *  Guacamole - Clientless Remote Desktop
 *  Copyright (C) 2010  Michael Jumper
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _GUACIO_H
#define _GUACIO_H

#include <unistd.h>

/**
 * Defines the GUACIO object and functionss for using and manipulating it.
 *
 * @file guacio.h
 */


/**
 * The core I/O object of Guacamole. GUACIO provides buffered input and output
 * as well as convenience methods for efficiently writing base64 data.
 */
typedef struct GUACIO {

    /**
     * The file descriptor to be read from / written to.
     */
    int fd; 
    
    /**
     * The number of bytes present in the base64 "ready" buffer.
     */
    int ready;

    /**
     * The base64 "ready" buffer. Once this buffer is filled, base64 data is
     * flushed to the main write buffer.
     */
    int ready_buf[3];

    /**
     * The number of bytes currently in the main write buffer.
     */
    int written;

    /**
     * The main write buffer. Bytes written go here before being flushed
     * to the open file descriptor.
     */
    char out_buf[8192];

    /**
     * The current size of the instruction buffer.
     */
    int instructionbuf_size;

    /**
     * The number of bytes currently in the instruction buffer.
     */
    int instructionbuf_used_length;

    /**
     * The instruction buffer. This is essentially the input buffer,
     * provided as a convenience to be used to buffer instructions until
     * those instructions are complete and ready to be parsed.
     */
    char* instructionbuf;

    /**
     * The transfer limit, in kilobytes per second. If 0, there is no
     * transfer limit. If non-zero, sleep calls are used at the end of
     * a write to ensure output never exceeds the specified limit.
     */
    unsigned int transfer_limit; /* KB/sec */

} GUACIO;

/**
 * Allocates and initializes a new GUACIO object with the given open
 * file descriptor.
 *
 * @param fd An open file descriptor that this GUACIO object should manage.
 * @return A newly allocated GUACIO object associated with the given
 *         file descriptor.
 */
GUACIO* guac_open(int fd);

/**
 * Writes the given unsigned int to the given GUACIO object. The data
 * written may be buffered until the buffer is flushed automatically or
 * manually.
 *
 * @param io The GUACIO object to write to.
 * @param i The unsigned int to write.
 * @return Zero on success, or non-zero if an error occurs while writing.
 */
ssize_t guac_write_int(GUACIO* io, unsigned int i);

/**
 * Writes the given string to the given GUACIO object. The data
 * written may be buffered until the buffer is flushed automatically or
 * manually. Note that if the string can contain characters used
 * internally by the Guacamole protocol (commas, semicolons, or
 * backslashes) it will need to be escaped.
 *
 * @param io The GUACIO object to write to.
 * @param str The string to write.
 * @return Zero on success, or non-zero if an error occurs while writing.
*/
ssize_t guac_write_string(GUACIO* io, const char* str);

/**
 * Writes the given binary data to the given GUACIO object as base64-encoded
 * data. The data written may be buffered until the buffer is flushed
 * automatically or manually. Beware that because base64 data is buffered
 * on top of the write buffer already used, a call to guac_flush_base64() must
 * be made before non-base64 writes (or writes of an independent block of
 * base64 data) can be made.
 *
 * @param io The GUACIO object to write to.
 * @param buf A buffer containing the data to write.
 * @param count The number of bytes to write.
 * @return Zero on success, or non-zero if an error occurs while writing.
 */
ssize_t guac_write_base64(GUACIO* io, const void* buf, size_t count);

/**
 * Flushes the base64 buffer, writing padding characters as necessary.
 *
 * @param io The GUACIO object to flush
 * @return Zero on success, or non-zero if an error occurs during flush.
 */
ssize_t guac_flush_base64(GUACIO* io);

/**
 * Flushes the write buffer.
 *
 * @param io The GUACIO object to flush
 * @return Zero on success, or non-zero if an error occurs during flush.
 */
ssize_t guac_flush(GUACIO* io);


/**
 * Waits for input to be available on the given GUACIO object until the
 * specified timeout elapses.
 *
 * @param io The GUACIO object to wait for.
 * @param usec_timeout The maximum number of microseconds to wait for data, or
 *                     -1 to potentially wait forever.
 * @return Positive on success, zero if the timeout elapsed and no data is
 *         available, negative on error.
 */
int guac_select(GUACIO* io, int usec_timeout);

/**
 * Frees resources allocated to the given GUACIO object. Note that this
 * implicitly flush all buffers, but will NOT close the associated file
 * descriptor.
 *
 * @param io The GUACIO object to close.
 */
void guac_close(GUACIO* io);

#endif

