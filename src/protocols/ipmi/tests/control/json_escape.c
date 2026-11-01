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

#include "control.h"

#include <CUnit/CUnit.h>
#include <string.h>

/**
 * Verifies that a string requiring no escaping is copied unchanged.
 */
void test_json_escape__plain(void) {
    char out[64];
    guac_ipmi_control_json_escape(out, sizeof(out), "power-on");
    CU_ASSERT_STRING_EQUAL(out, "power-on");
}

/**
 * Verifies that double quotes and backslashes are backslash-escaped, so the
 * result is safe to embed within a JSON string literal.
 */
void test_json_escape__quote_backslash(void) {
    char out[64];
    guac_ipmi_control_json_escape(out, sizeof(out), "a\"b\\c");
    CU_ASSERT_STRING_EQUAL(out, "a\\\"b\\\\c");
}

/**
 * Verifies that newline, carriage return, and tab are converted to their
 * two-character JSON escape sequences.
 */
void test_json_escape__whitespace_escapes(void) {
    char out[64];
    guac_ipmi_control_json_escape(out, sizeof(out), "a\nb\rc\td");
    CU_ASSERT_STRING_EQUAL(out, "a\\nb\\rc\\td");
}

/**
 * Verifies that other (non-printable) control characters are dropped rather
 * than emitted raw, which would produce invalid JSON.
 */
void test_json_escape__drops_control_chars(void) {
    char out[64];
    guac_ipmi_control_json_escape(out, sizeof(out), "a\x01\x1f" "b");
    CU_ASSERT_STRING_EQUAL(out, "ab");
}

/**
 * Verifies that the output is bounded by the destination buffer and remains
 * null-terminated even when the input (or an escape sequence) would overflow.
 */
void test_json_escape__bounds(void) {
    char out[4];
    guac_ipmi_control_json_escape(out, sizeof(out), "abcdefgh");
    CU_ASSERT(strlen(out) <= 3);
}
