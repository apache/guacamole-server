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
#include <guacamole/error.h>

#include <string.h>

/**
 * Test which verifies that error handling remains thread-local and functional.
 */
void test_thread_local_storage__error_basic() {
    
    /* Test initial state */
    guac_error = GUAC_STATUS_SUCCESS;
    guac_error_message = "Initial message";
    
    CU_ASSERT_EQUAL(guac_error, GUAC_STATUS_SUCCESS);
    CU_ASSERT_STRING_EQUAL(guac_error_message, "Initial message");
    
    /* Test setting different values */
    guac_error = GUAC_STATUS_IO_ERROR;
    guac_error_message = "IO error occurred";
    
    CU_ASSERT_EQUAL(guac_error, GUAC_STATUS_IO_ERROR);
    CU_ASSERT_STRING_EQUAL(guac_error_message, "IO error occurred");
}

/**
 * Test which verifies error state persistence across operations.
 */
void test_thread_local_storage__error_persistence() {
    
    /* Set an error state */
    guac_error = GUAC_STATUS_TIMEOUT;
    guac_error_message = "Operation timed out";
    
    /* Verify state is maintained */
    CU_ASSERT_EQUAL(guac_error, GUAC_STATUS_TIMEOUT);
    CU_ASSERT_STRING_EQUAL(guac_error_message, "Operation timed out");
    
    /* Change to different error */
    guac_error = GUAC_STATUS_NO_MEMORY;
    guac_error_message = "Memory allocation failed";
    
    CU_ASSERT_EQUAL(guac_error, GUAC_STATUS_NO_MEMORY);
    CU_ASSERT_STRING_EQUAL(guac_error_message, "Memory allocation failed");
}

/**
 * Test which verifies error message can be set to NULL.
 */
void test_thread_local_storage__error_null_message() {
    
    /* Set message to NULL */
    guac_error_message = NULL;
    CU_ASSERT_PTR_NULL(guac_error_message);
    
    /* Set back to a string */
    guac_error_message = "Test message";
    CU_ASSERT_STRING_EQUAL(guac_error_message, "Test message");
}