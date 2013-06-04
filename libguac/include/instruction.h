
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

} guac_instruction;


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

