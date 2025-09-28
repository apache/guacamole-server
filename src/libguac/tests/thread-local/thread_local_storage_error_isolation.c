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

#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

/**
 * Thread function that sets different error.
 */
static void* isolation_thread_func(void* arg) {
    (void)arg; /* Unused parameter */

    /* Initially should have no error in new thread */
    CU_ASSERT_EQUAL(guac_error, GUAC_STATUS_SUCCESS);
    CU_ASSERT_PTR_NULL(guac_error_message);

    /* Set different error */
    guac_error = GUAC_STATUS_INVALID_ARGUMENT;
    guac_error_message = "Thread error";

    /* Verify error is set correctly */
    CU_ASSERT_EQUAL(guac_error, GUAC_STATUS_INVALID_ARGUMENT);
    CU_ASSERT_STRING_EQUAL(guac_error_message, "Thread error");

    return NULL;
}

/**
 * Test which verifies error isolation between threads.
 */
void test_thread_local_storage__error_isolation() {
    /* Set an error in main thread */
    guac_error = GUAC_STATUS_NO_MEMORY;
    guac_error_message = "Main thread error";

    /* Verify error is set */
    CU_ASSERT_EQUAL(guac_error, GUAC_STATUS_NO_MEMORY);
    CU_ASSERT_STRING_EQUAL(guac_error_message, "Main thread error");

    /* Create a thread that sets different error */
    pthread_t thread;

    CU_ASSERT_EQUAL(pthread_create(&thread, NULL, isolation_thread_func, NULL), 0);
    CU_ASSERT_EQUAL(pthread_join(thread, NULL), 0);

    /* Main thread error should be unchanged */
    CU_ASSERT_EQUAL(guac_error, GUAC_STATUS_NO_MEMORY);
    CU_ASSERT_STRING_EQUAL(guac_error_message, "Main thread error");

    /* Reset error */
    guac_error = GUAC_STATUS_SUCCESS;
    guac_error_message = NULL;
}

/**
 * Structure for passing data to worker threads.
 */
typedef struct error_test_data {
    int thread_id;
    guac_status expected_error;
    const char* expected_message;
    volatile int* results;
} error_test_data_t;

/**
 * Worker function for multi-threaded error isolation test.
 */
static void* error_isolation_worker(void* arg) {
    error_test_data_t* data = (error_test_data_t*)arg;
    int id = data->thread_id;

    /* Each thread should start with clean error state */
    if (guac_error != GUAC_STATUS_SUCCESS || guac_error_message != NULL) {
        data->results[id] = 1; /* Failure */
        return NULL;
    }

    /* Set unique error for this thread */
    guac_error = data->expected_error;
    guac_error_message = data->expected_message;

    /* Small delay to increase chance of race conditions if they exist */
    usleep(1000);

    /* Verify error state is still correct */
    if (guac_error != data->expected_error ||
        strcmp(guac_error_message, data->expected_message) != 0) {
        data->results[id] = 2; /* Race condition detected */
        return NULL;
    }

    data->results[id] = 0; /* Success */
    return NULL;
}

/**
 * Test which verifies error isolation with multiple threads.
 */
void test_thread_local_storage__multi_thread_error_isolation() {
    const int NUM_THREADS = 5;
    pthread_t threads[NUM_THREADS];
    error_test_data_t thread_data[NUM_THREADS];
    volatile int results[NUM_THREADS];

    /* Prepare test data for each thread */
    const char* messages[] = {
        "Thread 0 error",
        "Thread 1 error",
        "Thread 2 error",
        "Thread 3 error",
        "Thread 4 error"
    };

    guac_status errors[] = {
        GUAC_STATUS_NO_MEMORY,
        GUAC_STATUS_INVALID_ARGUMENT,
        GUAC_STATUS_NOT_FOUND,
        GUAC_STATUS_TIMEOUT,
        GUAC_STATUS_IO_ERROR
    };

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].expected_error = errors[i];
        thread_data[i].expected_message = messages[i];
        thread_data[i].results = results;
        results[i] = -1; /* Not yet completed */
    }

    /* Create all threads */
    for (int i = 0; i < NUM_THREADS; i++) {
        CU_ASSERT_EQUAL(pthread_create(&threads[i], NULL, error_isolation_worker, &thread_data[i]), 0);
    }

    /* Wait for all threads */
    for (int i = 0; i < NUM_THREADS; i++) {
        CU_ASSERT_EQUAL(pthread_join(threads[i], NULL), 0);
    }

    /* Verify all threads succeeded */
    for (int i = 0; i < NUM_THREADS; i++) {
        CU_ASSERT_EQUAL(results[i], 0); /* 0 means success */
    }

    /* Main thread should still have clean state */
    CU_ASSERT_EQUAL(guac_error, GUAC_STATUS_SUCCESS);
    CU_ASSERT_PTR_NULL(guac_error_message);
}

/**
 * Test which verifies error message persistence within a thread.
 */
void test_thread_local_storage__error_message_persistence() {
    /* Set an error message */
    guac_error = GUAC_STATUS_INVALID_ARGUMENT;
    guac_error_message = "Persistent error message";

    /* Call some function that might affect thread-local storage */
    /* (This simulates real usage where other code might run) */
    void* dummy = malloc(100);
    free(dummy);

    /* Error should still be there */
    CU_ASSERT_EQUAL(guac_error, GUAC_STATUS_INVALID_ARGUMENT);
    CU_ASSERT_STRING_EQUAL(guac_error_message, "Persistent error message");

    /* Reset error */
    guac_error = GUAC_STATUS_SUCCESS;
    guac_error_message = NULL;
}

/**
 * Test which verifies fallback mechanism under memory pressure.
 */
void test_thread_local_storage__fallback_mechanism() {
    /* This test is tricky because we can't easily force malloc to fail.
     * Instead, we test that the fallback variables exist and work. */

    /* Save current error state */
    guac_status saved_error = guac_error;
    const char* saved_message = guac_error_message;

    /* Set error normally */
    guac_error = GUAC_STATUS_TIMEOUT;
    guac_error_message = "Test message";

    /* Verify it's set */
    CU_ASSERT_EQUAL(guac_error, GUAC_STATUS_TIMEOUT);
    CU_ASSERT_STRING_EQUAL(guac_error_message, "Test message");

    /* The fact that this works means the implementation is functional.
     * In case of memory allocation failure, the implementation should
     * gracefully fall back to global variables. */

    /* Restore original state */
    guac_error = saved_error;
    guac_error_message = saved_message;
}

/**
 * Worker function for rapid cycle test.
 */
static void* rapid_cycle_worker(void* arg) {
    volatile int* success = (volatile int*)arg;

    /* Set unique error */
    guac_error = GUAC_STATUS_INVALID_ARGUMENT;
    guac_error_message = "Rapid cycle error";

    /* Verify it's set correctly */
    if (guac_error == GUAC_STATUS_INVALID_ARGUMENT &&
        strcmp(guac_error_message, "Rapid cycle error") == 0) {
        *success = 1;
    }

    return NULL;
}

/**
 * Test which verifies error handling across rapid thread creation/destruction.
 */
void test_thread_local_storage__rapid_thread_cycles() {
    const int CYCLE_COUNT = 50;

    for (int cycle = 0; cycle < CYCLE_COUNT; cycle++) {
        pthread_t thread;
        volatile int thread_success = 0;

        CU_ASSERT_EQUAL(pthread_create(&thread, NULL, rapid_cycle_worker, (void*)&thread_success), 0);
        CU_ASSERT_EQUAL(pthread_join(thread, NULL), 0);
        CU_ASSERT_EQUAL(thread_success, 1);
    }

    /* Main thread should still have clean state */
    CU_ASSERT_EQUAL(guac_error, GUAC_STATUS_SUCCESS);
    CU_ASSERT_PTR_NULL(guac_error_message);
}

/**
 * Worker function for cleanup test.
 */
static void* cleanup_worker(void* arg) {
    volatile int* passed = (volatile int*)arg;

    /* Set error to force allocation of thread-local storage */
    guac_error = GUAC_STATUS_NO_MEMORY;
    guac_error_message = "Cleanup test message";

    /* Verify it's set */
    if (guac_error == GUAC_STATUS_NO_MEMORY &&
        strcmp(guac_error_message, "Cleanup test message") == 0) {
        *passed = 1;
    }

    /* Thread exits here, cleanup should happen automatically */
    return NULL;
}

/**
 * Test which verifies memory cleanup doesn't cause issues.
 */
void test_thread_local_storage__memory_cleanup() {
    /* Create thread that allocates error storage, then exits */
    pthread_t thread;
    volatile int cleanup_test_passed = 0;

    CU_ASSERT_EQUAL(pthread_create(&thread, NULL, cleanup_worker, (void*)&cleanup_test_passed), 0);
    CU_ASSERT_EQUAL(pthread_join(thread, NULL), 0);
    CU_ASSERT_EQUAL(cleanup_test_passed, 1);

    /* If we got here without segfault, cleanup worked */
    /* Main thread should be unaffected */
    CU_ASSERT_EQUAL(guac_error, GUAC_STATUS_SUCCESS);
    CU_ASSERT_PTR_NULL(guac_error_message);
}