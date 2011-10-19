
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is libguac.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef _GUAC_GUACIO_H
#define _GUAC_GUACIO_H

#include <stdint.h>
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
     * The number of bytes written total, since this GUACIO was opened.
     */
    int total_written;

    /**
     * The main write buffer. Bytes written go here before being flushed
     * to the open file descriptor.
     */
    char out_buf[8192];

    /**
     * The current location of parsing within the instruction buffer.
     */
    int instructionbuf_parse_start;

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
     * The number of elements parsed so far.
     */
    int instructionbuf_elementc;

    /**
     * Array of pointers into the instruction buffer, where each pointer
     * points to the start of the corresponding element.
     */
    char* instructionbuf_elementv[64];

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
 * Parses the given string as a decimal number, returning the result as
 * a 64-bit signed value. This value will be 64-bit regardless of platform.
 *
 * @param str The string to parse into a 64-bit integer.
 * @return The 64-bit integer representation of the number in the given
 *         string, undefined if the string does not contain a properly
 *         formatted number.
 */
int64_t guac_parse_int(const char* str);

/**
 * Writes the given unsigned int to the given GUACIO object. The data
 * written may be buffered until the buffer is flushed automatically or
 * manually.
 *
 * @param io The GUACIO object to write to.
 * @param i The unsigned int to write.
 * @return Zero on success, or non-zero if an error occurs while writing.
 */
ssize_t guac_write_int(GUACIO* io, int64_t i);

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

