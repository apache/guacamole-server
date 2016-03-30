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


#ifndef _GUAC_USER_HANDLERS__H
#define _GUAC_USER_HANDLERS__H

/**
 * Provides initial handler functions and a lookup structure for automatically
 * handling instructions received from each user. This is used only internally
 * within libguac, and is not installed along with the library.
 *
 * @file user-handlers.h
 */

#include "config.h"

#include "client.h"
#include "timestamp.h"

/**
 * Internal handler for Guacamole instructions. Instruction handlers will be
 * invoked when their corresponding instructions are received. The mapping
 * of instruction opcode to handler is defined by the
 * __guac_instruction_handler_map array.
 *
 * @param user
 *     The user that sent the instruction.
 *
 * @param argc
 *     The number of arguments in argv.
 *
 * @param argv
 *     The arguments included with the instruction, excluding the opcode.
 *
 * @return
 *     Zero if the instruction was successfully handled, non-zero otherwise.
 */
typedef int __guac_instruction_handler(guac_user* user, int argc, char** argv);

/**
 * Structure mapping an instruction opcode to an instruction handler.
 */
typedef struct __guac_instruction_handler_mapping {

    /**
     * The instruction opcode which maps to a specific handler.
     */
    char* opcode;

    /**
     * The handler which maps to a specific opcode.
     */
    __guac_instruction_handler* handler;

} __guac_instruction_handler_mapping;

/**
 * Internal initial handler for the sync instruction. When a sync instruction
 * is received, this handler will be called. Sync instructions are automatically
 * handled, thus there is no client handler for sync instruction.
 */
__guac_instruction_handler __guac_handle_sync;

/**
 * Internal initial handler for the mouse instruction. When a mouse instruction
 * is received, this handler will be called. The client's mouse handler will
 * be invoked if defined.
 */
__guac_instruction_handler __guac_handle_mouse;

/**
 * Internal initial handler for the key instruction. When a key instruction
 * is received, this handler will be called. The client's key handler will
 * be invoked if defined.
 */
__guac_instruction_handler __guac_handle_key;

/**
 * Internal initial handler for the audio instruction. When a audio
 * instruction is received, this handler will be called. The client's audio
 * handler will be invoked if defined.
 */
__guac_instruction_handler __guac_handle_audio;

/**
 * Internal initial handler for the clipboard instruction. When a clipboard
 * instruction is received, this handler will be called. The client's clipboard
 * handler will be invoked if defined.
 */
__guac_instruction_handler __guac_handle_clipboard;

/**
 * Internal initial handler for the file instruction. When a file instruction
 * is received, this handler will be called. The client's file handler will
 * be invoked if defined.
 */
__guac_instruction_handler __guac_handle_file;

/**
 * Internal initial handler for the pipe instruction. When a pipe instruction
 * is received, this handler will be called. The client's pipe handler will
 * be invoked if defined.
 */
__guac_instruction_handler __guac_handle_pipe;

/**
 * Internal initial handler for the ack instruction. When a ack instruction
 * is received, this handler will be called. The client's ack handler will
 * be invoked if defined.
 */
__guac_instruction_handler __guac_handle_ack;

/**
 * Internal initial handler for the blob instruction. When a blob instruction
 * is received, this handler will be called. The client's blob handler will
 * be invoked if defined.
 */
__guac_instruction_handler __guac_handle_blob;

/**
 * Internal initial handler for the end instruction. When a end instruction
 * is received, this handler will be called. The client's end handler will
 * be invoked if defined.
 */
__guac_instruction_handler __guac_handle_end;

/**
 * Internal initial handler for the get instruction. When a get instruction
 * is received, this handler will be called. The client's get handler will
 * be invoked if defined.
 */
__guac_instruction_handler __guac_handle_get;

/**
 * Internal initial handler for the put instruction. When a put instruction
 * is received, this handler will be called. The client's put handler will
 * be invoked if defined.
 */
__guac_instruction_handler __guac_handle_put;

/**
 * Internal initial handler for the size instruction. When a size instruction
 * is received, this handler will be called. The client's size handler will
 * be invoked if defined.
 */
__guac_instruction_handler __guac_handle_size;

/**
 * Internal initial handler for the disconnect instruction. When a disconnect
 * instruction is received, this handler will be called. Disconnect
 * instructions are automatically handled, thus there is no client handler for
 * disconnect instruction.
 */
__guac_instruction_handler __guac_handle_disconnect;

/**
 * Instruction handler mapping table. This is a NULL-terminated array of
 * __guac_instruction_handler_mapping structures, each mapping an opcode
 * to a __guac_instruction_handler. The end of the array must be marked
 * with a __guac_instruction_handler_mapping with the opcode set to
 * NULL (the NULL terminator).
 */
extern __guac_instruction_handler_mapping __guac_instruction_handler_map[];

#endif
