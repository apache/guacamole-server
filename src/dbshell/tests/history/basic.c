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

#include "dbshell/history.h"

#include <CUnit/CUnit.h>
#include <stdlib.h>

/**
 * Verifies that entries added to the history can be retrieved in
 * most-recent-first order.
 */
void test_history__add_get(void) {

    guac_dbshell_history* history = guac_dbshell_history_alloc(4);

    CU_ASSERT_PTR_NULL(guac_dbshell_history_get(history, 1));

    guac_dbshell_history_add(history, "first");
    guac_dbshell_history_add(history, "second");
    guac_dbshell_history_add(history, "third");

    CU_ASSERT_STRING_EQUAL(guac_dbshell_history_get(history, 1), "third");
    CU_ASSERT_STRING_EQUAL(guac_dbshell_history_get(history, 2), "second");
    CU_ASSERT_STRING_EQUAL(guac_dbshell_history_get(history, 3), "first");
    CU_ASSERT_PTR_NULL(guac_dbshell_history_get(history, 4));

    guac_dbshell_history_free(history);

}

/**
 * Verifies that empty lines and consecutive duplicates are not stored.
 */
void test_history__dedup(void) {

    guac_dbshell_history* history = guac_dbshell_history_alloc(4);

    guac_dbshell_history_add(history, "");
    CU_ASSERT_PTR_NULL(guac_dbshell_history_get(history, 1));

    guac_dbshell_history_add(history, "same");
    guac_dbshell_history_add(history, "same");
    guac_dbshell_history_add(history, "same");

    CU_ASSERT_STRING_EQUAL(guac_dbshell_history_get(history, 1), "same");
    CU_ASSERT_PTR_NULL(guac_dbshell_history_get(history, 2));

    /* Non-consecutive duplicates are stored */
    guac_dbshell_history_add(history, "other");
    guac_dbshell_history_add(history, "same");

    CU_ASSERT_STRING_EQUAL(guac_dbshell_history_get(history, 1), "same");
    CU_ASSERT_STRING_EQUAL(guac_dbshell_history_get(history, 2), "other");
    CU_ASSERT_STRING_EQUAL(guac_dbshell_history_get(history, 3), "same");

    guac_dbshell_history_free(history);

}

/**
 * Verifies that the oldest entries are discarded once the ring is full.
 */
void test_history__wraparound(void) {

    guac_dbshell_history* history = guac_dbshell_history_alloc(3);

    guac_dbshell_history_add(history, "one");
    guac_dbshell_history_add(history, "two");
    guac_dbshell_history_add(history, "three");
    guac_dbshell_history_add(history, "four");
    guac_dbshell_history_add(history, "five");

    CU_ASSERT_STRING_EQUAL(guac_dbshell_history_get(history, 1), "five");
    CU_ASSERT_STRING_EQUAL(guac_dbshell_history_get(history, 2), "four");
    CU_ASSERT_STRING_EQUAL(guac_dbshell_history_get(history, 3), "three");
    CU_ASSERT_PTR_NULL(guac_dbshell_history_get(history, 4));

    guac_dbshell_history_free(history);

}

/**
 * Verifies that out-of-range offsets are rejected.
 */
void test_history__bounds(void) {

    guac_dbshell_history* history = guac_dbshell_history_alloc(2);

    guac_dbshell_history_add(history, "entry");

    CU_ASSERT_PTR_NULL(guac_dbshell_history_get(history, 0));
    CU_ASSERT_PTR_NULL(guac_dbshell_history_get(history, -1));
    CU_ASSERT_PTR_NULL(guac_dbshell_history_get(history, 2));

    guac_dbshell_history_free(history);

}
