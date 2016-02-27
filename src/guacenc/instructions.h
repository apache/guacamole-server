/*
 * Copyright (C) 2016 Glyptodon, Inc.
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

#ifndef GUACENC_INSTRUCTIONS_H
#define GUACENC_INSTRUCTIONS_H

#include "config.h"

/**
 * A callback function which, when invoked, handles a particular Guacamole
 * instruction. The opcode of the instruction is implied (as it is expected
 * that there will be a 1:1 mapping of opcode to callback function), while the
 * arguments for that instruction are included in the parameters given to the
 * callback.
 *
 * @param argc
 *     The number of arguments (excluding opcode) passed to the instruction
 *     being handled by the callback.
 *
 * @param argv
 *     All arguments (excluding opcode) associated with the instruction being
 *     handled by the callback.
 *
 * @return
 *     Zero if the instruction was handled successfully, non-zero if an error
 *     occurs.
 */
typedef int guacenc_instruction_handler(int argc, const char** argv);

/**
 * Mapping of instruction opcode to corresponding handler function.
 */
typedef struct guacenc_instruction_handler_mapping {

    /**
     * The opcode of the instruction that the associated handler function
     * should be invoked for.
     */
    const char* opcode;

    /**
     * The handler function to invoke whenever an instruction having the
     * associated opcode is parsed.
     */
    guacenc_instruction_handler* handler;

} guacenc_instruction_handler_mapping;

/**
 * Array of all opcode/handler mappings for all supported opcodes, terminated
 * by an entry with a NULL opcode. All opcodes not listed here can be safely
 * ignored.
 */
extern guacenc_instruction_handler_mapping guacenc_instruction_handler_map[];

/**
 * Handles the instruction having the given opcode and arguments, encoding the
 * result to the in-progress video.
 *
 * @param opcode
 *     The opcode of the instruction being handled.
 *
 * @param argc
 *     The number of arguments (excluding opcode) passed to the instruction
 *     being handled by the callback.
 *
 * @param argv
 *     All arguments (excluding opcode) associated with the instruction being
 *     handled by the callback.
 *
 * @return
 *     Zero if the instruction was handled successfully, non-zero if an error
 *     occurs.
 */
int guacenc_handle_instruction(const char* opcode, int argc,
        const char** argv);

#endif

