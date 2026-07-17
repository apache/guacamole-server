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
 * Verifies that guac_ipmi_control_json_get() extracts a simple string value.
 */
void test_json_get__basic(void) {
    char out[64];
    CU_ASSERT(guac_ipmi_control_json_get(
            "{\"command\":\"power-on\"}", "command", out, sizeof(out)));
    CU_ASSERT_STRING_EQUAL(out, "power-on");
}

/**
 * Verifies that a token appearing only as a value is NOT matched as if it were
 * an object key. This is the property that keeps a command name embedded in a
 * value (e.g. the "command" in {"type":"command"}) from being misread as the
 * "command" key.
 */
void test_json_get__value_not_matched_as_key(void) {
    char out[64];
    CU_ASSERT_FALSE(guac_ipmi_control_json_get(
            "{\"type\":\"command\"}", "command", out, sizeof(out)));
}

/**
 * Verifies that whitespace around the key, colon, and value is tolerated.
 */
void test_json_get__whitespace(void) {
    char out[64];
    CU_ASSERT(guac_ipmi_control_json_get(
            "{ \"id\" : \"c7\" }", "id", out, sizeof(out)));
    CU_ASSERT_STRING_EQUAL(out, "c7");
}

/**
 * Verifies that backslash escapes within the value are unescaped.
 */
void test_json_get__escaped_value(void) {
    char out[64];
    CU_ASSERT(guac_ipmi_control_json_get(
            "{\"m\":\"a\\\"b\\\\c\"}", "m", out, sizeof(out)));
    CU_ASSERT_STRING_EQUAL(out, "a\"b\\c");
}

/**
 * Verifies that a missing key yields no match.
 */
void test_json_get__missing(void) {
    char out[64];
    CU_ASSERT_FALSE(guac_ipmi_control_json_get(
            "{\"type\":\"command\"}", "nope", out, sizeof(out)));
}

/**
 * Verifies that a bare numeric value is extracted literally. Clients are free
 * to send unquoted scalars, so a numeric value must not read as a missing key.
 */
void test_json_get__numeric_value(void) {
    char out[64];
    CU_ASSERT(guac_ipmi_control_json_get(
            "{\"n\":123}", "n", out, sizeof(out)));
    CU_ASSERT_STRING_EQUAL(out, "123");
}

/**
 * Verifies that bare boolean and null values are extracted literally, and that
 * a scalar is correctly delimited when followed by a further key.
 */
void test_json_get__literal_values(void) {

    char out[64];

    CU_ASSERT(guac_ipmi_control_json_get(
            "{\"a\":true,\"b\":2}", "a", out, sizeof(out)));
    CU_ASSERT_STRING_EQUAL(out, "true");

    CU_ASSERT(guac_ipmi_control_json_get(
            "{\"a\":false}", "a", out, sizeof(out)));
    CU_ASSERT_STRING_EQUAL(out, "false");

    CU_ASSERT(guac_ipmi_control_json_get(
            "{ \"a\" : null }", "a", out, sizeof(out)));
    CU_ASSERT_STRING_EQUAL(out, "null");

}

/**
 * Verifies that a negative, fractional scalar survives intact, as such a value
 * must not be truncated at the sign or decimal point.
 */
void test_json_get__negative_value(void) {
    char out[64];
    CU_ASSERT(guac_ipmi_control_json_get(
            "{\"t\":-12.5,\"u\":1}", "t", out, sizeof(out)));
    CU_ASSERT_STRING_EQUAL(out, "-12.5");
}

/**
 * Verifies that object and array values yield no match, rather than a fragment
 * of their contents, as only flat objects are supported.
 */
void test_json_get__structured_value(void) {

    char out[64];

    CU_ASSERT_FALSE(guac_ipmi_control_json_get(
            "{\"o\":{\"k\":\"v\"}}", "o", out, sizeof(out)));

    CU_ASSERT_FALSE(guac_ipmi_control_json_get(
            "{\"a\":[1,2]}", "a", out, sizeof(out)));

}

/**
 * Verifies that a key with no value at all yields no match.
 */
void test_json_get__empty_value(void) {
    char out[64];
    CU_ASSERT_FALSE(guac_ipmi_control_json_get(
            "{\"n\":}", "n", out, sizeof(out)));
}

/**
 * Verifies that the extracted value is bounded by the output buffer and is
 * always null-terminated, never overflowing.
 */
void test_json_get__bounds(void) {
    char out[4];
    CU_ASSERT(guac_ipmi_control_json_get(
            "{\"k\":\"abcdefgh\"}", "k", out, sizeof(out)));
    CU_ASSERT_EQUAL(strlen(out), 3);
    CU_ASSERT_STRING_EQUAL(out, "abc");
}
