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

#include "terminal/terminal.h"
#include "terminal/terminal-priv.h"

#include <CUnit/CUnit.h>
#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <guacamole/protocol-constants.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>
#include <guacamole/user.h>

#include <pthread.h>
#include <stdarg.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

/**
 * Test fixture for the text-output helpers. Only the fields used by the
 * text-output implementation are initialized, avoiding the full terminal render
 * thread while still exercising real guac_client_for_owner(), user streams,
 * ACK handlers, and protocol serialization.
 */
typedef struct text_output_fixture {
    guac_client* client;
    guac_user* owner;
    guac_terminal* term;
    int read_fd;
    int write_fd;
    guac_client_log_level last_log_level;
    int log_count;
} text_output_fixture;

static void capture_log(guac_client* client, guac_client_log_level level,
        const char* format, va_list ap) {

    text_output_fixture* fixture = (text_output_fixture*) client->data;
    fixture->last_log_level = level;
    fixture->log_count++;

}

static text_output_fixture* text_output_fixture_alloc(void) {

    int pipe_fds[2];
    CU_ASSERT_EQUAL_FATAL(pipe(pipe_fds), 0);

    text_output_fixture* fixture = guac_mem_zalloc(sizeof(text_output_fixture));
    fixture->read_fd = pipe_fds[0];
    fixture->write_fd = pipe_fds[1];

    fixture->client = guac_client_alloc();
    CU_ASSERT_PTR_NOT_NULL_FATAL(fixture->client);
    fixture->client->data = fixture;
    fixture->client->log_handler = capture_log;

    fixture->owner = guac_user_alloc();
    CU_ASSERT_PTR_NOT_NULL_FATAL(fixture->owner);
    fixture->owner->owner = 1;
    fixture->owner->socket = guac_socket_open(fixture->write_fd);
    CU_ASSERT_PTR_NOT_NULL_FATAL(fixture->owner->socket);
    fixture->client->__owner = fixture->owner;

    fixture->term = guac_mem_zalloc(sizeof(guac_terminal));
    fixture->term->client = fixture->client;
    pthread_mutex_init(&fixture->term->lock, NULL);
    pthread_cond_init(&fixture->term->text_output_acked, NULL);

    return fixture;

}

static void text_output_fixture_free(text_output_fixture* fixture) {

    if (fixture->term != NULL) {
        guac_terminal_text_output_close(fixture->term);
        pthread_cond_destroy(&fixture->term->text_output_acked);
        pthread_mutex_destroy(&fixture->term->lock);
        guac_mem_free(fixture->term);
    }

    if (fixture->owner != NULL) {
        if (fixture->owner->socket != NULL)
            guac_socket_free(fixture->owner->socket);
        guac_user_free(fixture->owner);
    }

    if (fixture->client != NULL) {
        fixture->client->__owner = NULL;
        guac_client_free(fixture->client);
    }

    if (fixture->read_fd != -1)
        close(fixture->read_fd);

    guac_mem_free(fixture);

}

static char* text_output_fixture_read(text_output_fixture* fixture) {

    guac_socket_free(fixture->owner->socket);
    fixture->owner->socket = NULL;

    char* buffer = guac_mem_zalloc(8192);
    int offset = 0;
    int numread;

    while ((numread = read(fixture->read_fd, buffer + offset,
                    8191 - offset)) > 0)
        offset += numread;

    buffer[offset] = '\0';
    close(fixture->read_fd);
    fixture->read_fd = -1;

    return buffer;

}

void test_text_output__disable_copy_blocks_open(void) {

    CU_ASSERT_FALSE(guac_terminal_text_output_should_open(0, 0));
    CU_ASSERT_FALSE(guac_terminal_text_output_should_open(0, 1));
    CU_ASSERT_TRUE(guac_terminal_text_output_should_open(1, 0));
    CU_ASSERT_FALSE(guac_terminal_text_output_should_open(1, 1));

}

void test_text_output__opens_owner_stream_and_acks_blobs(void) {

    text_output_fixture* fixture = text_output_fixture_alloc();

    guac_terminal_text_output_open(fixture->term, "STDOUT", 0);
    CU_ASSERT_PTR_NOT_NULL_FATAL(fixture->term->text_output_stream);
    CU_ASSERT_EQUAL(fixture->term->text_output_stream->index % 2, 0);
    CU_ASSERT_PTR_EQUAL(fixture->term->text_output_stream->data, fixture->term);
    CU_ASSERT_PTR_NOT_NULL(fixture->term->text_output_stream->ack_handler);

    guac_terminal_text_output_write(fixture->term, "hello", 5);
    guac_terminal_lock(fixture->term);
    guac_terminal_text_output_flush(fixture->term);
    guac_terminal_unlock(fixture->term);
    CU_ASSERT_EQUAL(fixture->term->text_output_inflight, 1);

    fixture->term->text_output_stream->ack_handler(fixture->owner,
            fixture->term->text_output_stream, "OK",
            GUAC_PROTOCOL_STATUS_SUCCESS);
    CU_ASSERT_EQUAL(fixture->term->text_output_inflight, 0);

    guac_terminal_text_output_close(fixture->term);
    char* instructions = text_output_fixture_read(fixture);

    CU_ASSERT_PTR_NOT_NULL(strstr(instructions,
                "4.pipe,1.0,24.application/octet-stream,6.STDOUT;"));
    CU_ASSERT_PTR_NOT_NULL(strstr(instructions,
                "4.blob,1.0,8.aGVsbG8=;"));
    CU_ASSERT_PTR_NOT_NULL(strstr(instructions, "3.end,1.0;"));

    guac_mem_free(instructions);
    text_output_fixture_free(fixture);

}

void test_text_output__tee_mode_drops_when_consumer_stalls(void) {

    text_output_fixture* fixture = text_output_fixture_alloc();

    guac_terminal_text_output_open(fixture->term, "STDOUT", 0);
    fixture->term->text_output_inflight = GUAC_TERMINAL_TEXT_OUTPUT_MAX_INFLIGHT;

    guac_terminal_text_output_write(fixture->term, "drop", 4);
    guac_terminal_lock(fixture->term);
    guac_terminal_text_output_flush(fixture->term);
    guac_terminal_unlock(fixture->term);

    CU_ASSERT_EQUAL(fixture->client->state, GUAC_CLIENT_RUNNING);
    CU_ASSERT_EQUAL(fixture->term->text_output_length, 0);
    CU_ASSERT_EQUAL(fixture->term->text_output_inflight,
            GUAC_TERMINAL_TEXT_OUTPUT_MAX_INFLIGHT);
    CU_ASSERT_EQUAL(fixture->last_log_level, GUAC_LOG_WARNING);

    guac_terminal_text_output_close(fixture->term);
    char* instructions = text_output_fixture_read(fixture);

    CU_ASSERT_PTR_NOT_NULL(strstr(instructions,
                "4.pipe,1.0,24.application/octet-stream,6.STDOUT;"));
    CU_ASSERT_PTR_NULL(strstr(instructions, "4.blob"));
    CU_ASSERT_PTR_NOT_NULL(strstr(instructions, "3.end,1.0;"));

    guac_mem_free(instructions);
    text_output_fixture_free(fixture);

}

/**
 * Regression test: the outstanding-output bound must be expressed in bytes, not
 * merely in blob count. Raw mode flushes every write as its own blob, so a
 * client which is acking perfectly normally still accumulates many small
 * outstanding blobs; a blob-count-only bound would abort a healthy session after
 * a trivial amount of interactive output.
 */
void test_text_output__many_tiny_blobs_do_not_trip_backpressure(void) {

    text_output_fixture* fixture = text_output_fixture_alloc();

    guac_terminal_text_output_open(fixture->term, "STDOUT", 1);

    /* Emit far more blobs than a naive count-based window would tolerate,
     * without acking any of them. The total volume remains tiny. */
    for (int i = 0; i < 200; i++)
        guac_terminal_text_output_write(fixture->term, "x", 1);

    CU_ASSERT_EQUAL(fixture->client->state, GUAC_CLIENT_RUNNING);
    CU_ASSERT_EQUAL(fixture->term->text_output_inflight, 200);
    CU_ASSERT_EQUAL(fixture->term->text_output_inflight_bytes, 200);

    text_output_fixture_free(fixture);

}

/**
 * Verifies that each ACK retires the oldest outstanding blob and reduces the
 * outstanding byte count by exactly that blob's size, rather than by a fixed
 * amount.
 */
void test_text_output__ack_retires_oldest_blob_bytes(void) {

    text_output_fixture* fixture = text_output_fixture_alloc();

    guac_terminal_text_output_open(fixture->term, "STDOUT", 1);

    guac_terminal_text_output_write(fixture->term, "abc", 3);
    guac_terminal_text_output_write(fixture->term, "defgh", 5);

    CU_ASSERT_EQUAL(fixture->term->text_output_inflight, 2);
    CU_ASSERT_EQUAL(fixture->term->text_output_inflight_bytes, 8);

    guac_stream* stream = fixture->term->text_output_stream;

    /* Acking retires the oldest blob (3 bytes), not the newest */
    stream->ack_handler(fixture->owner, stream, "OK",
            GUAC_PROTOCOL_STATUS_SUCCESS);
    CU_ASSERT_EQUAL(fixture->term->text_output_inflight, 1);
    CU_ASSERT_EQUAL(fixture->term->text_output_inflight_bytes, 5);

    stream->ack_handler(fixture->owner, stream, "OK",
            GUAC_PROTOCOL_STATUS_SUCCESS);
    CU_ASSERT_EQUAL(fixture->term->text_output_inflight, 0);
    CU_ASSERT_EQUAL(fixture->term->text_output_inflight_bytes, 0);

    /* A surplus ACK must not drive the counters negative */
    stream->ack_handler(fixture->owner, stream, "OK",
            GUAC_PROTOCOL_STATUS_SUCCESS);
    CU_ASSERT_EQUAL(fixture->term->text_output_inflight, 0);
    CU_ASSERT_EQUAL(fixture->term->text_output_inflight_bytes, 0);

    text_output_fixture_free(fixture);

}

/**
 * Verifies that the byte bound, rather than the blob count, is what ultimately
 * stops a stalled raw-mode consumer.
 */
void test_text_output__byte_bound_aborts_raw_mode(void) {

    text_output_fixture* fixture = text_output_fixture_alloc();

    guac_terminal_text_output_open(fixture->term, "STDOUT", 1);
    fixture->term->text_output_stall_timeout = 0;

    /* Simulate a large outstanding backlog well within the blob-count limit.
     * Set directly rather than written, as actually sending this volume would
     * fill the test pipe. */
    fixture->term->text_output_inflight = 1;
    fixture->term->text_output_inflight_bytes =
            GUAC_TERMINAL_TEXT_OUTPUT_MAX_INFLIGHT_BYTES;

    guac_terminal_text_output_write(fixture->term, "over", 4);

    CU_ASSERT_EQUAL(fixture->client->state, GUAC_CLIENT_STOPPING);
    CU_ASSERT_EQUAL(fixture->term->text_output_length, 0);
    CU_ASSERT_EQUAL(fixture->last_log_level, GUAC_LOG_ERROR);

    text_output_fixture_free(fixture);

}

/**
 * Verifies that closing the text-output stream releases it such that a late ACK
 * arriving afterwards is rejected before dispatch, rather than reaching the
 * terminal's ACK handler with a stale data pointer.
 */
void test_text_output__close_marks_stream_unroutable(void) {

    text_output_fixture* fixture = text_output_fixture_alloc();

    guac_terminal_text_output_open(fixture->term, "STDOUT", 0);

    guac_stream* stream = fixture->term->text_output_stream;
    CU_ASSERT_PTR_NOT_NULL_FATAL(stream);

    guac_terminal_text_output_close(fixture->term);

    /* The terminal must no longer reference the stream, and the stream must be
     * marked closed so that __guac_handle_ack() drops any late ACK for it */
    CU_ASSERT_PTR_NULL(fixture->term->text_output_stream);
    CU_ASSERT_EQUAL(stream->index, GUAC_USER_CLOSED_STREAM_INDEX);

    text_output_fixture_free(fixture);

}

/**
 * Acks every outstanding text-output blob once, after a short delay, as a
 * consumer that is keeping up but lagging would.
 */
static void* delayed_ack_thread(void* data) {

    text_output_fixture* fixture = (text_output_fixture*) data;

    /* Let the writer reach the full window and block */
    usleep(100000);

    guac_stream* stream = fixture->term->text_output_stream;
    while (fixture->term->text_output_inflight > 0)
        stream->ack_handler(fixture->owner, stream, "OK",
                GUAC_PROTOCOL_STATUS_SUCCESS);

    return NULL;

}

/**
 * Regression test: a raw-mode consumer which is merely slow must be throttled,
 * not disconnected. Sustained output (a large "cat", say) will always outrun a
 * consumer eventually; aborting in that situation kills healthy sessions, so
 * the writer waits for the window to drain instead.
 */
void test_text_output__raw_mode_throttles_rather_than_aborting(void) {

    text_output_fixture* fixture = text_output_fixture_alloc();

    guac_terminal_text_output_open(fixture->term, "STDOUT", 1);

    /* Present a full window, and a consumer which drains it shortly after */
    fixture->term->text_output_inflight = GUAC_TERMINAL_TEXT_OUTPUT_MAX_INFLIGHT;
    fixture->term->text_output_inflight_bytes = 4096;
    for (int i = 0; i < GUAC_TERMINAL_TEXT_OUTPUT_MAX_INFLIGHT; i++)
        fixture->term->text_output_inflight_sizes[i] = 16;

    pthread_t acker;
    pthread_create(&acker, NULL, delayed_ack_thread, fixture);

    guac_terminal_text_output_write(fixture->term, "throttled", 9);

    pthread_join(acker, NULL);

    /* The session must survive, and the data must have been sent rather than
     * dropped once room became available */
    CU_ASSERT_EQUAL(fixture->client->state, GUAC_CLIENT_RUNNING);
    CU_ASSERT_EQUAL(fixture->term->text_output_length, 0);

    guac_terminal_text_output_close(fixture->term);
    char* instructions = text_output_fixture_read(fixture);

    CU_ASSERT_PTR_NOT_NULL(strstr(instructions, "4.blob"));

    guac_mem_free(instructions);
    text_output_fixture_free(fixture);

}

/**
 * Regression test: a raw-mode writer waiting for window space must stop waiting
 * as soon as the connection is going away. Only an ACK signals that condition,
 * and a disconnected client never sends one, so a writer which ignored client
 * state would hold teardown up until the give-up deadline expired.
 */
void test_text_output__raw_mode_stops_waiting_once_disconnected(void) {

    text_output_fixture* fixture = text_output_fixture_alloc();

    guac_terminal_text_output_open(fixture->term, "STDOUT", 1);

    /* A generous give-up deadline: the test must not depend on reaching it */
    fixture->term->text_output_stall_timeout = 30;
    fixture->term->text_output_inflight = GUAC_TERMINAL_TEXT_OUTPUT_MAX_INFLIGHT;

    /* The connection is being torn down; no further ACK can arrive */
    fixture->client->state = GUAC_CLIENT_STOPPING;

    time_t started = time(NULL);
    guac_terminal_text_output_write(fixture->term, "gone", 4);
    time_t elapsed = time(NULL) - started;

    CU_ASSERT_TRUE(elapsed < 5);

    text_output_fixture_free(fixture);

}

void test_text_output__raw_mode_aborts_when_consumer_stalls(void) {

    text_output_fixture* fixture = text_output_fixture_alloc();

    guac_terminal_text_output_open(fixture->term, "STDOUT", 1);
    fixture->term->text_output_stall_timeout = 0;
    fixture->term->text_output_inflight = GUAC_TERMINAL_TEXT_OUTPUT_MAX_INFLIGHT;

    guac_terminal_text_output_write(fixture->term, "abort", 5);

    CU_ASSERT_EQUAL(fixture->client->state, GUAC_CLIENT_STOPPING);
    CU_ASSERT_EQUAL(fixture->term->text_output_length, 0);
    CU_ASSERT_EQUAL(fixture->term->text_output_inflight,
            GUAC_TERMINAL_TEXT_OUTPUT_MAX_INFLIGHT);
    CU_ASSERT_EQUAL(fixture->last_log_level, GUAC_LOG_ERROR);

    guac_terminal_text_output_close(fixture->term);
    char* instructions = text_output_fixture_read(fixture);

    CU_ASSERT_PTR_NOT_NULL(strstr(instructions,
                "4.pipe,1.0,24.application/octet-stream,6.STDOUT;"));
    CU_ASSERT_PTR_NULL(strstr(instructions, "4.blob"));
    CU_ASSERT_PTR_NOT_NULL(strstr(instructions, "3.end,1.0;"));

    guac_mem_free(instructions);
    text_output_fixture_free(fixture);

}
