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

#include "display-priv.h"

#include <CUnit/CUnit.h>
#include <guacamole/client.h>
#include <guacamole/display.h>

/**
 * Allocates a guac_display for a newly-allocated guac_client configured with
 * the given limit on the number of display worker threads, returning the
 * number of worker threads that were created for that display. The
 * guac_display and guac_client are both freed before this function returns.
 *
 * @param max_display_worker_threads
 *     The value to assign to the max_display_worker_threads member of the
 *     guac_client prior to allocating the guac_display.
 *
 * @return
 *     The number of worker threads created for the allocated guac_display.
 */
static int get_worker_thread_count(int max_display_worker_threads) {

    guac_client* client = guac_client_alloc();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    client->max_display_worker_threads = max_display_worker_threads;

    guac_display* display = guac_display_alloc(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(display);

    int count = display->worker_thread_count;

    guac_display_free(display);
    guac_client_free(client);

    return count;

}

/**
 * Test which verifies that the max_display_worker_threads member of
 * guac_client limits the number of worker threads created for that client's
 * guac_display, and that no limit is imposed by default.
 */
void test_display__max_worker_threads(void) {

    /* At least one worker thread should be created by default (a
     * newly-allocated guac_client imposes no limit) */
    int default_count = get_worker_thread_count(0);
    CU_ASSERT(default_count >= 1);

    /* A configured limit should cap the number of worker threads */
    CU_ASSERT_EQUAL(get_worker_thread_count(1), 1);

    /* A limit that exceeds the default thread count should have no effect
     * (the limit can only reduce the number of threads created, not
     * increase it) */
    CU_ASSERT_EQUAL(get_worker_thread_count(default_count + 100), default_count);

    /* Negative values should be treated as if no limit were set */
    CU_ASSERT_EQUAL(get_worker_thread_count(-1), default_count);

    /* The limit is per-client: a client configured with a limit must not
     * affect the displays of other clients */
    CU_ASSERT_EQUAL(get_worker_thread_count(1), 1);
    CU_ASSERT_EQUAL(get_worker_thread_count(0), default_count);

}
