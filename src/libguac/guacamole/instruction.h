
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

#ifndef _GUAC_INSTRUCTION_H
#define _GUAC_INSTRUCTION_H

#include "socket.h"

/**
 * Provides functions and structures for reading, writing, and manipulating
 * Guacamole instructions.
 *
 * @file instruction.h
 */

/**
 * The maximum number of characters per instruction.
 */
#define GUAC_INSTRUCTION_MAX_LENGTH 8192

/**
 * The maximum number of digits to allow per length prefix.
 */
#define GUAC_INSTRUCTION_MAX_DIGITS 5

/**
 * The maximum number of elements per instruction, including the opcode.
 */
#define GUAC_INSTRUCTION_MAX_ELEMENTS 64

/**
 * All possible states of the instruction parser.
 */
typedef enum guac_instruction_parse_state {

    /**
     * The parser is currently waiting for data to complete the length prefix
     * of the current element of the instruction.
     */
    GUAC_INSTRUCTION_PARSE_LENGTH,

    /**
     * The parser has finished reading the length prefix and is currently
     * waiting for data to complete the content of the instruction.
     */
    GUAC_INSTRUCTION_PARSE_CONTENT,

    /**
     * The instruction has been fully parsed.
     */
    GUAC_INSTRUCTION_PARSE_COMPLETE,

    /**
     * The instruction cannot be parsed because of a protocol error.
     */
    GUAC_INSTRUCTION_PARSE_ERROR

} guac_instruction_parse_state;

/**
 * Represents a single instruction within the Guacamole protocol.
 */
typedef struct guac_instruction {

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

} guac_instruction;

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

