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
#include <pthread.h>
#include <unistd.h>

static guac_thread_local_key_t test_key;
static int destructor_call_count = 0;
static pthread_mutex_t destructor_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * Destructor function for testing cleanup.
 */
static void test_destructor(void* value) {
    pthread_mutex_lock(&destructor_mutex);
    destructor_call_count++;
    pthread_mutex_unlock(&destructor_mutex);
    free(value);
}

/**
 * Thread worker function for multithreaded tests.
 */
static void* thread_worker(void* arg) {
    int thread_id = *(int*)arg;
    
    /* Each thread sets its own value */
    int* test_value = malloc(sizeof(int));
    *test_value = thread_id * 100;
    
    CU_ASSERT_EQUAL(guac_thread_local_setspecific(test_key, test_value), 0);
    
    /* Small delay to increase chance of race conditions */
    usleep(1000);
    
    /* Verify the value is still correct */
    int* retrieved = (int*)guac_thread_local_getspecific(test_key);
    CU_ASSERT_PTR_NOT_NULL(retrieved);
    if (retrieved) {
        CU_ASSERT_EQUAL(*retrieved, thread_id * 100);
    }
    
    return NULL;
}

/**
 * Test which verifies thread isolation of values.
 */
void test_thread_local_storage__multithreaded_isolation() {
    const int NUM_THREADS = 5;
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    
    CU_ASSERT_EQUAL(guac_thread_local_key_create(&test_key, free), 0);
    
    /* Create threads */
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i + 1;
        CU_ASSERT_EQUAL(pthread_create(&threads[i], NULL, thread_worker, &thread_ids[i]), 0);
    }
    
    /* Wait for all threads */
    for (int i = 0; i < NUM_THREADS; i++) {
        CU_ASSERT_EQUAL(pthread_join(threads[i], NULL), 0);
    }
    
    guac_thread_local_key_delete(test_key);
}

/**
 * Thread worker for destructor testing.
 */
static void* destructor_test_worker(void* arg) {
    int* test_value = malloc(sizeof(int));
    *test_value = 999;
    
    guac_thread_local_setspecific(test_key, test_value);
    return NULL; /* Thread exits, destructor should be called */
}

/**
 * Test which verifies that destructors are called on thread exit.
 */
void test_thread_local_storage__destructor_cleanup() {
    
    CU_ASSERT_EQUAL(guac_thread_local_key_create(&test_key, test_destructor), 0);
    
    int initial_count;
    pthread_mutex_lock(&destructor_mutex);
    initial_count = destructor_call_count;
    pthread_mutex_unlock(&destructor_mutex);
    
    pthread_t thread;
    CU_ASSERT_EQUAL(pthread_create(&thread, NULL, destructor_test_worker, NULL), 0);
    CU_ASSERT_EQUAL(pthread_join(thread, NULL), 0);
    
    /* Give some time for cleanup */
    usleep(10000);
    
    int final_count;
    pthread_mutex_lock(&destructor_mutex);
    final_count = destructor_call_count;
    pthread_mutex_unlock(&destructor_mutex);
    
    /* Destructor should have been called */
    CU_ASSERT_TRUE(final_count > initial_count);
    
    guac_thread_local_key_delete(test_key);
}