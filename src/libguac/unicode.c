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

#include "unicode.h"

#include <stddef.h>

size_t guac_utf8_charsize(unsigned char c) {

    /* Determine size in bytes of character */
    if ((c | 0x7F) == 0x7F) return 1;
    if ((c | 0x1F) == 0xDF) return 2;
    if ((c | 0x0F) == 0xEF) return 3;
    if ((c | 0x07) == 0xF7) return 4;

    /* Default to one character */
    return 1;

}

size_t guac_utf8_strlen(const char* str) {

    /* The current length of the string */
    int length = 0;

    /* Number of characters before start of next character */
    int skip = 0;

    while (*str != 0) {

        /* If skipping, then skip */
        if (skip > 0) skip--;

        /* Otherwise, determine next skip value, and increment length */
        else {

            /* Get next character */
            unsigned char c = (unsigned char) *str;

            /* Determine skip value (size in bytes of rest of character) */
            skip = guac_utf8_charsize(c) - 1;

            length++;
        }

        str++;
    }

    return length;

}

int guac_utf8_write(int codepoint, char* utf8, int length) {

    int i;
    int mask, bytes;

    /* If not even one byte, cannot write */
    if (length <= 0)
        return 0;

    /* Determine size and initial byte mask */
    if (codepoint <= 0x007F) {
        mask  = 0x00;
        bytes = 1;
    }
    else if (codepoint <= 0x7FF) {
        mask  = 0xC0;
        bytes = 2;
    }
    else if (codepoint <= 0xFFFF) {
        mask  = 0xE0;
        bytes = 3;
    }
    else if (codepoint <= 0x1FFFFF) {
        mask  = 0xF0;
        bytes = 4;
    }

    /* Otherwise, invalid codepoint */
    else {
        *(utf8++) = '?';
        return 1;
    }

    /* If not enough room, don't write anything */
    if (bytes > length)
        return 0;

    /* Offset buffer by size */
    utf8 += bytes - 1;

    /* Add trailing bytes, if any */
    for (i=1; i<bytes; i++) {
        *(utf8--) = 0x80 | (codepoint & 0x3F);
        codepoint >>= 6;
    }

    /* Set initial byte */
    *utf8 = mask | codepoint;

    /* Done */
    return bytes;

}

int guac_utf8_read(const char* utf8, int length, int* codepoint) {

    unsigned char initial;
    int bytes;
    int result;
    int i;

    /* If not even one byte, cannot read */
    if (length <= 0)
        return 0;

    /* Read initial byte */
    initial = (unsigned char) *(utf8++);

    /* 0xxxxxxx */
    if ((initial | 0x7F) == 0x7F) {
        result = initial;
        bytes  = 1;
    }

    /* 110xxxxx 10xxxxxx */
    else if ((initial | 0x1F) == 0xDF) {
        result = initial & 0x1F;
        bytes  = 2;
    }

    /* 1110xxxx 10xxxxxx 10xxxxxx */
    else if ((initial | 0x0F) == 0xEF) {
        result = initial & 0x0F;
        bytes  = 3;
    }

    /* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
    else if ((initial | 0x07) == 0xF7) {
        result = initial & 0x07;
        bytes  = 4;
    }

    /* Otherwise, invalid codepoint */
    else {
        *codepoint = 0xFFFD; /* Replacement character */
        return 1;
    }

    /* If not enough room, don't read anything */
    if (bytes > length)
        return 0;

    /* Read trailing bytes, if any */
    for (i=1; i<bytes; i++) {
        result <<= 6;
        result |= *(utf8++) & 0x3F;
    }

    *codepoint = result;
    return bytes;

}

