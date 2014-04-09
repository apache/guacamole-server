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


#ifndef _GUAC_INSTRUCTION_H
#define _GUAC_INSTRUCTION_H

/**
 * Provides functions and structures for reading, writing, and manipulating
 * Guacamole instructions.
 *
 * @file instruction.h
 */

#include "instruction-types.h"
#include "instruction-constants.h"
#include "socket-types.h"

struct guac_instruction {

    /**
     * The opcode of the instruction.
     */
    char* opcode;

    /**
     * The number of arguments passed to this instruction.
     */
    int argc;

    /**
     * Array of all arguments passed to this instruction.
     */
    char** argv;

    /**
     * The parse state of the instruction.
     */
    guac_instruction_parse_state state;

    /**
     * The length of the current element, if known.
     */
    int __element_length;

    /**
     * The number of elements currently parsed.
     */
    int __elementc;

    /**
     * All currently parsed elements.
     */
    char* __elementv[GUAC_INSTRUCTION_MAX_ELEMENTS];

};

/**
 * Allocates a new instruction. Each instruction contains within itself the
 * necessary facilities to parse instruction data.
 *
 * @return The newly allocated instruction, or NULL if an error occurs during
 *         allocation, in which case guac_error will be set appropriately.
 */
guac_instruction* guac_instruction_alloc();

/**
 * Resets the parse state and contents of the given instruction, such that the
 * memory of that instruction can be reused for another parse cycle.
 *
 * @param instruction The instruction to reset.
 */
void guac_instruction_reset(guac_instruction* instruction);

/**
 * Appends data from the given buffer to the given instruction. The data will
 * be appended, if possible, to this instruction as a reference and thus the
 * buffer must remain valid throughout the life of the instruction. This
 * function may modify the contents of the buffer when those contents are
 * part of an element within the instruction being read.
 *
 * @param instruction The instruction to append data to.
 * @param buffer A buffer containing data that should be appended to this
 *               instruction.
 * @param length The number of bytes available for appending within the buffer.
 * @return The number of bytes appended to this instruction, which may be
 *         zero if more data is needed.
 */
int guac_instruction_append(guac_instruction* instruction,
        void* buffer, int length);

/**
 * Frees all memory allocated to the given instruction.
 *
 * @param instruction The instruction to free.
 */
void guac_instruction_free(guac_instruction* instruction);

/**
 * Returns whether new instruction data is available on the given guac_socket
 * connection for parsing.
 *
 * @param socket The guac_socket connection to use.
 * @param usec_timeout The maximum number of microseconds to wait before
 *                     giving up.
 * @return A positive value if data is available, negative on error, or
 *         zero if no data is currently available.
 */
int guac_instruction_waiting(guac_socket* socket, int usec_timeout);

/**
 * Reads a single instruction from the given guac_socket connection.
 *
 * If an error occurs reading the instruction, NULL is returned,
 * and guac_error is set appropriately.
 *
 * @param socket The guac_socket connection to use.
 * @param usec_timeout The maximum number of microseconds to wait before
 *                     giving up.
 * @return A new instruction if data was successfully read, NULL on
 *         error or if the instruction could not be read completely
 *         because the timeout elapsed, in which case guac_error will be
 *         set to GUAC_STATUS_INPUT_TIMEOUT and subsequent calls to
 *         guac_protocol_read_instruction() will return the parsed instruction
 *         once enough data is available.
 */
guac_instruction* guac_instruction_read(guac_socket* socket, int usec_timeout);

/**
 * Reads a single instruction with the given opcode from the given guac_socket
 * connection.
 *
 * If an error occurs reading the instruction, NULL is returned,
 * and guac_error is set appropriately.
 *
 * If the instruction read is not the expected instruction, NULL is returned,
 * and guac_error is set to GUAC_STATUS_BAD_STATE.
 *
 * @param socket The guac_socket connection to use.
 * @param usec_timeout The maximum number of microseconds to wait before
 *                     giving up.
 * @param opcode The opcode of the instruction to read.
 * @return A new instruction if an instruction with the given opcode was read,
 *         NULL otherwise. If an instruction was read, but the instruction had
 *         a different opcode, NULL is returned and guac_error is set to
 *         GUAC_STATUS_BAD_STATE.
 */
guac_instruction* guac_instruction_expect(guac_socket* socket,
        int usec_timeout, const char* opcode);

#endif

