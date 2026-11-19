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

/** State shared with the join handler and its initialization thread. */
typedef struct owner_test_state {
    guac_user* observed_owner;
    int join_result;
    int thread_result;
} owner_test_state;

/** Return the owner supplied by guac_client_for_owner(). */
static void* return_owner(guac_user* owner, void* data) {
    return owner;
}

/** Model protocol initialization that exposes a filesystem to the owner. */
static void* initialize_protocol(void* data) {
    guac_user* user = (guac_user*) data;
    return guac_client_for_owner(user->client, return_owner, NULL);
}

/** Force protocol initialization to finish before the join handler returns. */
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

/** An initialization thread must be able to find its connection owner. */
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

/** A rejected owner must not leave a dangling pointer, and may retry. */
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

/** Neither successful nor rejected guests may replace the owner. */
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
