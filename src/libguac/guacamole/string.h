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

#endif

