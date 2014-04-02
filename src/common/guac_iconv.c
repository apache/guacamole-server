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
#include "guac_iconv.h"

#include <guacamole/unicode.h>
#include <stdint.h>

int guac_iconv(guac_iconv_read* reader, char** input, int in_remaining,
               guac_iconv_write* writer, char** output, int out_remaining) {

    while (in_remaining > 0 && out_remaining > 0) {

        int value;
        char* read_start;
        char* write_start;

        /* Read character */
        read_start = *input;
        value = reader(input, in_remaining);
        in_remaining -= *input - read_start;

        /* Write character */
        write_start = *output;
        writer(output, out_remaining, value);
        out_remaining -= *output - write_start;

        /* Stop if null terminator reached */
        if (value == 0)
            return 1;

    }

    /* Null terminator not reached */
    return 0;

}

int GUAC_READ_UTF8(char** input, int remaining) {

    int value;

    *input += guac_utf8_read(*input, remaining, &value);
    return value;

}

int GUAC_READ_UTF16(char** input, int remaining) {

    int value;

    /* Bail if not enough data */
    if (remaining < 2)
        return 0;

    /* Read two bytes as integer */
    value = *((uint16_t*) *input);
    *input += 2;

    return value;

}

void GUAC_WRITE_UTF8(char** output, int remaining, int value) {
    *output += guac_utf8_write(value, *output, remaining);
}

void GUAC_WRITE_UTF16(char** output, int remaining, int value) {

    /* Bail if not enough data */
    if (remaining < 2)
        return;

    /* Write two bytes as integer */
    *((uint16_t*) *output) = value;
    *output += 2;

}

