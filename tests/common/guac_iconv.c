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

#include "common_suite.h"
#include "guac_iconv.h"

#include <stdlib.h>
#include <CUnit/Basic.h>

static void test_conversion(
        guac_iconv_read* reader,  unsigned char* in_string,  int in_length,
        guac_iconv_write* writer, unsigned char* out_string, int out_length) {

    char output[4096];
    char input[4096];

    const char* current_input = input;
    char* current_output = output;

    memcpy(input, in_string, in_length);
    guac_iconv(reader, &current_input,  sizeof(input),
               writer, &current_output, sizeof(output));

    /* Verify output length */
    CU_ASSERT_EQUAL(out_length, current_output - output);

    /* Verify entire input read */
    CU_ASSERT_EQUAL(in_length, current_input - input);

    /* Verify output content */
    CU_ASSERT_EQUAL(0, memcmp(output, out_string, out_length));

}

void test_guac_iconv() {

    /* UTF8 for "papà è bello" */
    unsigned char test_string_utf8[] = {
        'p',  'a',  'p', 0xC3, 0xA0, ' ',
        0xC3, 0xA8, ' ',
        'b',  'e',  'l', 'l',  'o',
        0x00
    };

    /* UTF16 for "papà è bello" */
    unsigned char test_string_utf16[] = {
        'p',  0x00, 'a', 0x00, 'p', 0x00, 0xE0, 0x00, ' ', 0x00,
        0xE8, 0x00, ' ', 0x00,
        'b',  0x00, 'e', 0x00, 'l', 0x00, 'l',  0x00, 'o', 0x00,
        0x00, 0x00
    };

    /* ISO-8859-1 for "papà è bello" */
    unsigned char test_string_iso8859_1[] = {
        'p',  'a',  'p', 0xE0, ' ',
        0xE8, ' ',
        'b',  'e',  'l', 'l',  'o',
        0x00
    };

    /* CP1252 for "papà è bello" */
    unsigned char test_string_cp1252[] = {
        'p',  'a',  'p', 0xE0, ' ',
        0xE8, ' ',
        'b',  'e',  'l', 'l',  'o',
        0x00
    };

    /* UTF8 identity */
    test_conversion(
            GUAC_READ_UTF8,  test_string_utf8, sizeof(test_string_utf8),
            GUAC_WRITE_UTF8, test_string_utf8, sizeof(test_string_utf8));

    /* UTF16 identity */
    test_conversion(
            GUAC_READ_UTF16,  test_string_utf16, sizeof(test_string_utf16),
            GUAC_WRITE_UTF16, test_string_utf16, sizeof(test_string_utf16));

    /* UTF8 to UTF16 */
    test_conversion(
            GUAC_READ_UTF8,   test_string_utf8,  sizeof(test_string_utf8),
            GUAC_WRITE_UTF16, test_string_utf16, sizeof(test_string_utf16));

    /* UTF16 to UTF8 */
    test_conversion(
            GUAC_READ_UTF16, test_string_utf16, sizeof(test_string_utf16),
            GUAC_WRITE_UTF8, test_string_utf8,  sizeof(test_string_utf8));

    /* UTF16 to ISO-8859-1 */
    test_conversion(
            GUAC_READ_UTF16,      test_string_utf16,      sizeof(test_string_utf16),
            GUAC_WRITE_ISO8859_1, test_string_iso8859_1,  sizeof(test_string_iso8859_1));

    /* UTF16 to CP1252 */
    test_conversion(
            GUAC_READ_UTF16,   test_string_utf16,  sizeof(test_string_utf16),
            GUAC_WRITE_CP1252, test_string_cp1252, sizeof(test_string_cp1252));

    /* CP1252 to UTF8 */
    test_conversion(
            GUAC_READ_CP1252, test_string_cp1252, sizeof(test_string_cp1252),
            GUAC_WRITE_UTF8,  test_string_utf8,   sizeof(test_string_utf8));

    /* ISO-8859-1 to UTF8 */
    test_conversion(
            GUAC_READ_ISO8859_1, test_string_iso8859_1, sizeof(test_string_iso8859_1),
            GUAC_WRITE_UTF8,     test_string_utf8,      sizeof(test_string_utf8));

}

