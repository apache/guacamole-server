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

#include <CUnit/CUnit.h>
#include <guacamole/alloc.h>
#include <stdint.h>

/**
 * Test which verifies that guac_alloc_size() returns zero for all inputs
 * involving at least one zero value.
 */
void test_alloc__size_zero() {

    CU_ASSERT_EQUAL(guac_alloc_size(0), 0);
    CU_ASSERT_EQUAL(guac_alloc_size(0, 0), 0);
    CU_ASSERT_EQUAL(guac_alloc_size(0, 0, 0), 0);
    CU_ASSERT_EQUAL(guac_alloc_size(0, 0, 0, 0), 0);
    CU_ASSERT_EQUAL(guac_alloc_size(0, 0, 0, 0, 0), 0);

    CU_ASSERT_EQUAL(guac_alloc_size(1, 0), 0);
    CU_ASSERT_EQUAL(guac_alloc_size(3, 2, 0), 0);
    CU_ASSERT_EQUAL(guac_alloc_size(5, 0, 8, 9), 0);
    CU_ASSERT_EQUAL(guac_alloc_size(99, 99, 99, 0, 99), 0);

}

/**
 * Test which verifies that guac_alloc_size() returns expected values for
 * relatively small integer inputs.
 */
void test_alloc__size_small() {

    CU_ASSERT_EQUAL(guac_alloc_size(123), 123);
    CU_ASSERT_EQUAL(guac_alloc_size(123, 456), 123 * 456);
    CU_ASSERT_EQUAL(guac_alloc_size(123, 456, 789), 123 * 456 * 789);

}

/**
 * Test which verifies that guac_alloc_size() returns expected values for
 * relatively large integer inputs, including inputs that cause overflow
 * beyond the capacity of a size_t.
 */
void test_alloc__size_large() {

    CU_ASSERT_EQUAL(guac_alloc_size(SIZE_MAX), SIZE_MAX);

    CU_ASSERT_EQUAL(guac_alloc_size(123, 456, SIZE_MAX), 0);
    CU_ASSERT_EQUAL(guac_alloc_size(SIZE_MAX / 2, SIZE_MAX / 2), 0);

}

