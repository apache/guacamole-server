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

#include "unicode.h"

#include <CUnit/CUnit.h>
#include <stdint.h>

/**
 * Test which verifies that UTF-16 data is correctly converted to UTF-8.
 */
void test_unicode__utf16_to_utf8(void) {

    const uint16_t utf16[] = { 'h', 'e', 'l', 'l', 'o' };
    char utf8[16];

    guac_rdp_utf16_to_utf8((const unsigned char*) utf16, 5,
            utf8, sizeof(utf8));

    CU_ASSERT_STRING_EQUAL(utf8, "hello");

}

/**
 * Test which verifies that conversion succeeds when the converted string
 * and its NULL terminator exactly fill the provided buffer.
 */
void test_unicode__utf16_to_utf8_exact_fit(void) {

    const uint16_t utf16[] = { 'h', 'i' };
    char utf8[3];

    guac_rdp_utf16_to_utf8((const unsigned char*) utf16, 2,
            utf8, sizeof(utf8));

    CU_ASSERT_STRING_EQUAL(utf8, "hi");

}

/**
 * Test which verifies that conversion does not write past the end of the
 * buffer when the buffer is large enough for the converted data, but not
 * for the data and its NULL terminator.
 */
void test_unicode__utf16_to_utf8_no_room_for_terminator(void) {

    /* U+00E9 ("é") is 2 bytes in UTF-8, leaving no room for a terminator */
    const uint16_t utf16[] = { 0x00E9 };
    struct {
        char buffer[2];
        char canary;
    } out = { { 0x55, 0x55 }, 0x7F };

    guac_rdp_utf16_to_utf8((const unsigned char*) utf16, 1,
            out.buffer, sizeof(out.buffer));

    CU_ASSERT_EQUAL(out.buffer[0], '\0');
    CU_ASSERT_EQUAL(out.buffer[1], 0x55);
    CU_ASSERT_EQUAL(out.canary, 0x7F);

}

/**
 * Test which verifies that conversion truncates at a UTF-8 character
 * boundary rather than writing past the end of the buffer.
 */
void test_unicode__utf16_to_utf8_truncates(void) {

    /* "aé" is 3 bytes in UTF-8, but the buffer can only hold "a" + NUL */
    const uint16_t utf16[] = { 'a', 0x00E9 };
    struct {
        char buffer[3];
        char canary;
    } out = { { 0x55, 0x55, 0x55 }, 0x7F };

    guac_rdp_utf16_to_utf8((const unsigned char*) utf16, 2,
            out.buffer, sizeof(out.buffer));

    CU_ASSERT_STRING_EQUAL(out.buffer, "a");
    CU_ASSERT_EQUAL(out.canary, 0x7F);

}

/**
 * Test which verifies that no bytes are written if the provided buffer has
 * no space at all.
 */
void test_unicode__utf16_to_utf8_zero_size(void) {

    const uint16_t utf16[] = { 'a' };
    char canary = 0x7F;

    guac_rdp_utf16_to_utf8((const unsigned char*) utf16, 1, &canary, 0);

    CU_ASSERT_EQUAL(canary, 0x7F);

}
