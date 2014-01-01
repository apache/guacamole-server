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

#include "suite.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <CUnit/Basic.h>
#include <guacamole/protocol.h>

void test_base64_decode() {

    /* Test strings */
    char test_HELLO[]     = "SEVMTE8=";
    char test_AVOCADO[]   = "QVZPQ0FETw==";
    char test_GUACAMOLE[] = "R1VBQ0FNT0xF";

    /* Invalid strings */
    char invalid1[] = "====";
    char invalid2[] = "";

    /* Test one character of padding */
    CU_ASSERT_EQUAL(guac_protocol_decode_base64(test_HELLO), 5);
    CU_ASSERT_NSTRING_EQUAL(test_HELLO, "HELLO", 5);

    /* Test two characters of padding */
    CU_ASSERT_EQUAL(guac_protocol_decode_base64(test_AVOCADO), 7);
    CU_ASSERT_NSTRING_EQUAL(test_AVOCADO, "AVOCADO", 7);

    /* Test three characters of padding */
    CU_ASSERT_EQUAL(guac_protocol_decode_base64(test_GUACAMOLE), 9);
    CU_ASSERT_NSTRING_EQUAL(test_GUACAMOLE, "GUACAMOLE", 9);

    /* Verify invalid strings stop early as expected */
    CU_ASSERT_EQUAL(guac_protocol_decode_base64(invalid1), 0);
    CU_ASSERT_EQUAL(guac_protocol_decode_base64(invalid2), 0);

}

