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

#include "control.h"

#include <CUnit/CUnit.h>
#include <stdlib.h>
#include <string.h>

/**
 * The maximum number of lines recorded by test_feed_record().
 */
#define TEST_FEED_MAX_LINES 8

/**
 * Lines delivered to the handler during a single test.
 */
typedef struct test_feed_lines {

    /**
     * Copies of each line delivered, in order.
     */
    char* lines[TEST_FEED_MAX_LINES];

    /**
     * The number of lines delivered.
     */
    int count;

} test_feed_lines;

/**
 * guac_ipmi_control_line_handler which records a copy of each line delivered
 * into the given test_feed_lines.
 *
 * @param line
 *     The complete line delivered.
 *
 * @param data
 *     The test_feed_lines to record into.
 */
static void test_feed_record(const char* line, void* data) {
    test_feed_lines* recorded = (test_feed_lines*) data;
    if (recorded->count < TEST_FEED_MAX_LINES)
        recorded->lines[recorded->count] = strdup(line);
    recorded->count++;
}

/**
 * Frees all lines recorded within the given test_feed_lines.
 *
 * @param recorded
 *     The recorded lines to free.
 */
static void test_feed_free(test_feed_lines* recorded) {
    for (int i = 0; i < recorded->count && i < TEST_FEED_MAX_LINES; i++)
        free(recorded->lines[i]);
}

/**
 * Feeds the given null-terminated string to the given stream state.
 *
 * @param cs
 *     The stream state to feed.
 *
 * @param str
 *     The string to feed.
 *
 * @param recorded
 *     The test_feed_lines receiving any completed lines.
 */
static void test_feed_str(guac_ipmi_control_stream* cs, const char* str,
        test_feed_lines* recorded) {
    guac_ipmi_control_feed(cs, str, strlen(str), test_feed_record, recorded);
}

/**
 * Verifies that each newline-terminated line in a single blob is delivered,
 * in order, and that empty lines are skipped.
 */
void test_feed__multiple_lines(void) {
    guac_ipmi_control_stream cs = { 0 };
    test_feed_lines recorded = { 0 };
    test_feed_str(&cs, "one\n\ntwo\n", &recorded);
    CU_ASSERT_EQUAL_FATAL(recorded.count, 2);
    CU_ASSERT_STRING_EQUAL(recorded.lines[0], "one");
    CU_ASSERT_STRING_EQUAL(recorded.lines[1], "two");
    test_feed_free(&recorded);
}

/**
 * Verifies that a line split across several blobs is delivered once, whole,
 * when its newline arrives, and that an unterminated line is not delivered.
 */
void test_feed__split_across_blobs(void) {
    guac_ipmi_control_stream cs = { 0 };
    test_feed_lines recorded = { 0 };
    test_feed_str(&cs, "{\"command\":", &recorded);
    CU_ASSERT_EQUAL(recorded.count, 0);
    test_feed_str(&cs, "\"status\"}\npartial", &recorded);
    CU_ASSERT_EQUAL_FATAL(recorded.count, 1);
    CU_ASSERT_STRING_EQUAL(recorded.lines[0], "{\"command\":\"status\"}");
    test_feed_free(&recorded);
}

/**
 * Verifies that a line exactly filling the buffer (less its null terminator)
 * is still delivered intact.
 */
void test_feed__max_length(void) {
    guac_ipmi_control_stream cs = { 0 };
    test_feed_lines recorded = { 0 };
    char line[GUAC_IPMI_CONTROL_BUFFER_LENGTH];
    memset(line, 'a', sizeof(line) - 1);
    line[sizeof(line) - 1] = '\0';
    test_feed_str(&cs, line, &recorded);
    test_feed_str(&cs, "\n", &recorded);
    CU_ASSERT_EQUAL_FATAL(recorded.count, 1);
    CU_ASSERT_EQUAL(strlen(recorded.lines[0]), GUAC_IPMI_CONTROL_BUFFER_LENGTH - 1);
    test_feed_free(&recorded);
}

/**
 * Verifies that no part of an oversized line is delivered, even when its tail
 * would itself be a valid command, and that the line following it is
 * delivered normally. This is the regression test for a bug in which only the
 * buffered portion was dropped and the tail was dispatched at the next
 * newline.
 */
void test_feed__oversized_line_discarded(void) {
    guac_ipmi_control_stream cs = { 0 };
    test_feed_lines recorded = { 0 };
    char filler[GUAC_IPMI_CONTROL_BUFFER_LENGTH + 1];
    memset(filler, 'x', sizeof(filler) - 1);
    filler[sizeof(filler) - 1] = '\0';
    test_feed_str(&cs, filler, &recorded);
    test_feed_str(&cs, "{\"command\":\"power-off\"}\n", &recorded);
    CU_ASSERT_EQUAL(recorded.count, 0);
    test_feed_str(&cs, "{\"command\":\"status\"}\n", &recorded);
    CU_ASSERT_EQUAL_FATAL(recorded.count, 1);
    CU_ASSERT_STRING_EQUAL(recorded.lines[0], "{\"command\":\"status\"}");
    test_feed_free(&recorded);
}
