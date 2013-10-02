
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "error.h"
#include "instruction.h"
#include "protocol.h"
#include "socket.h"
#include "unicode.h"

guac_instruction* guac_instruction_alloc() {

    /* Allocate space for instruction */
    guac_instruction* instruction = malloc(sizeof(guac_instruction));
    if (instruction == NULL) {
        guac_error = GUAC_STATUS_NO_MEMORY;
        guac_error_message = "Insufficient memory to allocate instruction";
        return NULL;
    }

    /* Initialize state */
    instruction->opcode = NULL;
    instruction->argc = 0;
    instruction->state = GUAC_INSTRUCTION_PARSE_LENGTH;
    instruction->__elementc = 0;

    return instruction;

}

int guac_instruction_append(guac_instruction* instr,
        void* buffer, int length) {

    char* char_buffer = (char*) buffer;
    int bytes_parsed = 0;

    /* Do not exceed maximum number of elements */
    if (instr->__elementc == GUAC_INSTRUCTION_MAX_ELEMENTS
            && instr->state != GUAC_INSTRUCTION_PARSE_COMPLETE) {
        instr->state = GUAC_INSTRUCTION_PARSE_ERROR;
        return 0;
    }

    /* Parse element length */
    if (instr->state == GUAC_INSTRUCTION_PARSE_LENGTH) {

        int parsed_length = 0;
        while (bytes_parsed < length) {

            /* Pull next character */
            char c = *(char_buffer++);
            bytes_parsed++;

            /* If digit, add to length */
            if (c >= '0' && c <= '9')
                parsed_length = parsed_length*10 + c - '0';

            /* If period, switch to parsing content */
            else if (c == '.') {
                instr->__element_length = parsed_length;
                instr->state = GUAC_INSTRUCTION_PARSE_CONTENT;
                break;
            }

            /* If not digit, parse error */
            else {
                instr->state = GUAC_INSTRUCTION_PARSE_ERROR;
                return 0;
            }

        }

        /* If too long, parse error */
        if (parsed_length > GUAC_INSTRUCTION_MAX_LENGTH) {
            instr->state = GUAC_INSTRUCTION_PARSE_ERROR;
            return 0;
        }

    } /* end parse length */

    /* Parse element content */
    if (instr->state == GUAC_INSTRUCTION_PARSE_CONTENT) {

        /* If enough data given, finish element */
        if (length - bytes_parsed > instr->__element_length) {

            /* Pull terminator */
            char terminator = char_buffer[instr->__element_length];

            /* Store reference to string within elementv */
            instr->__elementv[instr->__elementc++] = char_buffer;
            char_buffer[instr->__element_length] = '\0';

            bytes_parsed += instr->__element_length+1;

            /* If semicolon, store end-of-instruction */
            if (terminator == ';') {
                instr->state = GUAC_INSTRUCTION_PARSE_COMPLETE;
                instr->opcode = instr->__elementv[0];
                instr->argv = &(instr->__elementv[1]);
                instr->argc = instr->__elementc - 1;
            }

            /* If comma, move on to next element */
            else if (terminator == ',')
                instr->state = GUAC_INSTRUCTION_PARSE_LENGTH;

            /* Otherwise, parse error */
            else {
                instr->state = GUAC_INSTRUCTION_PARSE_ERROR;
                return 0;
            }

        }

    } /* end parse content */

    return bytes_parsed;

}

/* Returns new instruction if one exists, or NULL if no more instructions. */
guac_instruction* guac_instruction_read(guac_socket* socket,
        int usec_timeout) {

    char* unparsed_end = socket->__instructionbuf_unparsed_end;
    char* unparsed_start = socket->__instructionbuf_unparsed_start;
    char* buffer_end = socket->__instructionbuf
                            + sizeof(socket->__instructionbuf);

    guac_instruction* instruction = guac_instruction_alloc();

    while (instruction->state != GUAC_INSTRUCTION_PARSE_COMPLETE
        && instruction->state != GUAC_INSTRUCTION_PARSE_ERROR) {

        /* Add any available data to buffer */
        int parsed = guac_instruction_append(instruction, unparsed_start,
                unparsed_end - unparsed_start);

        /* Read more data if not enough data to parse */
        if (parsed == 0) {

            int retval;

            /* If no space left to read, fail */
            if (unparsed_end == buffer_end) {
                guac_error = GUAC_STATUS_NO_MEMORY;
                guac_error_message = "Instruction too long";
                return NULL;
            }

            /* No instruction yet? Get more data ... */
            retval = guac_socket_select(socket, usec_timeout);
            if (retval <= 0)
                return NULL;
           
            /* Attempt to fill buffer */
            retval = guac_socket_read(socket, unparsed_end,
                    buffer_end - unparsed_end);

            /* Set guac_error if read unsuccessful */
            if (retval < 0) {
                guac_error = GUAC_STATUS_SEE_ERRNO;
                guac_error_message = "Error filling instruction buffer";
                return NULL;
            }

            /* EOF */
            if (retval == 0) {
                guac_error = GUAC_STATUS_NO_INPUT;
                guac_error_message = "End of stream reached while "
                                     "reading instruction";
                return NULL;
            }

            /* Update internal buffer */
            unparsed_end += retval;

        }

        /* If data was parsed, advance buffer */
        else
            unparsed_start += parsed;

    } /* end while parsing data */

    /* Fail on error */
    if (instruction->state == GUAC_INSTRUCTION_PARSE_ERROR) {
        guac_error = GUAC_STATUS_BAD_ARGUMENT;
        guac_error_message = "Instruction parse error";
        return NULL;
    }

    socket->__instructionbuf_unparsed_start = unparsed_start;
    socket->__instructionbuf_unparsed_end = unparsed_end;
    return instruction;

}


guac_instruction* guac_instruction_expect(guac_socket* socket, int usec_timeout,
        const char* opcode) {

    guac_instruction* instruction;

    /* Wait for data until timeout */
    if (guac_instruction_waiting(socket, usec_timeout) <= 0)
        return NULL;

    /* Read available instruction */
    instruction = guac_instruction_read(socket, usec_timeout);
    if (instruction == NULL)
        return NULL;            

    /* Validate instruction */
    if (strcmp(instruction->opcode, opcode) != 0) {
        guac_error = GUAC_STATUS_BAD_STATE;
        guac_error_message = "Instruction read did not have expected opcode";
        guac_instruction_free(instruction);
        return NULL;
    }

    /* Return instruction if valid */
    return instruction;

}


void guac_instruction_free(guac_instruction* instruction) {
    free(instruction);
}


int guac_instruction_waiting(guac_socket* socket, int usec_timeout) {

    if (socket->__instructionbuf_unparsed_end >
            socket->__instructionbuf_unparsed_start)
        return 1;

    return guac_socket_select(socket, usec_timeout);
}

