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

guac_ssh_ttymodes init_ttymodes() {
    // Simple allocation for a placeholder
    guac_ssh_ttymode* ttymode_array = malloc(1);
    
    // Set up the initial data structure.
    guac_ssh_ttymodes empty_modes = {
        ttymode_array,
        0
    };

    return empty_modes;
}

void add_ttymode(guac_ssh_ttymodes *tty_modes, char opcode, uint32_t value) {
    tty_modes->num_opcodes++;
    tty_modes->ttymode_array = realloc(tty_modes->ttymode_array, sizeof(guac_ssh_ttymode) * tty_modes->num_opcodes);
    tty_modes->ttymode_array[tty_modes->num_opcodes - 1].opcode = opcode;
    tty_modes->ttymode_array[tty_modes->num_opcodes - 1].value = value;

}

int sizeof_ttymodes(guac_ssh_ttymodes *tty_modes) {
    // Each opcode pair is 5 bytes (1 opcode, 4 value)
    // Add one for the ending opcode
    return (tty_modes->num_opcodes * 5) + 1;
}

char* ttymodes_to_array(guac_ssh_ttymodes *tty_modes) {
    // Total data size should be number of tracked opcodes
    // plus one final byte for the TTY_OP_END code.
    int data_size = (tty_modes->num_opcodes * 5) + 1;
    char* temp = malloc(data_size);

    // Loop through the array based on number of tracked
    // opcodes and convert each one.
    for (int i = 0; i < tty_modes->num_opcodes; i++) {
        int idx = i * 5;
        uint32_t value = tty_modes->ttymode_array[i].value;
        // Opcode goes in first byte.
        temp[idx] = tty_modes->ttymode_array[i].opcode;

        // Convert 32-bit int to individual bytes.
        temp[idx+1] = (value >> 24) & 0xFF;
        temp[idx+2] = (value >> 16) & 0xFF;
        temp[idx+3] = (value >> 8) & 0xFF;
        temp[idx+4] = value & 0xFF;
    }
    
    // Add the ending opcode
    temp[data_size - 1] = GUAC_SSH_TTY_OP_END;

    return temp;
}
