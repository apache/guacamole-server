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
#include <guacamole/user-constants.h>

#include <stdlib.h>
#include <unistd.h>

/**
 * Count of free_handler invocations across the active test. The
 * free_handler increments this on every call.
 */
static int free_handler_calls = 0;

/**
 * The data pointer most recently passed to the free_handler.
 */
static void* last_free_handler_data = NULL;

/**
 * free_handler for the stream's data pointer. Records the invocation in
 * free_handler_calls and last_free_handler_data so the test can assert it.
 *
 * @param stream
 *     The stream being freed.
 */
static void counting_free_handler(guac_stream* stream) {
    free_handler_calls++;
    last_free_handler_data = stream->data;
}

/**
 * Verifies that a stream's free_handler fires when the stream is freed
 * via guac_user_free_stream while the user is still alive.
 */
void test_client__stream_free_handler_on_free_stream(void) {

    free_handler_calls = 0;
    last_free_handler_data = NULL;

    guac_client* client = guac_client_alloc();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_user* user = guac_user_alloc();
    CU_ASSERT_PTR_NOT_NULL_FATAL(user);
    user->client = client;

    guac_stream* stream = guac_user_alloc_stream(user);
    CU_ASSERT_PTR_NOT_NULL_FATAL(stream);

    int dummy = 42;
    stream->data = &dummy;
    stream->free_handler = counting_free_handler;

    guac_user_free_stream(user, stream);

    CU_ASSERT_EQUAL(free_handler_calls, 1);
    CU_ASSERT_PTR_EQUAL(last_free_handler_data, &dummy);

    guac_user_free(user);
    guac_client_free(client);

}

/**
 * Verifies that a free_handler on a stream still open at disconnect
 * fires during guac_user_free, so a stream whose end never arrived does
 * not leak its data pointer.
 */
void test_client__stream_free_handler_on_user_free(void) {

    free_handler_calls = 0;
    last_free_handler_data = NULL;

    guac_client* client = guac_client_alloc();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_user* user = guac_user_alloc();
    CU_ASSERT_PTR_NOT_NULL_FATAL(user);
    user->client = client;

    guac_stream* stream = guac_user_alloc_stream(user);
    CU_ASSERT_PTR_NOT_NULL_FATAL(stream);

    int dummy = 7;
    stream->data = &dummy;
    stream->free_handler = counting_free_handler;

    /* No guac_user_free_stream. Simulate disconnect with the stream
     * still open. */
    guac_user_free(user);

    CU_ASSERT_EQUAL(free_handler_calls, 1);
    CU_ASSERT_PTR_EQUAL(last_free_handler_data, &dummy);

    guac_client_free(client);

}

/**
 * Verifies that a stream allocated with no free_handler set (the
 * default) does not invoke any callback when freed. Confirms that the
 * free_handler field is opt-in and existing handlers are unaffected.
 */
void test_client__stream_free_handler_unset_is_noop(void) {

    free_handler_calls = 0;

    guac_client* client = guac_client_alloc();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_user* user = guac_user_alloc();
    CU_ASSERT_PTR_NOT_NULL_FATAL(user);
    user->client = client;

    guac_stream* stream = guac_user_alloc_stream(user);
    CU_ASSERT_PTR_NOT_NULL_FATAL(stream);

    int dummy = 99;
    stream->data = &dummy;
    /* No free_handler installed */

    guac_user_free_stream(user, stream);
    guac_user_free(user);
    guac_client_free(client);

    CU_ASSERT_EQUAL(free_handler_calls, 0);

}

/**
 * Regression test: verifies that opening an input stream via the regular
 * pipe handler path leaves the free_handler at NULL even when a stale
 * pointer was present on the slot before init. If __init_input_stream
 * forgot to reset free_handler, guac_user_free would walk the open slot
 * and call the stale function pointer.
 */
void test_client__stream_free_handler_reset_on_input_init(void) {

    free_handler_calls = 0;

    guac_client* client = guac_client_alloc();
    CU_ASSERT_PTR_NOT_NULL_FATAL(client);

    guac_user* user = guac_user_alloc();
    CU_ASSERT_PTR_NOT_NULL_FATAL(user);
    user->client = client;

    /* Send-side socket so the unsupported-pipe ack does not crash. */
    int devnull = open("/dev/null", O_WRONLY);
    CU_ASSERT_TRUE_FATAL(devnull >= 0);
    user->socket = guac_socket_open(devnull);
    CU_ASSERT_PTR_NOT_NULL_FATAL(user->socket);

    /* Simulate a stale free_handler pointer left over from prior use of
     * the slot's storage. __init_input_stream must reset this when the
     * slot is reused. */
    user->__input_streams[0].free_handler = counting_free_handler;

    /* Send a pipe instruction. With no pipe_handler installed, the
     * dispatcher will ack unsupported and return. The slot is now
     * "open" (index != CLOSED) but should have its free_handler
     * cleared. */
    char idx[]  = "0";
    char mt[]   = "text/plain";
    char name[] = "some-pipe";
    char* pipe_argv[] = { idx, mt, name };
    CU_ASSERT_EQUAL(guac_user_handle_instruction(user, "pipe", 3, pipe_argv), 0);

    /* Disconnect with the stream still open. guac_user_free should
     * walk the slot and find free_handler == NULL, calling nothing. */
    guac_socket_free(user->socket);
    user->socket = NULL;
    guac_user_free(user);
    guac_client_free(client);

    CU_ASSERT_EQUAL(free_handler_calls, 0);

}
