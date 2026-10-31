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

#ifndef GUACCLIP_INSTRUCTIONS_H
#define GUACCLIP_INSTRUCTIONS_H

#include "state.h"

/**
 * A callback function which, when invoked, handles a particular Guacamole
 * instruction. The opcode of the instruction is implied (as it is expected
 * that there will be a 1:1 mapping of opcode to callback function), while the
 * arguments for that instruction are included in the parameters given to the
 * callback.
 *
 * @param state
 *     The current state of the guacclip interpreter.
 *
 * @param argc
 *     The number of arguments (excluding opcode) passed to the instruction
 *     being handled by the callback.
 *
 * @param argv
 *     All arguments (excluding opcode) associated with the instruction being
 *     handled by the callback. Note that the parser owns this memory and
 *     reuses it on the next read, so any data which must persist must be
 *     copied.
 *
 * @return
 *     Zero if the instruction was handled successfully, non-zero if an error
 *     occurs.
 */
typedef int guacclip_instruction_handler(guacclip_state* state,
        int argc, char** argv);

/**
 * Mapping of instruction opcode to corresponding handler function.
 */
typedef struct guacclip_instruction_handler_mapping {

    /**
     * The opcode of the instruction that the associated handler function
     * should be invoked for.
     */
    const char* opcode;

    /**
     * The handler function to invoke whenever an instruction having the
     * associated opcode is parsed.
     */
    guacclip_instruction_handler* handler;

} guacclip_instruction_handler_mapping;

/**
 * Array of all opcode/handler mappings for all supported opcodes, terminated
 * by an entry with a NULL opcode. All opcodes not listed here can be safely
 * ignored.
 */
extern guacclip_instruction_handler_mapping guacclip_instruction_handler_map[];

/**
 * Handles the instruction having the given opcode and arguments, updating
 * the state of the interpreter accordingly.
 *
 * @param state
 *     The current state of the guacclip interpreter.
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
int guacclip_handle_instruction(guacclip_state* state,
        const char* opcode, int argc, char** argv);

/**
 * Handler for the Guacamole "sync" instruction. Tracks the latest recording
 * timestamp.
 */
guacclip_instruction_handler guacclip_handle_sync;

/**
 * Handler for the Guacamole "log" instruction. Parses clipboard direction
 * annotations emitted by the recording.
 */
guacclip_instruction_handler guacclip_handle_log;

/**
 * Handler for the Guacamole "clipboard" instruction. Begins a clipboard
 * stream.
 */
guacclip_instruction_handler guacclip_handle_clipboard;

/**
 * Handler for the Guacamole "blob" instruction. Appends decoded data to an
 * open clipboard stream (blobs for non-clipboard streams are ignored).
 */
guacclip_instruction_handler guacclip_handle_blob;

/**
 * Handler for the Guacamole "end" instruction. Ends an open clipboard stream
 * (ends for non-clipboard streams are ignored).
 */
guacclip_instruction_handler guacclip_handle_end;

#endif
