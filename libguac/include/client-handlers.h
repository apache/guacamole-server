
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

#ifndef _GUAC_CLIENT_HANDLERS__H
#define _GUAC_CLIENT_HANDLERS__H

#include "client.h"
#include "instruction.h"

/**
 * Provides initial handler functions and a lookup structure for automatically
 * handling client instructions. This is used only internally within libguac,
 * and is not installed along with the library.
 *
 * @file client-handlers.h
 */

/**
 * Internal handler for Guacamole instructions.
 */
typedef int __guac_instruction_handler(guac_client* client, guac_instruction* copied);

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
int __guac_handle_sync(guac_client* client, guac_instruction* instruction);

/**
 * Internal initial handler for the mouse instruction. When a mouse instruction
 * is received, this handler will be called. The client's mouse handler will
 * be invoked if defined.
 */
int __guac_handle_mouse(guac_client* client, guac_instruction* instruction);

/**
 * Internal initial handler for the key instruction. When a key instruction
 * is received, this handler will be called. The client's key handler will
 * be invoked if defined.
 */
int __guac_handle_key(guac_client* client, guac_instruction* instruction);

/**
 * Internal initial handler for the clipboard instruction. When a clipboard instruction
 * is received, this handler will be called. The client's clipboard handler will
 * be invoked if defined.
 */
int __guac_handle_clipboard(guac_client* client, guac_instruction* instruction);

/**
 * Internal initial handler for the size instruction. When a size instruction
 * is received, this handler will be called. The client's size handler will
 * be invoked if defined.
 */
int __guac_handle_size(guac_client* client, guac_instruction* instruction);

/**
 * Internal initial handler for the video instruction. When a video instruction
 * is received, this handler will be called. The client's video handler will
 * be invoked if defined.
 */
int __guac_handle_video(guac_client* client, guac_instruction* instruction);

/**
 * Internal initial handler for the audio instruction. When a audio instruction
 * is received, this handler will be called. The client's audio handler will
 * be invoked if defined.
 */
int __guac_handle_audio(guac_client* client, guac_instruction* instruction);

/**
 * Internal initial handler for the disconnect instruction. When a disconnect instruction
 * is received, this handler will be called. Disconnect instructions are automatically
 * handled, thus there is no client handler for disconnect instruction.
 */
int __guac_handle_disconnect(guac_client* client, guac_instruction* instruction);

/**
 * Instruction handler mapping table. This is a NULL-terminated array of
 * __guac_instruction_handler_mapping structures, each mapping an opcode
 * to a __guac_instruction_handler. The end of the array must be marked
 * with a __guac_instruction_handler_mapping with the opcode set to
 * NULL (the NULL terminator).
 */
extern __guac_instruction_handler_mapping __guac_instruction_handler_map[];

#endif
