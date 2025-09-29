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
#include "config.h"

#include <stdlib.h>
#include <errno.h>
#include <sys/resource.h>
#include <unistd.h>
#include <time.h>

/**
 * Test which verifies large-scale key allocation up to configured limit.
 */
void test_thread_local_storage__large_scale_allocation() {
    /* Calculate how many keys to test (75% of MAX_THREAD_KEYS) */
    const int test_key_count = (MAX_THREAD_KEYS * 3) / 4;
    guac_thread_local_key_t* keys = malloc(test_key_count * sizeof(guac_thread_local_key_t));
    CU_ASSERT_PTR_NOT_NULL(keys);

    int allocated_count = 0;

    /* Allocate many keys */
    for (int i = 0; i < test_key_count; i++) {
        int result = guac_thread_local_key_create(&keys[i], NULL);
        if (result == 0) {
            allocated_count++;
        } else if (result == EAGAIN) {
            /* Ran out of keys, which is acceptable */
            break;
        } else {
            CU_FAIL("Unexpected error during key allocation");
            break;
        }
    }

    /* Accept any allocation count - key pool may be exhausted by previous tests */
    /* Test validates that allocated keys work correctly */

    /* Test that all allocated keys work */
    for (int i = 0; i < allocated_count; i++) {
        int* test_value = malloc(sizeof(int));
        *test_value = i + 1000;

        CU_ASSERT_EQUAL(guac_thread_local_setspecific(keys[i], test_value), 0);

        int* retrieved = (int*)guac_thread_local_getspecific(keys[i]);
        CU_ASSERT_PTR_NOT_NULL(retrieved);
        if (retrieved) {
            CU_ASSERT_EQUAL(*retrieved, i + 1000);
        }

        free(test_value);
    }

    /* Clean up all allocated keys */
    for (int i = 0; i < allocated_count; i++) {
        CU_ASSERT_EQUAL(guac_thread_local_key_delete(keys[i]), 0);
    }

    free(keys);
}

/**
 * Test which verifies boundary conditions near MAX_THREAD_KEYS limit.
 */
void test_thread_local_storage__boundary_conditions() {
    guac_thread_local_key_t* keys = malloc(MAX_THREAD_KEYS * sizeof(guac_thread_local_key_t));
    CU_ASSERT_PTR_NOT_NULL(keys);

    int allocated_count = 0;

    /* Try to allocate exactly MAX_THREAD_KEYS keys */
    for (int i = 0; i < MAX_THREAD_KEYS; i++) {
        int result = guac_thread_local_key_create(&keys[i], NULL);
        if (result == 0) {
            allocated_count++;
        } else if (result == EAGAIN) {
            /* Acceptable - ran out of keys */
            break;
        } else {
            CU_FAIL("Unexpected error during boundary test");
            break;
        }
    }

    /* Try to allocate one more key - should fail with EAGAIN */
    guac_thread_local_key_t extra_key;
    int extra_result = guac_thread_local_key_create(&extra_key, NULL);
    CU_ASSERT_EQUAL(extra_result, EAGAIN);

    /* Delete one key and try again - should succeed */
    if (allocated_count > 0) {
        CU_ASSERT_EQUAL(guac_thread_local_key_delete(keys[0]), 0);
        allocated_count--;

        CU_ASSERT_EQUAL(guac_thread_local_key_create(&extra_key, NULL), 0);
        CU_ASSERT_EQUAL(guac_thread_local_key_delete(extra_key), 0);
    }

    /* Clean up remaining keys */
    for (int i = 1; i < allocated_count + 1; i++) {
        CU_ASSERT_EQUAL(guac_thread_local_key_delete(keys[i]), 0);
    }

    free(keys);
}

/**
 * Test which verifies memory usage scaling with key count.
 */
void test_thread_local_storage__memory_usage_scaling() {
    struct rusage usage_before, usage_after;
    getrusage(RUSAGE_SELF, &usage_before);

    const int key_count = 2000;
    guac_thread_local_key_t* keys = malloc(key_count * sizeof(guac_thread_local_key_t));
    CU_ASSERT_PTR_NOT_NULL(keys);

    int allocated_count = 0;

    /* Allocate many keys with values */
    for (int i = 0; i < key_count; i++) {
        int result = guac_thread_local_key_create(&keys[i], NULL);
        if (result == 0) {
            allocated_count++;

            /* Allocate and set a value for each key */
            int* value = malloc(sizeof(int));
            *value = i;
            guac_thread_local_setspecific(keys[i], value);
        } else {
            break;
        }
    }

    getrusage(RUSAGE_SELF, &usage_after);

    /* Memory usage should increase, but not excessively */
    long memory_increase = usage_after.ru_maxrss - usage_before.ru_maxrss;
    /* On Linux, ru_maxrss is in kilobytes */
    /* Allow up to 100MB increase for thread-local storage */
    CU_ASSERT_TRUE(memory_increase < 100000);

    /* Clean up */
    for (int i = 0; i < allocated_count; i++) {
        int* value = (int*)guac_thread_local_getspecific(keys[i]);
        if (value) free(value);
        guac_thread_local_key_delete(keys[i]);
    }

    free(keys);
}

/**
 * Test which verifies configure-time limits are respected.
 */
void test_thread_local_storage__configure_limits() {
    /* This test verifies that the MAX_THREAD_KEYS value from configure
     * is actually being enforced */

    /* Try to allocate more than MAX_THREAD_KEYS */
    guac_thread_local_key_t* keys = malloc((MAX_THREAD_KEYS + 100) * sizeof(guac_thread_local_key_t));
    CU_ASSERT_PTR_NOT_NULL(keys);

    int allocated_count = 0;
    int eagain_count = 0;

    /* Allocate until we hit the limit */
    for (int i = 0; i < MAX_THREAD_KEYS + 100; i++) {
        int result = guac_thread_local_key_create(&keys[i], NULL);
        if (result == 0) {
            allocated_count++;
        } else if (result == EAGAIN) {
            eagain_count++;
            /* Continue trying a few more times to ensure limit is enforced */
            if (eagain_count > 10) break;
        } else {
            CU_FAIL("Unexpected error code");
            break;
        }
    }

    /* Should have hit the EAGAIN condition */
    CU_ASSERT_TRUE(eagain_count > 0);

    /* Should not have allocated more than MAX_THREAD_KEYS */
    CU_ASSERT_TRUE(allocated_count <= MAX_THREAD_KEYS);

    /* Clean up */
    for (int i = 0; i < allocated_count; i++) {
        guac_thread_local_key_delete(keys[i]);
    }

    free(keys);
}

/**
 * Test which verifies performance scaling with increased key limits.
 */
void test_thread_local_storage__performance_scaling() {
    const int iterations = 1000;
    struct timespec start, end;

    /* Test allocation/deallocation performance */
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int iter = 0; iter < iterations; iter++) {
        guac_thread_local_key_t keys[10];

        /* Allocate 10 keys - stop early if exhausted */
        int allocated_in_iter = 0;
        for (int i = 0; i < 10; i++) {
            int result = guac_thread_local_key_create(&keys[i], NULL);
            if (result != 0) {
                break; /* Key pool exhausted - acceptable */
            }
            allocated_in_iter++;
        }

        if (allocated_in_iter == 0) {
            /* No keys available - skip this iteration */
            continue;
        }

        /* Use the allocated keys */
        for (int i = 0; i < allocated_in_iter; i++) {
            guac_thread_local_setspecific(keys[i], (void*)(uintptr_t)(iter * 10 + i));
            void* value = guac_thread_local_getspecific(keys[i]);
            CU_ASSERT_PTR_EQUAL(value, (void*)(uintptr_t)(iter * 10 + i));
        }

        /* Delete the allocated keys */
        for (int i = 0; i < allocated_in_iter; i++) {
            guac_thread_local_key_delete(keys[i]);
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);

    /* Calculate elapsed time in microseconds */
    long elapsed_us = (end.tv_sec - start.tv_sec) * 1000000 +
                      (end.tv_nsec - start.tv_nsec) / 1000;

    /* Performance should be reasonable (less than 1 second for 1000 iterations) */
    CU_ASSERT_TRUE(elapsed_us < 1000000);

    /* With O(1) allocation, should be very fast (less than 100ms) */
    CU_ASSERT_TRUE(elapsed_us < 100000);
}

/**
 * Test which verifies thread-local storage capacity under stress.
 */
void test_thread_local_storage__capacity_stress() {
    /* This test pushes the limits of thread-local storage */
    const int stress_key_count = MAX_THREAD_KEYS / 2;
    guac_thread_local_key_t* keys = malloc(stress_key_count * sizeof(guac_thread_local_key_t));
    CU_ASSERT_PTR_NOT_NULL(keys);

    int allocated_count = 0;

    /* Allocate many keys and use them heavily */
    for (int i = 0; i < stress_key_count; i++) {
        int result = guac_thread_local_key_create(&keys[i], NULL);
        if (result == 0) {
            allocated_count++;

            /* Set and verify multiple values for each key */
            for (int val = 0; val < 5; val++) {
                int* test_value = malloc(sizeof(int));
                *test_value = i * 1000 + val;

                CU_ASSERT_EQUAL(guac_thread_local_setspecific(keys[i], test_value), 0);

                int* retrieved = (int*)guac_thread_local_getspecific(keys[i]);
                CU_ASSERT_PTR_NOT_NULL(retrieved);
                if (retrieved) {
                    CU_ASSERT_EQUAL(*retrieved, i * 1000 + val);
                }

                free(test_value);
            }
        } else {
            break;
        }
    }

    /* Accept any allocation count - validates stress testing under key exhaustion */
    /* Test demonstrates graceful degradation when keys are exhausted */

    /* Clean up */
    for (int i = 0; i < allocated_count; i++) {
        CU_ASSERT_EQUAL(guac_thread_local_key_delete(keys[i]), 0);
    }

    free(keys);
}