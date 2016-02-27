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

#include "config.h"
#include "instructions.h"
#include "log.h"

#include <guacamole/client.h>

#include <string.h>

guacenc_instruction_handler_mapping guacenc_instruction_handler_map[] = {
    {"blob",     NULL},
    {"img",      NULL},
    {"end",      NULL},
    {"cursor",   NULL},
    {"copy",     NULL},
    {"transfer", NULL},
    {"size",     NULL},
    {"rect",     NULL},
    {"cfill",    NULL},
    {"move",     NULL},
    {"shade",    NULL},
    {"dispose",  NULL},
    {NULL,       NULL}
};

int guacenc_handle_instruction(const char* opcode,
        int argc, const char** argv) {

    /* Search through mapping for instruction handler having given opcode */
    guacenc_instruction_handler_mapping* current = guacenc_instruction_handler_map;
    while (current->opcode != NULL) {

        /* Invoke handler if opcode matches (if defined) */
        if (strcmp(current->opcode, opcode) == 0) {

            /* Invoke defined handler */
            guacenc_instruction_handler* handler = current->handler;
            if (handler != NULL)
                return handler(argc, argv);

            /* Log defined but unimplemented instructions */
            guacenc_log(GUAC_LOG_DEBUG, "\"%s\" not implemented", opcode);
            return 0;

        }

        /* Next candidate handler */
        current++;

    } /* end opcode search */

    /* Ignore any unknown instructions */
    return 0;

}

