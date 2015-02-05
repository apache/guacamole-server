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
#include "types.h"

#include <stdbool.h>
#include <unistd.h>

int guac_terminal_fit_to_range(int value, int min, int max) {

    if (value < min) return min;
    if (value > max) return max;

    return value;

}

int guac_terminal_encode_utf8(int codepoint, char* utf8) {

    int i;
    int mask, bytes;

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

bool guac_terminal_has_glyph(int codepoint) {
    return
           codepoint != 0
        && codepoint != ' '
        && codepoint != GUAC_CHAR_CONTINUATION;
}

int guac_terminal_write_all(int fd, const char* buffer, int size) {

    int remaining = size;
    while (remaining > 0) {

        /* Attempt to write data */
        int ret_val = write(fd, buffer, remaining);
        if (ret_val <= 0)
            return -1;

        /* If successful, contine with what data remains (if any) */
        remaining -= ret_val;
        buffer += ret_val;

    }

    return size;

}

int guac_terminal_fill_buffer(int fd, char* buffer, int size) {

    int remaining = size;
    while (remaining > 0) {

        /* Attempt to read data */
        int ret_val = read(fd, buffer, remaining);
        if (ret_val <= 0)
            return -1;

        /* If successful, continue with what space remains (if any) */
        remaining -= ret_val;
        buffer += ret_val;

    }

    return size;

}

