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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

int guac_ssh_ttymodes_init(char opcode_array[], const int array_size,
        const int num_opcodes, ...) {

    /* Initialize the variable argument list. */
    va_list args;
    va_start(args, num_opcodes);

    /* Check to make sure the array has enough space.
       We need one extra byte at the end for the ending opcode. */
    if ((num_opcodes * GUAC_SSH_TTY_OPCODE_SIZE) >= (array_size))
        return 1;

    for (int i = 0; i < num_opcodes; i++) {
        /* Calculate offset in array */
        int offset = i * GUAC_SSH_TTY_OPCODE_SIZE;

        /* Get the next argument to this function */
        guac_ssh_ttymode ttymode = va_arg(args, guac_ssh_ttymode);

        /* Place opcode and value in array */
        opcode_array[offset] = ttymode.opcode;
        opcode_array[offset + 1] = (ttymode.value >> 24) & 0xFF;
        opcode_array[offset + 2] = (ttymode.value >> 16) & 0xFF;
        opcode_array[offset + 3] = (ttymode.value >> 8) & 0xFF;
        opcode_array[offset + 4] = ttymode.value & 0xFF;
    }

    /* Put the end opcode in the last opcode space */
    opcode_array[num_opcodes * GUAC_SSH_TTY_OPCODE_SIZE] = GUAC_SSH_TTY_OP_END;

    return 0;

}
