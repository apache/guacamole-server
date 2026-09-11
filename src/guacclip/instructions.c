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
#include "instructions.h"
#include "log.h"
#include "state.h"

#include <guacamole/protocol.h>

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Parses the given string as a Guacamole stream index. Only strictly valid,
 * non-negative base-10 integers (with no trailing garbage) are accepted;
 * anything else, including empty strings and negative numbers, is rejected.
 * This avoids cross-associating malformed instructions (which would
 * otherwise parse as index 0 under atoi()) with an unrelated, legitimately
 * open stream 0.
 *
 * @param text
 *     The string to parse as a stream index.
 *
 * @param index
 *     Pointer to an int which will receive the parsed stream index if
 *     parsing succeeds.
 *
 * @return
 *     true if the given string was a valid stream index, false otherwise.
 */
static bool guacclip_parse_stream_index(const char* text, int* index) {

    char* endptr = NULL;

    errno = 0;
    long value = strtol(text, &endptr, 10);

    if (endptr == text || *endptr != '\0' || errno != 0
            || value < 0 || value > INT_MAX)
        return false;

    *index = (int) value;
    return true;

}

guacclip_instruction_handler_mapping guacclip_instruction_handler_map[] = {
    {"sync",      guacclip_handle_sync},
    {"log",       guacclip_handle_log},
    {"clipboard", guacclip_handle_clipboard},
    {"blob",      guacclip_handle_blob},
    {"end",       guacclip_handle_end},
    {NULL,        NULL}
};

int guacclip_handle_sync(guacclip_state* state, int argc, char** argv) {

    /* The sync instruction carries a single timestamp argument */
    if (argc < 1)
        return 0;

    guacclip_state_sync(state, (int64_t) strtoll(argv[0], NULL, 10));
    return 0;

}

int guacclip_handle_log(guacclip_state* state, int argc, char** argv) {

    /* The log instruction carries a single human-readable message */
    if (argc < 1)
        return 0;

    const char* message = argv[0];

    int index;
    char direction[64];
    char mimetype[256];

    /* Only clipboard direction annotations are of interest */
    if (sscanf(message, "clipboard stream=%d direction=%63s mimetype=%255s",
                &index, direction, mimetype) != 3)
        return 0;

    /* The direction must be one of the two recognized values */
    if (strcmp(direction, "guest-to-client") != 0
            && strcmp(direction, "client-to-guest") != 0)
        return 0;

    /* Parse the optional byte-count annotation, if present */
    int64_t expected_bytes = -1;
    const char* bytes = strstr(message, " bytes=");
    if (bytes != NULL) {
        long long value;
        if (sscanf(bytes, " bytes=%lld", &value) == 1 && value >= 0)
            expected_bytes = (int64_t) value;
    }

    guacclip_state_buffer_direction(state, index, direction, expected_bytes);
    return 0;

}

int guacclip_handle_clipboard(guacclip_state* state, int argc, char** argv) {

    /* The clipboard instruction requires a stream index and mimetype */
    if (argc < 2)
        return 0;

    int index;
    if (!guacclip_parse_stream_index(argv[0], &index))
        return 0;

    const char* mimetype = argv[1];

    guacclip_state_open(state, index, mimetype);
    return 0;

}

int guacclip_handle_blob(guacclip_state* state, int argc, char** argv) {

    /* The blob instruction requires a stream index and base64 payload */
    if (argc < 2)
        return 0;

    int index;
    if (!guacclip_parse_stream_index(argv[0], &index))
        return 0;

    /* Decode the base64 payload in place; the parser owns this memory and
     * will reuse it on the next read, so the decoded bytes are immediately
     * copied into the stream's accumulation buffer by guacclip_state_append.
     * guac_protocol_decode_base64() never returns a negative length; zero
     * (an empty or fully-invalid payload) is safely handled by
     * guacclip_state_append(). */
    int length = guac_protocol_decode_base64(argv[1]);

    guacclip_state_append(state, index, argv[1], (size_t) length);
    return 0;

}

int guacclip_handle_end(guacclip_state* state, int argc, char** argv) {

    /* The end instruction requires a stream index */
    if (argc < 1)
        return 0;

    int index;
    if (!guacclip_parse_stream_index(argv[0], &index))
        return 0;

    guacclip_state_end(state, index);
    return 0;

}

int guacclip_handle_instruction(guacclip_state* state, const char* opcode,
        int argc, char** argv) {

    /* Search through mapping for instruction handler having given opcode */
    guacclip_instruction_handler_mapping* current =
            guacclip_instruction_handler_map;
    while (current->opcode != NULL) {

        /* Invoke handler if opcode matches (if defined) */
        if (strcmp(current->opcode, opcode) == 0) {

            /* Invoke defined handler */
            guacclip_instruction_handler* handler = current->handler;
            if (handler != NULL)
                return handler(state, argc, argv);

            /* Log defined but unimplemented instructions */
            guacclip_log(GUAC_LOG_DEBUG, "\"%s\" not implemented", opcode);
            return 0;

        }

        /* Next candidate handler */
        current++;

    } /* end opcode search */

    /* Ignore any unknown instructions */
    return 0;

}
