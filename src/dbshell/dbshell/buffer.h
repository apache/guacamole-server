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

#ifndef GUAC_DBSHELL_BUFFER_H
#define GUAC_DBSHELL_BUFFER_H

/**
 * Declarations for the dynamically-sized byte buffer used throughout the
 * dbshell library to accumulate terminal output before writing it in a
 * single call.
 *
 * @file buffer.h
 */

/**
 * A dynamically-sized byte buffer.
 */
typedef struct guac_dbshell_buffer {

    /**
     * The accumulated bytes. This pointer is NULL until the first append.
     */
    char* data;

    /**
     * The number of accumulated bytes.
     */
    int length;

    /**
     * The number of bytes currently allocated.
     */
    int allocated;

} guac_dbshell_buffer;

/**
 * Initializes the given buffer to an empty state.
 *
 * @param buffer
 *     The buffer to initialize.
 */
void guac_dbshell_buffer_init(guac_dbshell_buffer* buffer);

/**
 * Releases the storage of the given buffer.
 *
 * @param buffer
 *     The buffer to release.
 */
void guac_dbshell_buffer_destroy(guac_dbshell_buffer* buffer);

/**
 * Appends the given bytes to the given buffer, expanding its storage as
 * necessary.
 *
 * @param buffer
 *     The buffer to append to.
 *
 * @param bytes
 *     The bytes to append.
 *
 * @param length
 *     The number of bytes to append.
 */
void guac_dbshell_buffer_append(guac_dbshell_buffer* buffer,
        const char* bytes, int length);

/**
 * Appends the given null-terminated string to the given buffer.
 *
 * @param buffer
 *     The buffer to append to.
 *
 * @param string
 *     The null-terminated string to append.
 */
void guac_dbshell_buffer_append_string(guac_dbshell_buffer* buffer,
        const char* string);

/**
 * Appends the given character to the given buffer the given number of
 * times.
 *
 * @param buffer
 *     The buffer to append to.
 *
 * @param c
 *     The character to append.
 *
 * @param count
 *     The number of copies of the character to append. Values less than
 *     one result in no change.
 */
void guac_dbshell_buffer_append_repeat(guac_dbshell_buffer* buffer, char c,
        int count);

/**
 * Appends the given printf-style formatted text to the given buffer. The
 * formatted text must not exceed 63 bytes; this function is intended only
 * for short terminal control sequences and numbers.
 *
 * @param buffer
 *     The buffer to append to.
 *
 * @param format
 *     The printf-style format string.
 *
 * @param ...
 *     Any arguments to use when filling the format string.
 */
void guac_dbshell_buffer_appendf(guac_dbshell_buffer* buffer,
        const char* format, ...);

#endif
