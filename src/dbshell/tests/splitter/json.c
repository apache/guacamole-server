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

#include "dbshell/splitter.h"

#include <CUnit/CUnit.h>
#include <guacamole/mem.h>
#include <stdlib.h>
#include <string.h>

/**
 * Verifies that a balanced JSON document on a single line is produced as
 * one statement.
 */
void test_splitter_json__single_line(void) {

    guac_dbshell_splitter* splitter =
        guac_dbshell_splitter_alloc(GUAC_DBSHELL_DIALECT_JSON);

    guac_dbshell_splitter_feed(splitter, "{\"ping\": 1}");

    char* statement = guac_dbshell_splitter_next_statement(splitter);
    CU_ASSERT_PTR_NOT_NULL_FATAL(statement);
    CU_ASSERT_STRING_EQUAL(statement, "{\"ping\": 1}");
    guac_mem_free(statement);

    guac_dbshell_splitter_free(splitter);

}

/**
 * Verifies that a JSON document spanning several lines completes only
 * once its braces balance.
 */
void test_splitter_json__multi_line(void) {

    guac_dbshell_splitter* splitter =
        guac_dbshell_splitter_alloc(GUAC_DBSHELL_DIALECT_JSON);

    guac_dbshell_splitter_feed(splitter, "{\"find\": \"users\",");
    CU_ASSERT_PTR_NULL(guac_dbshell_splitter_next_statement(splitter));
    CU_ASSERT_TRUE(guac_dbshell_splitter_pending(splitter));

    guac_dbshell_splitter_feed(splitter, " \"filter\": {\"age\": 30},");
    CU_ASSERT_PTR_NULL(guac_dbshell_splitter_next_statement(splitter));

    guac_dbshell_splitter_feed(splitter, " \"batchSize\": [1, 2]}");

    char* statement = guac_dbshell_splitter_next_statement(splitter);
    CU_ASSERT_PTR_NOT_NULL_FATAL(statement);
    CU_ASSERT_PTR_NOT_NULL(strstr(statement, "\"filter\""));
    guac_mem_free(statement);

    CU_ASSERT_FALSE(guac_dbshell_splitter_pending(splitter));

    guac_dbshell_splitter_free(splitter);

}

/**
 * Verifies that braces within JSON string literals do not affect
 * balancing, including escaped quotes.
 */
void test_splitter_json__strings(void) {

    guac_dbshell_splitter* splitter =
        guac_dbshell_splitter_alloc(GUAC_DBSHELL_DIALECT_JSON);

    guac_dbshell_splitter_feed(splitter,
            "{\"a\": \"}}{{\", \"b\": \"\\\"}\"}");

    char* statement = guac_dbshell_splitter_next_statement(splitter);
    CU_ASSERT_PTR_NOT_NULL_FATAL(statement);
    guac_mem_free(statement);

    CU_ASSERT_FALSE(guac_dbshell_splitter_pending(splitter));

    guac_dbshell_splitter_free(splitter);

}

/**
 * Verifies that non-JSON scalar input completes at the end of its line
 * rather than accumulating forever.
 */
void test_splitter_json__scalar(void) {

    guac_dbshell_splitter* splitter =
        guac_dbshell_splitter_alloc(GUAC_DBSHELL_DIALECT_JSON);

    guac_dbshell_splitter_feed(splitter, "hello");

    char* statement = guac_dbshell_splitter_next_statement(splitter);
    CU_ASSERT_PTR_NOT_NULL_FATAL(statement);
    CU_ASSERT_STRING_EQUAL(statement, "hello");
    guac_mem_free(statement);

    guac_dbshell_splitter_free(splitter);

}
