/*
 * Copyright (C) 2013 Glyptodon LLC
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

#include "guac_string.h"

#include <stdlib.h>
#include <string.h>

int guac_count_occurrences(const char* string, char c) {

    int count = 0;

    while (*string != 0) {

        /* Count each occurrence */
        if (*string == c)
            count++;

        /* Next character */
        string++;

    }

    return count;

}

char** guac_split(const char* string, char delim) {

    int i = 0;

    int token_count = guac_count_occurrences(string, delim) + 1;
    const char* token_start = string;

    /* Allocate space for tokens */
    char** tokens = malloc(sizeof(char*) * (token_count+1));

    do {

        int length;
        char* token;

        /* Find end of token */
        while (*string != 0 && *string != delim)
            string++;

        /* Calculate token length */
        length = string - token_start;

        /* Allocate space for token and NULL terminator */
        tokens[i++] = token = malloc(length + 1);

        /* Copy token, store null */
        memcpy(token, token_start, length);
        token[length] = 0;

        /* Stop at end of string */
        if (*string == 0)
            break;

        /* Next token */
        token_start = ++string;

    } while (i < token_count);

    /* NULL terminator */
    tokens[i] = NULL;

    return tokens;

}

