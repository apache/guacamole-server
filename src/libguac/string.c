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

#include <stddef.h>
#include <string.h>

size_t guac_strlcpy(char* restrict dest, const char* restrict src, size_t n) {

#ifdef HAVE_STRLCPY
    return strlcpy(dest, src, n);
#else
    /* Calculate actual length of desired string */
    size_t length = strlen(src);

    /* Copy nothing if there is no space */
    if (n <= 0)
        return length;

    /* Calculate length of the string which will be copied */
    size_t copy_length = length;
    if (copy_length >= n)
        copy_length = n - 1;

    /* Copy only as much of string as possible, manually adding a null
     * terminator */
    memcpy(dest, src, copy_length);
    dest[copy_length] = '\0';

    /* Return the overall length of the desired string */
    return length;
#endif

}

size_t guac_strlcat(char* restrict dest, const char* restrict src, size_t n) {

#ifdef HAVE_STRLCPY
    return strlcat(dest, src, n);
#else
    int length = strnlen(dest, n);
    return length + guac_strlcpy(dest + length, src, n - length);
#endif

}

size_t guac_strljoin(char* restrict dest, const char* restrict const* elements,
        int nmemb, const char* restrict delim, size_t n) {

    int length = 0;
    const char* restrict const* current = elements;

    /* If no elements are provided, nothing to do but ensure the destination
     * buffer is null terminated */
    if (nmemb <= 0)
        return guac_strlcpy(dest, "", n);

    /* Initialize destination buffer with first element */
    length += guac_strlcpy(dest, *current, n);

    /* Copy all remaining elements, separated by delimiter */
    for (current++; nmemb > 1; current++, nmemb--) {
        length += guac_strlcat(dest + length, delim, n - length);
        length += guac_strlcat(dest + length, *current, n - length);
    }

    return length;

}

