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

#include "dbshell/table.h"

#include <CUnit/CUnit.h>
#include <guacamole/mem.h>
#include <stdlib.h>

/**
 * Verifies that terminal escape sequences within cell data are
 * neutralized, preventing injection of terminal control sequences by the
 * database server.
 */
void test_sanitize__escape_sequences(void) {

    char* sanitized = guac_dbshell_table_sanitize("a\x1B[31mred\x1B[0m");
    CU_ASSERT_STRING_EQUAL(sanitized, "a [31mred [0m");
    guac_mem_free(sanitized);

}

/**
 * Verifies that all C0 control characters and DEL are replaced with
 * spaces while printable characters survive.
 */
void test_sanitize__control_characters(void) {

    char* sanitized = guac_dbshell_table_sanitize(
            "a\tb\nc\rd\x07""e\x7F""f");
    CU_ASSERT_STRING_EQUAL(sanitized, "a b c d e f");
    guac_mem_free(sanitized);

}

/**
 * Verifies that C1 control characters, including the 8-bit form of CSI
 * (U+009B), are replaced with a single space, while other two-byte UTF-8
 * characters whose bytes fall within the same ranges are preserved.
 */
void test_sanitize__c1_controls(void) {

    /* U+009B (CSI) followed by "2J" would clear the screen */
    char* sanitized = guac_dbshell_table_sanitize("a\xC2\x9B""2Jb");
    CU_ASSERT_STRING_EQUAL(sanitized, "a 2Jb");
    guac_mem_free(sanitized);

    /* Bounds of the C1 range */
    sanitized = guac_dbshell_table_sanitize("\xC2\x80\xC2\x9F");
    CU_ASSERT_STRING_EQUAL(sanitized, "  ");
    guac_mem_free(sanitized);

    /* U+00A0 (no-break space) and U+00DB ("\xC3\x9B") are not controls */
    sanitized = guac_dbshell_table_sanitize("\xC2\xA0\xC3\x9B");
    CU_ASSERT_STRING_EQUAL(sanitized, "\xC2\xA0\xC3\x9B");
    guac_mem_free(sanitized);

    /* A trailing lead byte must not be read past the terminator */
    sanitized = guac_dbshell_table_sanitize("x\xC2");
    CU_ASSERT_STRING_EQUAL(sanitized, "x\xC2");
    guac_mem_free(sanitized);

}

/**
 * Verifies that multi-byte UTF-8 content passes through unmodified.
 */
void test_sanitize__utf8_preserved(void) {

    char* sanitized = guac_dbshell_table_sanitize("caf\xC3\xA9 \xE4\xB8\xAD");
    CU_ASSERT_STRING_EQUAL(sanitized, "caf\xC3\xA9 \xE4\xB8\xAD");
    guac_mem_free(sanitized);

}

/**
 * Verifies that empty values survive sanitization.
 */
void test_sanitize__empty(void) {

    char* sanitized = guac_dbshell_table_sanitize("");
    CU_ASSERT_STRING_EQUAL(sanitized, "");
    guac_mem_free(sanitized);

}
