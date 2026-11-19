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
#include <guacamole/client.h>
#include <guacamole/user.h>

#include <pthread.h>
#include <stddef.h>

/**
 * State used by the owner tests to control the result of the join handler and
 * record the owner seen by its protocol initialization thread.
 */
typedef struct owner_test_state {

    /**
     * The connection owner returned by the initialization thread. This is NULL
     * if no owner was available when the thread looked up the owner.
     */
    guac_user* observed_owner;

    /**
     * The result that the join handler should return if thread creation and
     * joining succeed. Zero accepts the user; any non-zero value rejects it.
     */
    int join_result;

    /**
     * The error code returned by pthread_create() if thread creation fails,
     * or the result of pthread_join() otherwise. Zero indicates success.
     */
    int thread_result;

} owner_test_state;

/**
 * Callback for guac_client_for_owner() which returns the supplied owner
 * without modifying it, allowing tests to inspect the owner through the same
 * lookup mechanism used by protocol initialization.
 *
 * @param owner
 *     The current connection owner, or NULL if no owner is available.
 *
 * @param data
 *     Arbitrary callback data. This value is not used.
 *
 * @return
 *     The supplied owner pointer, which may be NULL.
 */
static void* return_owner(guac_user* owner, void* data) {
    return owner;
}

/**
 * Thread routine which looks up the connection owner. This models the owner
 * lookup performed when protocol initialization exposes a filesystem.
 *
 * @param data
 *     The guac_user whose connection is being initialized. Its client member
 *     must point to the guac_client being tested.
 *
 * @return
 *     The connection owner returned by guac_client_for_owner(), or NULL if
 *     the owner was not yet available.
 */
static void* initialize_protocol(void* data) {
    guac_user* user = (guac_user*) data;
    return guac_client_for_owner(user->client, return_owner, NULL);
}

/**
 * Join handler which starts a protocol initialization thread and waits for it
 * to finish before returning. This forces the owner lookup to occur during
 * the join handler, making the initialization race reproducible without
 * depending on thread scheduling. The thread result and observed owner are
 * recorded in the owner_test_state referenced by user->data.
 *
 * @param user
 *     The user being added to the connection. Its client member must reference
 *     the connection, and its data member must reference an owner_test_state.
 *
 * @param argc
 *     The number of connection arguments. This value is not used.
 *
 * @param argv
 *     The connection arguments. This value is not used.
 *
 * @return
 *     The configured join_result if thread creation and joining succeed, or
 *     one if either thread operation fails.
 */
static int join_with_initialization(guac_user* user, int argc, char** argv) {
    owner_test_state* state = (owner_test_state*) user->data;
    pthread_t thread;
    void* owner = NULL;

    state->thread_result = pthread_create(&thread, NULL,
            initialize_protocol, user);
    if (state->thread_result)
        return 1;

    state->thread_result = pthread_join(thread, &owner);
    state->observed_owner = (guac_user*) owner;
    return state->thread_result ? 1 : state->join_result;
}

/**
 * Test which verifies that a protocol initialization thread can find the
 * connection owner before the owner's join handler returns, and that the
 * owner remains available after the user has been added successfully.
 */
void test_client__owner_available_during_join(void) {
    guac_client* client = guac_client_alloc();
    guac_user* owner = guac_user_alloc();
    owner_test_state state = { 0 };
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(owner);
    client->join_handler = join_with_initialization;
    owner->client = client;
    owner->owner = 1;
    owner->data = &state;

    CU_ASSERT_EQUAL(guac_client_add_user(client, owner, 0, NULL), 0);
    CU_ASSERT_EQUAL(state.thread_result, 0);
    CU_ASSERT_PTR_EQUAL(state.observed_owner, owner);
    CU_ASSERT_PTR_EQUAL(guac_client_for_owner(client, return_owner, NULL), owner);

    guac_client_free(client);
    guac_user_free(owner);
}

/**
 * Test which verifies that the owner is available during a join which is
 * subsequently rejected, but is cleared after the rejection without adding a
 * connected user. Retrying the join successfully must make the owner
 * available both during and after the second join attempt.
 */
void test_client__failed_owner_join(void) {
    guac_client* client = guac_client_alloc();
    guac_user* owner = guac_user_alloc();
    owner_test_state state = { .join_result = 1 };
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(owner);
    client->join_handler = join_with_initialization;
    owner->client = client;
    owner->owner = 1;
    owner->data = &state;

    CU_ASSERT_EQUAL(guac_client_add_user(client, owner, 0, NULL), 1);
    CU_ASSERT_EQUAL(state.thread_result, 0);
    CU_ASSERT_PTR_EQUAL(state.observed_owner, owner);
    CU_ASSERT_PTR_NULL(guac_client_for_owner(client, return_owner, NULL));
    CU_ASSERT_EQUAL(client->connected_users, 0);

    state.join_result = 0;
    CU_ASSERT_EQUAL(guac_client_add_user(client, owner, 0, NULL), 0);
    CU_ASSERT_PTR_EQUAL(state.observed_owner, owner);
    CU_ASSERT_PTR_EQUAL(guac_client_for_owner(client, return_owner, NULL), owner);

    guac_client_free(client);
    guac_user_free(owner);
}

/**
 * Test which verifies that the initialization thread for a guest sees the
 * original connection owner. Both rejected and successful guest joins must
 * preserve that owner after their join handlers return.
 */
void test_client__guest_preserves_owner(void) {
    guac_client* client = guac_client_alloc();
    guac_user* owner = guac_user_alloc();
    guac_user* guest = guac_user_alloc();
    owner_test_state owner_state = { 0 };
    owner_test_state guest_state = { .join_result = 1 };
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);
    CU_ASSERT_PTR_NOT_NULL_FATAL(owner);
    CU_ASSERT_PTR_NOT_NULL_FATAL(guest);
    client->join_handler = join_with_initialization;
    owner->client = client;
    owner->owner = 1;
    owner->data = &owner_state;
    guest->client = client;
    guest->data = &guest_state;

    CU_ASSERT_EQUAL(guac_client_add_user(client, owner, 0, NULL), 0);
    CU_ASSERT_EQUAL(guac_client_add_user(client, guest, 0, NULL), 1);
    CU_ASSERT_EQUAL(guest_state.thread_result, 0);
    CU_ASSERT_PTR_EQUAL(guest_state.observed_owner, owner);
    CU_ASSERT_PTR_EQUAL(guac_client_for_owner(client, return_owner, NULL), owner);

    guest_state.join_result = 0;
    CU_ASSERT_EQUAL(guac_client_add_user(client, guest, 0, NULL), 0);
    CU_ASSERT_PTR_EQUAL(guest_state.observed_owner, owner);
    CU_ASSERT_PTR_EQUAL(guac_client_for_owner(client, return_owner, NULL), owner);

    guac_client_free(client);
    guac_user_free(guest);
    guac_user_free(owner);
}
