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

#include "dbshell/buffer.h"

#include <guacamole/mem.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/**
 * The initial number of bytes allocated for a buffer upon its first
 * append.
 */
#define GUAC_DBSHELL_BUFFER_INITIAL_SIZE 1024

void guac_dbshell_buffer_init(guac_dbshell_buffer* buffer) {
    buffer->data = NULL;
    buffer->length = 0;
    buffer->allocated = 0;
}

void guac_dbshell_buffer_destroy(guac_dbshell_buffer* buffer) {
    guac_mem_free(buffer->data);
    buffer->data = NULL;
    buffer->length = 0;
    buffer->allocated = 0;
}

void guac_dbshell_buffer_append(guac_dbshell_buffer* buffer,
        const char* bytes, int length) {

    if (length <= 0)
        return;

    /* Expand storage as necessary */
    int required = buffer->length + length;
    if (required > buffer->allocated) {

        int allocated = buffer->allocated;
        if (allocated == 0)
            allocated = GUAC_DBSHELL_BUFFER_INITIAL_SIZE;

        while (allocated < required)
            allocated = guac_mem_ckd_mul_or_die(allocated, 2);

        buffer->data = guac_mem_realloc(buffer->data, allocated);
        buffer->allocated = allocated;

    }

    memcpy(buffer->data + buffer->length, bytes, length);
    buffer->length += length;

}

void guac_dbshell_buffer_append_string(guac_dbshell_buffer* buffer,
        const char* string) {
    guac_dbshell_buffer_append(buffer, string, strlen(string));
}

void guac_dbshell_buffer_append_repeat(guac_dbshell_buffer* buffer, char c,
        int count) {

    for (int i = 0; i < count; i++)
        guac_dbshell_buffer_append(buffer, &c, 1);

}

void guac_dbshell_buffer_appendf(guac_dbshell_buffer* buffer,
        const char* format, ...) {

    char text[64];

    va_list args;
    va_start(args, format);
    int length = vsnprintf(text, sizeof(text), format, args);
    va_end(args);

    if (length > (int) sizeof(text) - 1)
        length = sizeof(text) - 1;

    if (length > 0)
        guac_dbshell_buffer_append(buffer, text, length);

}
