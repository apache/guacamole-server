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

#include <stdint.h>

#include <guacamole/unicode.h>

void guac_rdp_utf16_to_utf8(const unsigned char* utf16, int length,
        char* utf8, int size) {

    int i;
    const uint16_t* in_codepoint = (const uint16_t*) utf16;

    /* For each UTF-16 character */
    for (i=0; i<length; i++) {

        /* Get next codepoint */
        uint16_t codepoint = *(in_codepoint++);

        /* Save codepoint as UTF-8 */
        int bytes_written = guac_utf8_write(codepoint, utf8, size);
        size -= bytes_written;
        utf8 += bytes_written;

    }

    /* Save NULL terminator */
    *(utf8++) = 0;

}

void guac_rdp_utf8_to_utf16(const unsigned char* utf8, int length,
        char* utf16, int size) {

    int i;
    uint16_t* out_codepoint = (uint16_t*) utf16;

    /* For each UTF-8 character */
    for (i=0; i<length; i++) {

        /* Get next codepoint */
        int codepoint;
        utf8 += guac_utf8_read((const char*) utf8, 4, &codepoint);

        /* Save codepoint as UTF-16 */
        *(out_codepoint++) = codepoint;

        /* Stop if buffer full */
        size -= 2;
        if (size < 2)
            break;

    }

}

