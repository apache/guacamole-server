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
#include <fcntl.h>
#include <guacamole/client.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>
#include <guacamole/user.h>

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/**
 * State captured by the test auth_response_handler so the test can
 * verify what it was invoked with.
 */
typedef struct auth_capture {

    /**
     * Number of times the capturing handler has been invoked.
     */
    int call_count;

    /**
     * Stream index of the most recent invocation's stream.
     */
    int stream_index;

    /**
     * Mimetype argument of the most recent invocation.
     */
    char mimetype[64];

    /**
     * Challenge id argument of the most recent invocation.
     */
    char challenge_id[64];

} auth_capture;

/**
 * auth_response_handler that copies its arguments into the user's data
 * pointer (assumed to be an auth_capture) so the test can assert them.
 *
 * @param user
 *     The user whose handler is being invoked. user->data is expected
 *     to point at an auth_capture.
 *
 * @param stream
 *     The stream announced by the inbound auth-response.
 *
 * @param mimetype
 *     The mimetype announced by the inbound auth-response.
 *
 * @param challenge_id
 *     The challenge id announced by the inbound auth-response.
 *
 * @return
 *     Zero, always; the capture handler never fails.
 */
static int capture_auth(guac_user* user, guac_stream* stream,
        char* mimetype, char* challenge_id) {
    auth_capture* cap = (auth_capture*) user->data;
    cap->call_count++;
    cap->stream_index = stream->index;
    snprintf(cap->mimetype, sizeof(cap->mimetype), "%s", mimetype);
    snprintf(cap->challenge_id, sizeof(cap->challenge_id), "%s",
            challenge_id);
    return 0;
}

/**
 * Allocates a guac_user attached to the given client, installs
 * capture_auth as the auth_response_handler, and points user->data at
 * the given capture struct so the handler can record its arguments.
 *
 * @param client
 *     The client the new user should be attached to.
 *
 * @param cap
 *     The capture struct that capture_auth will write into.
 *
 * @return
 *     The allocated user. Caller must free via teardown_auth_user.
 */
static guac_user* setup_auth_user(guac_client* client, auth_capture* cap) {

    guac_user* user = guac_user_alloc();
    CU_ASSERT_PTR_NOT_NULL_FATAL(user);
    user->client = client;
    user->data = cap;
    user->auth_response_handler = capture_auth;

    /* Wire the user's outbound socket to /dev/null so any ack writes do
     * not crash. */
    int devnull = open("/dev/null", O_WRONLY);
    CU_ASSERT_TRUE_FATAL(devnull >= 0);
    user->socket = guac_socket_open(devnull);
    CU_ASSERT_PTR_NOT_NULL_FATAL(user->socket);

    return user;

}

/**
 * Frees a user previously allocated by setup_auth_user, closing its
 * outbound socket first.
 *
 * @param user
 *     The user to free.
 */
static void teardown_auth_user(guac_user* user) {
    guac_socket_free(user->socket);
    user->socket = NULL;
    guac_user_free(user);
}

/**
 * Verifies that an auth-response instruction dispatches to the user's
 * auth_response_handler with the correct arguments.
 */
void test_user__auth_response_dispatch(void) {

    guac_client* client = guac_client_alloc();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    auth_capture cap = { 0 };
    guac_user* user = setup_auth_user(client, &cap);

    char stream_idx[]   = "11";
    char mimetype[]     = "application/x-webauthn-get+json";
    char challenge_id[] = "bbbbbbbb-bbbb-4bbb-8bbb-bbbbbbbbbbbb";

    char* argv[] = { stream_idx, mimetype, challenge_id };
    CU_ASSERT_EQUAL(
        guac_user_handle_instruction(user, "auth-response", 3, argv), 0);

    CU_ASSERT_EQUAL(cap.call_count, 1);
    CU_ASSERT_EQUAL(cap.stream_index, 11);
    CU_ASSERT_STRING_EQUAL(cap.mimetype,
            "application/x-webauthn-get+json");
    CU_ASSERT_STRING_EQUAL(cap.challenge_id,
            "bbbbbbbb-bbbb-4bbb-8bbb-bbbbbbbbbbbb");

    teardown_auth_user(user);
    guac_client_free(client);

}

/**
 * Verifies that an auth-response instruction with no registered handler
 * does not crash and does not invoke the response handler.
 */
void test_user__auth_response_no_handler(void) {

    guac_client* client = guac_client_alloc();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    auth_capture cap = { 0 };
    guac_user* user = setup_auth_user(client, &cap);
    user->auth_response_handler = NULL;

    char stream_idx[]   = "3";
    char mimetype[]     = "application/x-webauthn-create+json";
    char challenge_id[] = "cccccccc-cccc-4ccc-8ccc-cccccccccccc";

    char* argv[] = { stream_idx, mimetype, challenge_id };
    CU_ASSERT_EQUAL(
        guac_user_handle_instruction(user, "auth-response", 3, argv), 0);

    CU_ASSERT_EQUAL(cap.call_count, 0);

    teardown_auth_user(user);
    guac_client_free(client);

}
