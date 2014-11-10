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

#include "config.h"

#include "error.h"
#include "instruction.h"
#include "socket.h"
#include "unicode.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

guac_instruction* guac_instruction_alloc() {

    /* Allocate space for instruction */
    guac_instruction* instruction = malloc(sizeof(guac_instruction));
    if (instruction == NULL) {
        guac_error = GUAC_STATUS_NO_MEMORY;
        guac_error_message = "Insufficient memory to allocate instruction";
        return NULL;
    }

    guac_instruction_reset(instruction);
    return instruction;

}

void guac_instruction_reset(guac_instruction* instruction) {
    instruction->opcode = NULL;
    instruction->argc = 0;
    instruction->state = GUAC_INSTRUCTION_PARSE_LENGTH;
    instruction->__elementc = 0;
    instruction->__element_length = 0;
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

        int parsed_length = instr->__element_length;
        while (bytes_parsed < length) {

            /* Pull next character */
            char c = *(char_buffer++);
            bytes_parsed++;

            /* If digit, add to length */
            if (c >= '0' && c <= '9')
                parsed_length = parsed_length*10 + c - '0';

            /* If period, switch to parsing content */
            else if (c == '.') {
                instr->__elementv[instr->__elementc++] = char_buffer;
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

        /* Save length */
        instr->__element_length = parsed_length;

    } /* end parse length */

    /* Parse element content */
    if (instr->state == GUAC_INSTRUCTION_PARSE_CONTENT) {

        while (bytes_parsed < length && instr->__element_length >= 0) {

            /* Get length of current character */
            char c = *char_buffer;
            int char_length = guac_utf8_charsize((unsigned char) c);

            /* If full character not present in buffer, stop now */
            if (char_length + bytes_parsed > length)
                break;

            /* Record character as parsed */
            bytes_parsed += char_length;

            /* If end of element, handle terminator */
            if (instr->__element_length == 0) {

                *char_buffer = '\0';

                /* If semicolon, store end-of-instruction */
                if (c == ';') {
                    instr->state = GUAC_INSTRUCTION_PARSE_COMPLETE;
                    instr->opcode = instr->__elementv[0];
                    instr->argv = &(instr->__elementv[1]);
                    instr->argc = instr->__elementc - 1;
                    break;
                }

                /* If comma, move on to next element */
                else if (c == ',') {
                    instr->state = GUAC_INSTRUCTION_PARSE_LENGTH;
                    break;
                }

                /* Otherwise, parse error */
                else {
                    instr->state = GUAC_INSTRUCTION_PARSE_ERROR;
                    return 0;
                }

            } /* end if end of element */

            /* Advance to next character */
            instr->__element_length--;
            char_buffer += char_length;

        }

    } /* end parse content */

    return bytes_parsed;

}

/* Returns new instruction if one exists, or NULL if no more instructions. */
guac_instruction* guac_instruction_read(guac_socket* socket,
        int usec_timeout) {

    char* unparsed_end = socket->__instructionbuf_unparsed_end;
    char* unparsed_start = socket->__instructionbuf_unparsed_start;
    char* instr_start = socket->__instructionbuf_unparsed_start;
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

                /* Shift backward if possible */
                if (instr_start != socket->__instructionbuf) {

                    int i;

                    /* Shift buffer */
                    int offset = instr_start - socket->__instructionbuf;
                    memmove(socket->__instructionbuf, instr_start,
                            unparsed_end - instr_start);

                    /* Update tracking pointers */
                    unparsed_end -= offset;
                    unparsed_start -= offset;
                    instr_start = socket->__instructionbuf;

                    /* Update parsed elements, if any */
                    for (i=0; i<instruction->__elementc; i++)
                        instruction->__elementv[i] -= offset;

                }

                /* Otherwise, no memory to read */
                else {
                    guac_error = GUAC_STATUS_NO_MEMORY;
                    guac_error_message = "Instruction too long";
                    return NULL;
                }

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
                guac_error = GUAC_STATUS_CLOSED;
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
        guac_error = GUAC_STATUS_PROTOCOL_ERROR;
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
        guac_error = GUAC_STATUS_PROTOCOL_ERROR;
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

