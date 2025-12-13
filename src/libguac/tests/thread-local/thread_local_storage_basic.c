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
#include <guacamole/thread-local.h>
#include <guacamole/error.h>

#include <stdlib.h>
#include <pthread.h>
#include <errno.h>

/* Static helper functions for once tests */
static int once_test_init_count = 0;

static void increment_counter_for_once_test() {
    once_test_init_count++;
}

static void dummy_function_for_once_test() {
    /* Empty function for testing */
}

/**
 * Test which verifies basic thread-local key creation and deletion.
 */
void test_thread_local_storage__key_create_delete() {
    guac_thread_local_key_t key;
    
    /* Key creation should succeed */
    CU_ASSERT_EQUAL(guac_thread_local_key_create(&key, NULL), 0);
    
    /* Key deletion should succeed */
    CU_ASSERT_EQUAL(guac_thread_local_key_delete(key), 0);
}

/**
 * Test which verifies basic setspecific/getspecific functionality.
 */
void test_thread_local_storage__setspecific_getspecific() {
    guac_thread_local_key_t key;
    int test_value = 42;
    
    CU_ASSERT_EQUAL(guac_thread_local_key_create(&key, NULL), 0);
    
    /* Set and retrieve a value */
    CU_ASSERT_EQUAL(guac_thread_local_setspecific(key, &test_value), 0);
    CU_ASSERT_PTR_EQUAL(guac_thread_local_getspecific(key), &test_value);
    
    /* Set and retrieve NULL */
    CU_ASSERT_EQUAL(guac_thread_local_setspecific(key, NULL), 0);
    CU_ASSERT_PTR_NULL(guac_thread_local_getspecific(key));
    
    guac_thread_local_key_delete(key);
}

/**
 * Test which verifies error handling for invalid arguments.
 */
void test_thread_local_storage__invalid_arguments() {
    
    /* NULL key pointer should return EINVAL */
    CU_ASSERT_EQUAL(guac_thread_local_key_create(NULL, NULL), EINVAL);
    
    /* Invalid key should return appropriate error */
    CU_ASSERT_NOT_EQUAL(guac_thread_local_setspecific(0xFFFFFFFF, NULL), 0);
    CU_ASSERT_PTR_NULL(guac_thread_local_getspecific(0xFFFFFFFF));
}

/**
 * Test which verifies behavior after key deletion.
 */
void test_thread_local_storage__deleted_key() {
    guac_thread_local_key_t key;
    
    CU_ASSERT_EQUAL(guac_thread_local_key_create(&key, NULL), 0);
    CU_ASSERT_EQUAL(guac_thread_local_setspecific(key, (void*)0x123), 0);
    
    /* Delete the key */
    guac_thread_local_key_delete(key);
    
    /* Operations on deleted key should fail */
    CU_ASSERT_NOT_EQUAL(guac_thread_local_setspecific(key, (void*)0x456), 0);
    CU_ASSERT_PTR_NULL(guac_thread_local_getspecific(key));
}

/**
 * Test which verifies thread-local once functionality.
 */
void test_thread_local_storage__once_init() {
    static guac_thread_local_once_t once_control = GUAC_THREAD_LOCAL_ONCE_INIT;
    
    /* Reset counter for this test */
    once_test_init_count = 0;
    
    /* Call multiple times - should only increment once */
    CU_ASSERT_EQUAL(guac_thread_local_once(&once_control, increment_counter_for_once_test), 0);
    CU_ASSERT_EQUAL(guac_thread_local_once(&once_control, increment_counter_for_once_test), 0);
    CU_ASSERT_EQUAL(guac_thread_local_once(&once_control, increment_counter_for_once_test), 0);
    
    CU_ASSERT_EQUAL(once_test_init_count, 1);
}

/**
 * Test which verifies that NULL arguments to once function return EINVAL.
 */
void test_thread_local_storage__once_invalid_args() {
    guac_thread_local_once_t once_control = GUAC_THREAD_LOCAL_ONCE_INIT;
    
    CU_ASSERT_EQUAL(guac_thread_local_once(NULL, dummy_function_for_once_test), EINVAL);
    CU_ASSERT_EQUAL(guac_thread_local_once(&once_control, NULL), EINVAL);
    CU_ASSERT_EQUAL(guac_thread_local_once(NULL, NULL), EINVAL);
}