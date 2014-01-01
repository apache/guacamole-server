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

#include "util_suite.h"

#include <CUnit/Basic.h>
#include <guacamole/unicode.h>

void test_guac_unicode() {

    int codepoint;
    char buffer[16];

    /* Test character length */
    CU_ASSERT_EQUAL(1, guac_utf8_charsize(UTF8_1b[0]));
    CU_ASSERT_EQUAL(2, guac_utf8_charsize(UTF8_2b[0]));
    CU_ASSERT_EQUAL(3, guac_utf8_charsize(UTF8_3b[0]));
    CU_ASSERT_EQUAL(4, guac_utf8_charsize(UTF8_4b[0]));

    /* Test string length */
    CU_ASSERT_EQUAL(0, guac_utf8_strlen(""));
    CU_ASSERT_EQUAL(1, guac_utf8_strlen(UTF8_4b));
    CU_ASSERT_EQUAL(2, guac_utf8_strlen(UTF8_4b UTF8_1b));
    CU_ASSERT_EQUAL(2, guac_utf8_strlen(UTF8_2b UTF8_3b));
    CU_ASSERT_EQUAL(3, guac_utf8_strlen(UTF8_1b UTF8_3b UTF8_4b));
    CU_ASSERT_EQUAL(3, guac_utf8_strlen(UTF8_2b UTF8_1b UTF8_3b));
    CU_ASSERT_EQUAL(3, guac_utf8_strlen(UTF8_4b UTF8_2b UTF8_1b));
    CU_ASSERT_EQUAL(3, guac_utf8_strlen(UTF8_3b UTF8_4b UTF8_2b));
    CU_ASSERT_EQUAL(5, guac_utf8_strlen("hello"));
    CU_ASSERT_EQUAL(9, guac_utf8_strlen("guacamole"));

    /* Test writes */
    CU_ASSERT_EQUAL(1, guac_utf8_write(0x00065, &(buffer[0]),  10));
    CU_ASSERT_EQUAL(2, guac_utf8_write(0x00654, &(buffer[1]),   9));
    CU_ASSERT_EQUAL(3, guac_utf8_write(0x00876, &(buffer[3]),   7));
    CU_ASSERT_EQUAL(4, guac_utf8_write(0x12345, &(buffer[6]),   4));
    CU_ASSERT_EQUAL(0, guac_utf8_write(0x00066, &(buffer[10]),  0));

    /* Test result of write */
    CU_ASSERT(memcmp("\x65",             &(buffer[0]), 1) == 0); /* U+0065  */
    CU_ASSERT(memcmp("\xD9\x94",         &(buffer[1]), 2) == 0); /* U+0654  */
    CU_ASSERT(memcmp("\xE0\xA1\xB6",     &(buffer[3]), 3) == 0); /* U+0876  */
    CU_ASSERT(memcmp("\xF0\x92\x8D\x85", &(buffer[6]), 4) == 0); /* U+12345 */

    /* Test reads */

    CU_ASSERT_EQUAL(1, guac_utf8_read(&(buffer[0]), 10, &codepoint));
    CU_ASSERT_EQUAL(0x0065, codepoint);

    CU_ASSERT_EQUAL(2, guac_utf8_read(&(buffer[1]),  9, &codepoint));
    CU_ASSERT_EQUAL(0x0654, codepoint);

    CU_ASSERT_EQUAL(3, guac_utf8_read(&(buffer[3]),  7, &codepoint));
    CU_ASSERT_EQUAL(0x0876, codepoint);

    CU_ASSERT_EQUAL(4, guac_utf8_read(&(buffer[6]),  4, &codepoint));
    CU_ASSERT_EQUAL(0x12345, codepoint);

    CU_ASSERT_EQUAL(0, guac_utf8_read(&(buffer[10]), 0, &codepoint));
    CU_ASSERT_EQUAL(0x12345, codepoint);

}

