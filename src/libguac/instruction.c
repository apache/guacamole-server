
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
    instruction->state = GUAC_INSTRUCTION_PARSE_LENGTH;

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

int __guac_fill_instructionbuf(guac_socket* socket) {

    int retval;
    
    /* Attempt to fill buffer */
    retval = guac_socket_read(
        socket,
        socket->__instructionbuf + socket->__instructionbuf_used_length,
        socket->__instructionbuf_size - socket->__instructionbuf_used_length
    );

    /* Set guac_error if recv() unsuccessful */
    if (retval < 0) {
        guac_error = GUAC_STATUS_SEE_ERRNO;
        guac_error_message = "Error filling instruction buffer";
        return retval;
    }

    socket->__instructionbuf_used_length += retval;

    /* Expand buffer if necessary */
    if (socket->__instructionbuf_used_length >
            socket->__instructionbuf_size / 2) {

        socket->__instructionbuf_size *= 2;
        socket->__instructionbuf = realloc(socket->__instructionbuf,
                socket->__instructionbuf_size);
    }

    return retval;

}


/* Returns new instruction if one exists, or NULL if no more instructions. */
guac_instruction* guac_instruction_read(guac_socket* socket,
        int usec_timeout) {

    int retval;
   
    /* Loop until a instruction is read */
    for (;;) {

        /* Length of element, in Unicode characters */
        int element_length = 0;

        /* Length of element, in bytes */
        int element_byte_length = 0;

        /* Current position within the element, in Unicode characters */
        int current_unicode_length = 0;

        /* Position within buffer */
        int i = socket->__instructionbuf_parse_start;

        /* Parse instruction in buffer */
        while (i < socket->__instructionbuf_used_length) {

            /* Read character from buffer */
            char c = socket->__instructionbuf[i++];

            /* If digit, calculate element length */
            if (c >= '0' && c <= '9')
                element_length = element_length * 10 + c - '0';

            /* Otherwise, if end of length */
            else if (c == '.') {

                /* Calculate element byte length by walking buffer */
                while (i + element_byte_length <
                            socket->__instructionbuf_used_length
                    && current_unicode_length < element_length) {

                    /* Get next byte */
                    c = socket->__instructionbuf[i + element_byte_length];

                    /* Update byte and character lengths */
                    element_byte_length += guac_utf8_charsize((unsigned) c);
                    current_unicode_length++;

                }

                /* Verify element is fully read */
                if (current_unicode_length == element_length) {

                    /* Get element value */
                    char* elementv = &(socket->__instructionbuf[i]);
                   
                    /* Get terminator, set null terminator of elementv */ 
                    char terminator = elementv[element_byte_length];
                    elementv[element_byte_length] = '\0';

                    /* Move to char after terminator of element */
                    i += element_byte_length+1;

                    /* Reset element length */
                    element_length =
                    element_byte_length =
                    current_unicode_length = 0;

                    /* As element has been read successfully, update
                     * parse start */
                    socket->__instructionbuf_parse_start = i;

                    /* Save element */
                    socket->__instructionbuf_elementv[socket->__instructionbuf_elementc++] = elementv;

                    /* Finish parse if terminator is a semicolon */
                    if (terminator == ';') {

                        guac_instruction* parsed_instruction;
                        int j;

                        /* Allocate instruction */
                        parsed_instruction = malloc(sizeof(guac_instruction));
                        if (parsed_instruction == NULL) {
                            guac_error = GUAC_STATUS_NO_MEMORY;
                            guac_error_message = "Could not allocate memory for parsed instruction";
                            return NULL;
                        }

                        /* Init parsed instruction */
                        parsed_instruction->argc = socket->__instructionbuf_elementc - 1;
                        parsed_instruction->argv = malloc(sizeof(char*) * parsed_instruction->argc);

                        /* Fail if memory could not be alloc'd for argv */
                        if (parsed_instruction->argv == NULL) {
                            guac_error = GUAC_STATUS_NO_MEMORY;
                            guac_error_message = "Could not allocate memory for arguments of parsed instruction";
                            free(parsed_instruction);
                            return NULL;
                        }

                        /* Set opcode */
                        parsed_instruction->opcode = strdup(socket->__instructionbuf_elementv[0]);

                        /* Fail if memory could not be alloc'd for opcode */
                        if (parsed_instruction->opcode == NULL) {
                            guac_error = GUAC_STATUS_NO_MEMORY;
                            guac_error_message = "Could not allocate memory for opcode of parsed instruction";
                            free(parsed_instruction->argv);
                            free(parsed_instruction);
                            return NULL;
                        }


                        /* Copy element values to parsed instruction */
                        for (j=0; j<parsed_instruction->argc; j++) {
                            parsed_instruction->argv[j] = strdup(socket->__instructionbuf_elementv[j+1]);

                            /* Free memory and fail if out of mem */
                            if (parsed_instruction->argv[j] == NULL) {
                                guac_error = GUAC_STATUS_NO_MEMORY;
                                guac_error_message = "Could not allocate memory for single argument of parsed instruction";

                                /* Free all alloc'd argv values */
                                while (--j >= 0)
                                    free(parsed_instruction->argv[j]);

                                free(parsed_instruction->opcode);
                                free(parsed_instruction->argv);
                                free(parsed_instruction);
                                return NULL;
                            }

                        }

                        /* Reset buffer */
                        memmove(socket->__instructionbuf, socket->__instructionbuf + i, socket->__instructionbuf_used_length - i);
                        socket->__instructionbuf_used_length -= i;
                        socket->__instructionbuf_parse_start = 0;
                        socket->__instructionbuf_elementc = 0;

                        /* Done */
                        return parsed_instruction;

                    } /* end if terminator */

                    /* Error if expected comma is not present */
                    else if (terminator != ',') {
                        guac_error = GUAC_STATUS_BAD_ARGUMENT;
                        guac_error_message = "Element terminator of instruction was not ';' nor ','";
                        return NULL;
                    }

                } /* end if element fully read */

                /* Otherwise, read more data */
                else
                    break;

            }

            /* Error if length is non-numeric or does not end in a period */
            else {
                guac_error = GUAC_STATUS_BAD_ARGUMENT;
                guac_error_message = "Non-numeric character in element length";
                return NULL;
            }

        }

        /* No instruction yet? Get more data ... */
        retval = guac_socket_select(socket, usec_timeout);
        if (retval <= 0)
            return NULL;

        /* If more data is available, fill into buffer */
        retval = __guac_fill_instructionbuf(socket);

        /* Error, guac_error already set */
        if (retval < 0)
            return NULL;

        /* EOF */
        if (retval == 0) {
            guac_error = GUAC_STATUS_NO_INPUT;
            guac_error_message = "End of stream reached while reading instruction";
            return NULL;
        }

    }

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

    int argc = instruction->argc;

    /* Free opcode */
    free(instruction->opcode);

    /* Free argv if set (may be NULL of argc is 0) */
    if (instruction->argv) {

        /* All argument values */
        while (argc > 0)
            free(instruction->argv[--argc]);

        /* Free actual array */
        free(instruction->argv);

    }

    /* Free instruction */
    free(instruction);

}


int guac_instruction_waiting(guac_socket* socket, int usec_timeout) {

    if (socket->__instructionbuf_used_length > 0)
        return 1;

    return guac_socket_select(socket, usec_timeout);
}

