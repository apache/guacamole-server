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

#ifndef GUAC_STRING_H
#define GUAC_STRING_H

/**
 * Provides convenience functions for manipulating strings.
 *
 * @file string.h
 */

#include <stddef.h>
#include <string.h>

/**
 * Copies a limited number of bytes from the given source string to the given
 * destination buffer. The resulting buffer will always be null-terminated,
 * even if doing so means that the intended string is truncated, unless the
 * destination buffer has no space available at all. As this function always
 * returns the length of the string it tried to create (the length of the
 * source string), whether truncation has occurred can be detected by comparing
 * the return value against the size of the destination buffer. If the value
 * returned is greater than or equal to the size of the destination buffer, then
 * the string has been truncated.
 *
 * The source and destination buffers MAY NOT overlap.
 *
 * @param dest
 *     The buffer which should receive the contents of the source string. This
 *     buffer will always be null terminated unless zero bytes are available
 *     within the buffer.
 *
 * @param src
 *     The source string to copy into the destination buffer. This string MUST
 *     be null terminated.
 *
 * @param n
 *     The number of bytes available within the destination buffer. If this
 *     value is zero, no bytes will be written to the destination buffer, and
 *     the destination buffer may not be null terminated. In all other cases,
 *     the destination buffer will always be null terminated, even if doing
 *     so means that the copied data from the source string will be truncated.
 *
 * @return
 *     The length of the copied string (the source string) in bytes, excluding
 *     the null terminator.
 */
size_t guac_strlcpy(char* restrict dest, const char* restrict src, size_t n);

/**
 * Appends the given source string after the end of the given destination
 * string, writing at most the given number of bytes. Both the source and
 * destination strings MUST be null-terminated. The resulting buffer will
 * always be null-terminated, even if doing so means that the intended string
 * is truncated, unless the destination buffer has no space available at all.
 * As this function always returns the length of the string it tried to create
 * (the length of destination and source strings added together), whether
 * truncation has occurred can be detected by comparing the return value
 * against the size of the destination buffer. If the value returned is greater
 * than or equal to the size of the destination buffer, then the string has
 * been truncated.
 *
 * The source and destination buffers MAY NOT overlap.
 *
 * @param dest
 *     The buffer which should be appended with the contents of the source
 *     string. This buffer MUST already be null-terminated and will always be
 *     null-terminated unless zero bytes are available within the buffer.
 *
 *     As a safeguard against incorrectly-written code, in the event that the
 *     destination buffer is not null-terminated, this function will still stop
 *     before overrunning the buffer, instead behaving as if the length of the
 *     string in the buffer is exactly the size of the buffer. The destination
 *     buffer will remain untouched (and unterminated) in this case.
 *
 * @param src
 *     The source string to append to the the destination buffer. This string
 *     MUST be null-terminated.
 *
 * @param n
 *     The number of bytes available within the destination buffer. If this
 *     value is not greater than zero, no bytes will be written to the
 *     destination buffer, and the destination buffer may not be
 *     null-terminated. In all other cases, the destination buffer will always
 *     be null-terminated, even if doing so means that the copied data from the
 *     source string will be truncated.
 *
 * @return
 *     The length of the string this function tried to create (the lengths of
 *     the source and destination strings added together) in bytes, excluding
 *     the null terminator.
 */
size_t guac_strlcat(char* restrict dest, const char* restrict src, size_t n);

/**
 * Concatenates each of the given strings, separated by the given delimiter,
 * storing the result within a destination buffer. The number of bytes written
 * will be no more than the given number of bytes, and the destination buffer
 * is guaranteed to be null-terminated, even if doing so means that one or more
 * of the intended strings are truncated or omitted from the end of the result,
 * unless the destination buffer has no space available at all. As this
 * function always returns the length of the string it tried to create (the
 * length of all source strings and all delimiters added together), whether
 * truncation has occurred can be detected by comparing the return value
 * against the size of the destination buffer. If the value returned is greater
 * than or equal to the size of the destination buffer, then the string has
 * been truncated.
 *
 * The source strings, delimiter string, and destination buffer MAY NOT
 * overlap.
 *
 * @param dest
 *     The buffer which should receive the result of joining the given strings.
 *     This buffer will always be null terminated unless zero bytes are
 *     available within the buffer.
 *
 * @param elements
 *     The elements to concatenate together, separated by the given delimiter.
 *     Each element MUST be null-terminated.
 *
 * @param nmemb
 *     The number of elements within the elements array.
 *
 * @param delim
 *     The delimiter to include between each pair of elements.
 *
 * @param n
 *     The number of bytes available within the destination buffer. If this
 *     value is not greater than zero, no bytes will be written to the
 *     destination buffer, and the destination buffer may not be null
 *     terminated. In all other cases, the destination buffer will always be
 *     null terminated, even if doing so means that the result will be
 *     truncated.
 *
 * @return
 *     The length of the string this function tried to create (the length of
 *     all source strings and all delimiters added together) in bytes,
 *     excluding the null terminator.
 */
size_t guac_strljoin(char* restrict dest, const char* restrict const* elements,
        int nmemb, const char* restrict delim, size_t n);

#endif

