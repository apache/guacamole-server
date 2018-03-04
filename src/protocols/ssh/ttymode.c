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

#include "config.h"
#include "ttymode.h"

#include <stdlib.h>
#include <string.h>

guac_ssh_ttymodes* guac_ssh_ttymodes_init(int max_opcodes) {
    /* Allocate enough space for the max opcodes */
    guac_ssh_ttymode* ttymode_array = malloc(sizeof(guac_ssh_ttymode) * max_opcodes);
    
    /* Set up the initial data structure. */
    guac_ssh_ttymodes* empty_modes = malloc(sizeof(guac_ssh_ttymodes));
    empty_modes->ttymode_array = ttymode_array;
    empty_modes->num_opcodes = 0;

    return empty_modes;
}

void guac_ssh_ttymodes_add(guac_ssh_ttymodes *tty_modes, char opcode, uint32_t value) {
    /* Increment number of opcodes */
    tty_modes->num_opcodes++;

    /* Add new values */
    tty_modes->ttymode_array[tty_modes->num_opcodes - 1].opcode = opcode;
    tty_modes->ttymode_array[tty_modes->num_opcodes - 1].value = value;
}

int guac_ssh_ttymodes_size(guac_ssh_ttymodes *tty_modes) {
    /* Each opcode pair is 5 bytes (1 opcode, 4 value)
       Add one for the ending opcode */
    return (tty_modes->num_opcodes * 5) + 1;
}

char* guac_ssh_ttymodes_to_array(guac_ssh_ttymodes *tty_modes, int data_size) {
 
    char* temp = malloc(data_size);

    /* Loop through the array based on number of tracked
       opcodes and convert each one. */
    for (int i = 0; i < tty_modes->num_opcodes; i++) {
        int idx = i * 5;
        uint32_t value = tty_modes->ttymode_array[i].value;

        /* Opcode goes in first byte. */
        temp[idx] = tty_modes->ttymode_array[i].opcode;

        /* Convert 32-bit int to individual bytes. */
        temp[idx+1] = (value >> 24) & 0xFF;
        temp[idx+2] = (value >> 16) & 0xFF;
        temp[idx+3] = (value >> 8) & 0xFF;
        temp[idx+4] = value & 0xFF;
    }
    
    /* Add the ending opcode */
    temp[data_size - 1] = GUAC_SSH_TTY_OP_END;

    return temp;
}
