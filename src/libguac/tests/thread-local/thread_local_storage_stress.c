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

#include <stdlib.h>

/**
 * Test which attempts to create many keys to test resource limits.
 */
void test_thread_local_storage__key_exhaustion() {
    guac_thread_local_key_t keys[1025]; /* More than MAX_THREAD_KEYS */
    int created = 0;
    
    /* Try to create more keys than allowed */
    for (int i = 0; i < 1025; i++) {
        if (guac_thread_local_key_create(&keys[i], NULL) == 0) {
            created++;
        } else {
            break;
        }
    }
    
    /* Should not be able to create unlimited keys */
    CU_ASSERT_TRUE(created > 0);
    CU_ASSERT_TRUE(created <= 1024); /* Should respect MAX_THREAD_KEYS */
    
    /* Clean up created keys */
    for (int i = 0; i < created; i++) {
        guac_thread_local_key_delete(keys[i]);
    }
}

/**
 * Test which verifies behavior with rapid key creation/deletion cycles.
 */
void test_thread_local_storage__rapid_key_cycling() {
    const int NUM_CYCLES = 100;
    
    for (int i = 0; i < NUM_CYCLES; i++) {
        guac_thread_local_key_t key;
        
        /* Create key */
        CU_ASSERT_EQUAL(guac_thread_local_key_create(&key, NULL), 0);
        
        /* Use key */
        CU_ASSERT_EQUAL(guac_thread_local_setspecific(key, (void*)(intptr_t)i), 0);
        CU_ASSERT_PTR_EQUAL(guac_thread_local_getspecific(key), (void*)(intptr_t)i);
        
        /* Delete key */
        guac_thread_local_key_delete(key);
    }
}

/**
 * Test which verifies multiple keys can be used simultaneously.
 */
void test_thread_local_storage__multiple_keys() {
    const int NUM_KEYS = 10;
    guac_thread_local_key_t keys[NUM_KEYS];
    
    /* Create multiple keys */
    for (int i = 0; i < NUM_KEYS; i++) {
        CU_ASSERT_EQUAL(guac_thread_local_key_create(&keys[i], NULL), 0);
    }
    
    /* Set different values for each key */
    for (int i = 0; i < NUM_KEYS; i++) {
        CU_ASSERT_EQUAL(guac_thread_local_setspecific(keys[i], (void*)(intptr_t)(i * 100)), 0);
    }
    
    /* Verify each key has the correct value */
    for (int i = 0; i < NUM_KEYS; i++) {
        void* value = guac_thread_local_getspecific(keys[i]);
        CU_ASSERT_PTR_EQUAL(value, (void*)(intptr_t)(i * 100));
    }
    
    /* Clean up */
    for (int i = 0; i < NUM_KEYS; i++) {
        guac_thread_local_key_delete(keys[i]);
    }
}

/**
 * Test which verifies double deletion doesn't crash.
 */
void test_thread_local_storage__double_delete() {
    guac_thread_local_key_t key;
    
    CU_ASSERT_EQUAL(guac_thread_local_key_create(&key, NULL), 0);
    
    /* First deletion should succeed */
    CU_ASSERT_EQUAL(guac_thread_local_key_delete(key), 0);
    
    /* Second deletion should not crash (behavior is undefined but shouldn't crash) */
    guac_thread_local_key_delete(key);
}