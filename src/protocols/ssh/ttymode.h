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

#ifndef GUAC_SSH_TTYMODE_H
#define GUAC_SSH_TTYMODE_H

#include "config.h"

#include <stdint.h>

/**
 * The SSH TTY mode encoding opcode that terminates
 * the list of TTY modes.
 */
#define GUAC_SSH_TTY_OP_END 0

/**
 * The SSH tty mode encoding opcode that configures
 * the TTY erase code to configure the server
 * backspace key.
 */
#define GUAC_SSH_TTY_OP_VERASE 3

/**
 * A data type which holds a single opcode
 * and the value for that opcode.
 */
typedef struct guac_ssh_ttymode {
    char opcode;
    uint32_t value;
} guac_ssh_ttymode;

/**
 * A data type which holds an array of 
 * guac_ssh_ttymode data, along with the count of
 * the number of opcodes currently in the array.
 */
typedef struct guac_ssh_ttymodes {
    guac_ssh_ttymode* ttymode_array;
    int num_opcodes;
} guac_ssh_ttymodes;


/**
 * Initialize an empty guac_ssh_ttymodes data structure,
 * with a null array of guac_ssh_ttymode and opcodes
 * set to zero.
 */
guac_ssh_ttymodes init_ttymodes();

/**
 * Add an item to the opcode array.  This resizes the
 * array, increments the number of opcodes, and adds
 * the specified opcode and value pair to the data
 * structure.
 */
void add_ttymode(guac_ssh_ttymodes *tty_modes, char opcode, uint32_t value);

/**
 * Retrieve the size, in bytes, of the ttymode_array
 * in the given guac_ssh_ttymodes data structure.
 */
int sizeof_ttymodes(guac_ssh_ttymodes *tty_modes);

/**
 * Convert the ttymodes data structure into a char
 * pointer array suitable for passing into the
 * libssh2_channel_request_pty_ex() function.
 */
char* ttymodes_to_array(guac_ssh_ttymodes *tty_modes);

#endif
