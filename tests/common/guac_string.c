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

#include "config.h"

#include "common_suite.h"
#include "common/string.h"

#include <stdlib.h>
#include <CUnit/Basic.h>

void test_guac_string() {

    char** tokens;

    /* Test occurrence counting */
    CU_ASSERT_EQUAL(4, guac_count_occurrences("this is a test string", 's'));
    CU_ASSERT_EQUAL(3, guac_count_occurrences("this is a test string", 'i'));
    CU_ASSERT_EQUAL(0, guac_count_occurrences("", 's'));

    /* Split test string */
    tokens = guac_split("this is a test string", ' ');

    CU_ASSERT_PTR_NOT_NULL(tokens);

    /* Check resulting tokens */
    CU_ASSERT_PTR_NOT_NULL_FATAL(tokens[0]);
    CU_ASSERT_STRING_EQUAL("this", tokens[0]);

    CU_ASSERT_PTR_NOT_NULL_FATAL(tokens[1]);
    CU_ASSERT_STRING_EQUAL("is", tokens[1]);

    CU_ASSERT_PTR_NOT_NULL_FATAL(tokens[2]);
    CU_ASSERT_STRING_EQUAL("a", tokens[2]);

    CU_ASSERT_PTR_NOT_NULL_FATAL(tokens[3]);
    CU_ASSERT_STRING_EQUAL("test", tokens[3]);

    CU_ASSERT_PTR_NOT_NULL_FATAL(tokens[4]);
    CU_ASSERT_STRING_EQUAL("string", tokens[4]);

    CU_ASSERT_PTR_NULL(tokens[5]);


    /* Clean up */
    free(tokens[0]);
    free(tokens[1]);
    free(tokens[2]);
    free(tokens[3]);
    free(tokens[4]);
    free(tokens);

}

