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
