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
 * Internal handler for Guacamole instructions.
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
int __guac_handle_sync(guac_user* user, int argc, char** argv);

/**
 * Internal initial handler for the mouse instruction. When a mouse instruction
 * is received, this handler will be called. The client's mouse handler will
 * be invoked if defined.
 */
int __guac_handle_mouse(guac_user* user, int argc, char** argv);

/**
 * Internal initial handler for the key instruction. When a key instruction
 * is received, this handler will be called. The client's key handler will
 * be invoked if defined.
 */
int __guac_handle_key(guac_user* user, int argc, char** argv);

/**
 * Internal initial handler for the clipboard instruction. When a clipboard instruction
 * is received, this handler will be called. The client's clipboard handler will
 * be invoked if defined.
 */
int __guac_handle_clipboard(guac_user* user, int argc, char** argv);

/**
 * Internal initial handler for the file instruction. When a file instruction
 * is received, this handler will be called. The client's file handler will
 * be invoked if defined.
 */
int __guac_handle_file(guac_user* user, int argc, char** argv);

/**
 * Internal initial handler for the pipe instruction. When a pipe instruction
 * is received, this handler will be called. The client's pipe handler will
 * be invoked if defined.
 */
int __guac_handle_pipe(guac_user* user, int argc, char** argv);

/**
 * Internal initial handler for the ack instruction. When a ack instruction
 * is received, this handler will be called. The client's ack handler will
 * be invoked if defined.
 */
int __guac_handle_ack(guac_user* user, int argc, char** argv);

/**
 * Internal initial handler for the blob instruction. When a blob instruction
 * is received, this handler will be called. The client's blob handler will
 * be invoked if defined.
 */
int __guac_handle_blob(guac_user* user, int argc, char** argv);

/**
 * Internal initial handler for the end instruction. When a end instruction
 * is received, this handler will be called. The client's end handler will
 * be invoked if defined.
 */
int __guac_handle_end(guac_user* user, int argc, char** argv);

/**
 * Internal initial handler for the get instruction. When a get instruction
 * is received, this handler will be called. The client's get handler will
 * be invoked if defined.
 */
int __guac_handle_get(guac_user* user, int argc, char** argv);

/**
 * Internal initial handler for the put instruction. When a put instruction
 * is received, this handler will be called. The client's put handler will
 * be invoked if defined.
 */
int __guac_handle_put(guac_user* user, int argc, char** argv);

/**
 * Internal initial handler for the size instruction. When a size instruction
 * is received, this handler will be called. The client's size handler will
 * be invoked if defined.
 */
int __guac_handle_size(guac_user* user, int argc, char** argv);

/**
 * Internal initial handler for the disconnect instruction. When a disconnect instruction
 * is received, this handler will be called. Disconnect instructions are automatically
 * handled, thus there is no client handler for disconnect instruction.
 */
int __guac_handle_disconnect(guac_user* user, int argc, char** argv);

/**
 * Instruction handler mapping table. This is a NULL-terminated array of
 * __guac_instruction_handler_mapping structures, each mapping an opcode
 * to a __guac_instruction_handler. The end of the array must be marked
 * with a __guac_instruction_handler_mapping with the opcode set to
 * NULL (the NULL terminator).
 */
extern __guac_instruction_handler_mapping __guac_instruction_handler_map[];

#endif
