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
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <CUnit/CUnit.h>
#include <guacamole/error.h>
#include <guacamole/thread.h>
#include <guacamole/timestamp.h>

#include <stdio.h>
#include <string.h>

/**
 * The size of the buffer used to receive the name of a thread. This is
 * intentionally larger than the 16 bytes allowed by guac_thread, for the sake
 * of verifying truncation.
 */
#define NAME_BUFFER_SIZE 64

/**
 * The number of milliseconds to sleep between each iteration of the busy loop
 * within block_until_canceled(). This affects only how promptly cancellation of
 * the thread takes effect.
 */
#define CANCEL_SLEEP_INTERVAL 10

/**
 * Arbitrary non-NULL value for use as a relatively unique sentinel where a
 * void* is needed.
 */
void* SENTINEL = &SENTINEL;

/**
 * Thread routine which returns its data pointer verbatim.
 *
 * @param data
 *     A pointer to arbitrary data.
 *
 * @return
 *     The provided data pointer, unmodified.
 */
static void* return_data(void* data) {
    return data;
}

/**
 * Thread routine which sleeps indefinitely, terminating only if canceled.
 *
 * @param data
 *     A pointer to arbitrary data. This value is ignored.
 *
 * @return
 *     This function never returns.
 */
static void* block_until_canceled(void* data) {

    for (;;) {

        /* Each sleep is a cancellation point */
        guac_timestamp_msleep(CANCEL_SLEEP_INTERVAL);

    }

    CU_ASSERT_FATAL(0);
    return NULL;

}

/**
 * Test which verifies that a guac_thread that has never been started may be
 * safely joined, detached, and cancelled.
 */
void test_thread__not_started() {

    guac_thread thread;
    guac_thread_init(&thread);
    CU_ASSERT_FALSE(thread.started);

    guac_thread_cancel(&thread);
    guac_thread_detach(&thread);

    /* No return value should be stored by guac_thread_join() when the thread
     * hasn't started */
    void* value = SENTINEL;
    CU_ASSERT_TRUE(guac_thread_join(&thread, &value));
    CU_ASSERT_EQUAL(guac_error, GUAC_STATUS_NOT_FOUND);
    CU_ASSERT_PTR_EQUAL(value, SENTINEL);
    CU_ASSERT_FALSE(thread.started);

}

/**
 * Test which verifies that a zeroed guac_thread is equivalent to an initialized
 * guac_thread that has not been started.
 */
void test_thread__zeroed() {

    guac_thread thread = { 0 };
    CU_ASSERT_FALSE(thread.started);
    CU_ASSERT_TRUE(guac_thread_join(&thread, NULL));

}

/**
 * Test which verifies that guac_thread_create() starts a thread that runs the
 * given routine, that joining the started thread returns whatever that routine
 * returns, and that a joined thread is no longer considered started.
 */
void test_thread__create_join() {

    void* value = NULL;
    guac_thread thread;

    CU_ASSERT_FALSE_FATAL(guac_thread_create(&thread, return_data, SENTINEL, "test"));
    CU_ASSERT_TRUE(thread.started);

    CU_ASSERT_FALSE(guac_thread_join(&thread, &value));
    CU_ASSERT_PTR_EQUAL(value, SENTINEL);
    CU_ASSERT_FALSE(thread.started);

    /* Joining again must report that there is nothing to join, leaving any
     * previously stored value alone */
    CU_ASSERT_TRUE(guac_thread_join(&thread, &value));
    CU_ASSERT_PTR_EQUAL(value, SENTINEL);

}

/**
 * Test which verifies that a thread routine returning NULL is distinguishable
 * from a thread that never started.
 */
void test_thread__returned_null() {

    void* value = SENTINEL;
    guac_thread thread;

    CU_ASSERT_FALSE_FATAL(guac_thread_create(&thread, return_data, NULL, "test"));
    CU_ASSERT_FALSE(guac_thread_join(&thread, &value));
    CU_ASSERT_PTR_NULL(value);

    value = SENTINEL;
    CU_ASSERT_TRUE(guac_thread_join(&thread, &value));
    CU_ASSERT_PTR_EQUAL(value, SENTINEL);

}

/**
 * Test which verifies that a canceled thread is joined by guac_thread_join()
 * without touching the return value.
 */
void test_thread__canceled() {

    void* value = SENTINEL;
    guac_thread thread;

    CU_ASSERT_FALSE_FATAL(guac_thread_create(&thread, block_until_canceled, NULL, "test"));
    guac_thread_cancel(&thread);

    CU_ASSERT_TRUE(guac_thread_join(&thread, &value));
    CU_ASSERT_EQUAL(guac_error, GUAC_STATUS_CANCELED);
    CU_ASSERT_PTR_EQUAL(value, SENTINEL);
    CU_ASSERT_FALSE(thread.started);

}

/**
 * Test which verifies that a detached guac_thread is no longer considered
 * started.
 */
void test_thread__detach() {

    guac_thread thread;

    CU_ASSERT_FALSE_FATAL(guac_thread_create(&thread, return_data, NULL, "test"));
    CU_ASSERT_TRUE(thread.started);

    guac_thread_detach(&thread);
    CU_ASSERT_FALSE(thread.started);

}

/* NOTE: For sake of simplicity, this test is Linux-specific */

#ifdef __linux__
/**
 * Thread routine that stores the name of the calling thread within a provided
 * buffer. The buffer must be at least NAME_BUFFER_SIZE bytes. If the name
 * cannot be determined, the buffer is set to the empty string. The buffer will
 * always be null-terminated.
 *
 * @param data
 *     A pointer to a buffer of at least NAME_BUFFER_SIZE bytes.
 *
 * @return
 *     Always NULL.
 */
static void* store_name(void* data) {

    char* name = (char*) data;
    name[0] = '\0';

    FILE* comm = fopen("/proc/thread-self/comm", "r");
    if (comm != NULL) {
        if (fgets(name, NAME_BUFFER_SIZE, comm) != NULL)
            name[strcspn(name, "\n")] = '\0';
        fclose(comm);
    }

    return NULL;

}
#endif

/**
 * Test which verifies that the name of a guac_thread is formatted from the
 * provided format string and arguments.
 */
void test_thread__name() {

#ifdef __linux__
    char name[NAME_BUFFER_SIZE];
    guac_thread thread;

    CU_ASSERT_FALSE_FATAL(guac_thread_create(&thread, store_name, name, "test-%i-%s", 42, "abc"));
    guac_thread_join(&thread, NULL);

    CU_ASSERT_STRING_EQUAL(name, "test-42-abc");
#endif

}

/**
 * Test which verifies that names longer than the platform limit are truncated
 * but do not prevent the thread from starting.
 */
void test_thread__name_truncated() {

#ifdef __linux__
    char name[NAME_BUFFER_SIZE];
    guac_thread thread;

    CU_ASSERT_FALSE_FATAL(guac_thread_create(&thread, store_name, name, "0123456789abcdefXYZ"));
    guac_thread_join(&thread, NULL);

    CU_ASSERT_STRING_EQUAL(name, "0123456789abcde");
#endif

}
