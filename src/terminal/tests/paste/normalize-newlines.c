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

#include "terminal/terminal.h"

#include <CUnit/CUnit.h>
#include <string.h>

/**
 * Runs guac_terminal_paste_normalize_newlines() against the given input and
 * verifies both the returned length and the resulting bytes match expected.
 */
static void assert_normalized(const char* in, int in_len,
        const char* expected, int expected_len) {

    char out[64];
    int written = guac_terminal_paste_normalize_newlines(in, in_len, out);

    CU_ASSERT_EQUAL(expected_len, written);
    CU_ASSERT_EQUAL(0, memcmp(out, expected, expected_len));

}

/**
 * Verifies that bare LF is rewritten as CR.
 */
void test_paste__lf_to_cr() {
    assert_normalized("a\nb", 3, "a\rb", 3);
}

/**
 * Verifies that CRLF pairs collapse to a single CR (not CRCR).
 */
void test_paste__crlf_to_single_cr() {
    assert_normalized("a\r\nb", 4, "a\rb", 3);
}

/**
 * Verifies that bare CR passes through as CR.
 */
void test_paste__bare_cr_passthrough() {
    assert_normalized("a\rb", 3, "a\rb", 3);
}

/**
 * Verifies that a mixed sequence of LF, CRLF, and bare CR each become exactly
 * one CR.
 */
void test_paste__mixed_endings() {
    assert_normalized("a\nb\r\nc\rd", 8, "a\rb\rc\rd", 7);
}

/**
 * Verifies that consecutive line endings each produce one CR. Catches
 * cursor-advancement errors that would let one CRLF's LF be consumed by the
 * next iteration.
 */
void test_paste__consecutive_endings() {
    assert_normalized("\n\n", 2, "\r\r", 2);
    assert_normalized("\r\n\r\n", 4, "\r\r", 2);
    assert_normalized("\r\r", 2, "\r\r", 2);
    assert_normalized("\n\r\n\r", 4, "\r\r\r", 3);
}

/**
 * Verifies that a trailing bare CR at end of input is emitted without reading
 * past the buffer.
 */
void test_paste__trailing_cr() {
    assert_normalized("abc\r", 4, "abc\r", 4);
}

/**
 * Verifies that an input with no line endings is copied verbatim.
 */
void test_paste__no_endings() {
    assert_normalized("hello world", 11, "hello world", 11);
}

/**
 * Verifies that zero-length input writes nothing.
 */
void test_paste__empty() {
    char out[1] = { 0x7F };
    int written = guac_terminal_paste_normalize_newlines("", 0, out);
    CU_ASSERT_EQUAL(0, written);
    CU_ASSERT_EQUAL((char) 0x7F, out[0]);
}

/**
 * Verifies that embedded NUL bytes are preserved (the function is byte-driven,
 * not C-string driven).
 */
void test_paste__embedded_nul() {
    const char in[]       = { 'a', '\0', '\n', 'b' };
    const char expected[] = { 'a', '\0', '\r', 'b' };
    assert_normalized(in, sizeof(in), expected, sizeof(expected));
}
