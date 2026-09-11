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

#include "dbshell/line-editor.h"

#include <CUnit/CUnit.h>
#include <string.h>

/**
 * Verifies insertion of text at the cursor position.
 */
void test_line__insert(void) {

    guac_dbshell_line line;
    guac_dbshell_line_init(&line);

    guac_dbshell_line_insert(&line, "hello", 5);
    CU_ASSERT_STRING_EQUAL(line.buffer, "hello");
    CU_ASSERT_EQUAL(line.cursor, 5);

    /* Insert in the middle */
    line.cursor = 4;
    guac_dbshell_line_insert(&line, "XY", 2);
    CU_ASSERT_STRING_EQUAL(line.buffer, "hellXYo");
    CU_ASSERT_EQUAL(line.cursor, 6);

    guac_dbshell_line_destroy(&line);

}

/**
 * Verifies that insertion grows the buffer beyond its initial size.
 */
void test_line__insert_growth(void) {

    guac_dbshell_line line;
    guac_dbshell_line_init(&line);

    for (int i = 0; i < 100; i++)
        guac_dbshell_line_insert(&line, "0123456789", 10);

    CU_ASSERT_EQUAL(line.length, 1000);
    CU_ASSERT_EQUAL(line.cursor, 1000);
    CU_ASSERT_EQUAL(strlen(line.buffer), 1000);

    guac_dbshell_line_destroy(&line);

}

/**
 * Verifies backspace and delete across multi-byte UTF-8 characters.
 */
void test_line__utf8_editing(void) {

    guac_dbshell_line line;
    guac_dbshell_line_init(&line);

    /* "aé中" = 1 + 2 + 3 bytes */
    guac_dbshell_line_insert(&line, "a\xC3\xA9\xE4\xB8\xAD", 6);
    CU_ASSERT_EQUAL(line.length, 6);

    /* Backspace removes the entire 3-byte character */
    guac_dbshell_line_backspace(&line);
    CU_ASSERT_STRING_EQUAL(line.buffer, "a\xC3\xA9");
    CU_ASSERT_EQUAL(line.cursor, 3);

    /* Cursor movement is by character */
    guac_dbshell_line_left(&line);
    CU_ASSERT_EQUAL(line.cursor, 1);
    guac_dbshell_line_left(&line);
    CU_ASSERT_EQUAL(line.cursor, 0);

    /* Delete removes the character at the cursor */
    guac_dbshell_line_delete(&line);
    CU_ASSERT_STRING_EQUAL(line.buffer, "\xC3\xA9");
    CU_ASSERT_EQUAL(line.cursor, 0);

    guac_dbshell_line_right(&line);
    CU_ASSERT_EQUAL(line.cursor, 2);

    guac_dbshell_line_destroy(&line);

}

/**
 * Verifies that backspace at the beginning and delete at the end of the
 * line are no-ops.
 */
void test_line__edit_bounds(void) {

    guac_dbshell_line line;
    guac_dbshell_line_init(&line);

    guac_dbshell_line_backspace(&line);
    guac_dbshell_line_delete(&line);
    guac_dbshell_line_left(&line);
    guac_dbshell_line_right(&line);

    CU_ASSERT_EQUAL(line.length, 0);
    CU_ASSERT_EQUAL(line.cursor, 0);

    guac_dbshell_line_insert(&line, "ab", 2);
    guac_dbshell_line_delete(&line);
    CU_ASSERT_STRING_EQUAL(line.buffer, "ab");

    guac_dbshell_line_destroy(&line);

}

/**
 * Verifies the kill operations (Ctrl+U, Ctrl+K, Ctrl+W).
 */
void test_line__kill(void) {

    guac_dbshell_line line;
    guac_dbshell_line_init(&line);

    guac_dbshell_line_set(&line, "select * from users");

    /* Ctrl+W removes the word before the cursor */
    guac_dbshell_line_kill_word(&line);
    CU_ASSERT_STRING_EQUAL(line.buffer, "select * from ");

    /* Ctrl+W skips whitespace before the word */
    guac_dbshell_line_kill_word(&line);
    CU_ASSERT_STRING_EQUAL(line.buffer, "select * ");

    /* Ctrl+U removes everything before the cursor */
    line.cursor = 7;
    guac_dbshell_line_kill_before(&line);
    CU_ASSERT_STRING_EQUAL(line.buffer, "* ");
    CU_ASSERT_EQUAL(line.cursor, 0);

    /* Ctrl+K removes everything at and after the cursor */
    guac_dbshell_line_set(&line, "abcdef");
    line.cursor = 3;
    guac_dbshell_line_kill_after(&line);
    CU_ASSERT_STRING_EQUAL(line.buffer, "abc");

    guac_dbshell_line_destroy(&line);

}

/**
 * Verifies wholesale replacement of the line content.
 */
void test_line__set(void) {

    guac_dbshell_line line;
    guac_dbshell_line_init(&line);

    guac_dbshell_line_insert(&line, "old content", 11);
    guac_dbshell_line_set(&line, "new");

    CU_ASSERT_STRING_EQUAL(line.buffer, "new");
    CU_ASSERT_EQUAL(line.length, 3);
    CU_ASSERT_EQUAL(line.cursor, 3);

    guac_dbshell_line_set(&line, "");
    CU_ASSERT_EQUAL(line.length, 0);
    CU_ASSERT_EQUAL(line.cursor, 0);

    guac_dbshell_line_destroy(&line);

}
