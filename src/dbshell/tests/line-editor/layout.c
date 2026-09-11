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
#include <locale.h>
#include <string.h>

/**
 * Populates the given line with the given text and computes its layout
 * with the given prompt and terminal width.
 *
 * @param prompt
 *     The prompt string to lay out before the text.
 *
 * @param text
 *     The text content of the line.
 *
 * @param cursor
 *     The byte position of the cursor within the text.
 *
 * @param cols
 *     The terminal width, in columns.
 *
 * @param layout
 *     The structure to populate with the computed layout.
 */
static void compute(const char* prompt, const char* text, int cursor,
        int cols, guac_dbshell_layout* layout) {

    guac_dbshell_line line;
    guac_dbshell_line_init(&line);
    guac_dbshell_line_set(&line, text);
    line.cursor = cursor;

    guac_dbshell_line_layout(prompt, &line, cols, layout);

    guac_dbshell_line_destroy(&line);

}

/**
 * Verifies the layout of input which fits within a single display row.
 */
void test_layout__single_row(void) {

    guac_dbshell_layout layout;

    compute("db> ", "select", 6, 80, &layout);
    CU_ASSERT_EQUAL(layout.rows, 1);
    CU_ASSERT_EQUAL(layout.cursor_row, 0);
    CU_ASSERT_EQUAL(layout.cursor_col, 10);
    CU_ASSERT_FALSE(layout.forced_wrap);

    /* Cursor in the middle of the text */
    compute("db> ", "select", 3, 80, &layout);
    CU_ASSERT_EQUAL(layout.cursor_col, 7);

    /* Empty line */
    compute("db> ", "", 0, 80, &layout);
    CU_ASSERT_EQUAL(layout.rows, 1);
    CU_ASSERT_EQUAL(layout.cursor_col, 4);

}

/**
 * Verifies the layout of input which wraps across display rows.
 */
void test_layout__wrap(void) {

    guac_dbshell_layout layout;

    /* Prompt (4) + text (10) across 10 columns: rows 0-1 */
    compute("db> ", "0123456789", 10, 10, &layout);
    CU_ASSERT_EQUAL(layout.rows, 2);
    CU_ASSERT_EQUAL(layout.cursor_row, 1);
    CU_ASSERT_EQUAL(layout.cursor_col, 4);
    CU_ASSERT_FALSE(layout.forced_wrap);

    /* Cursor exactly at a row boundary is displayed at the start of the
     * following row */
    compute("db> ", "0123456789", 6, 10, &layout);
    CU_ASSERT_EQUAL(layout.cursor_row, 1);
    CU_ASSERT_EQUAL(layout.cursor_col, 0);

}

/**
 * Verifies that text ending exactly at the right margin forces a wrap.
 */
void test_layout__forced_wrap(void) {

    guac_dbshell_layout layout;

    /* Prompt (4) + text (6) = exactly 10 columns */
    compute("db> ", "012345", 6, 10, &layout);
    CU_ASSERT_TRUE(layout.forced_wrap);
    CU_ASSERT_EQUAL(layout.rows, 2);
    CU_ASSERT_EQUAL(layout.cursor_row, 1);
    CU_ASSERT_EQUAL(layout.cursor_col, 0);

}

/**
 * Verifies that wide characters wrap as whole units, leaving a gap at the
 * right margin rather than splitting.
 */
void test_layout__wide_chars(void) {

    /* Width data for CJK characters requires a UTF-8 locale; skip this
     * test if the build host provides none */
    if (setlocale(LC_CTYPE, "C.UTF-8") == NULL
            && setlocale(LC_CTYPE, "en_US.UTF-8") == NULL)
        return;

    if (guac_dbshell_display_width("\xE4\xB8\xAD", 3) != 2)
        return;

    guac_dbshell_layout layout;

    /* Prompt (3) + "中中中" (6 columns) in 8 columns: the third character
     * (columns 7-8 would straddle the margin) wraps whole to row 1 */
    compute(">> ", "\xE4\xB8\xAD\xE4\xB8\xAD\xE4\xB8\xAD", 9, 8, &layout);
    CU_ASSERT_EQUAL(layout.rows, 2);
    CU_ASSERT_EQUAL(layout.cursor_row, 1);
    CU_ASSERT_EQUAL(layout.cursor_col, 2);

}

/**
 * Verifies that degenerate terminal widths are tolerated.
 */
void test_layout__degenerate(void) {

    guac_dbshell_layout layout;

    /* A zero-column terminal is laid out as a single column: five
     * characters occupy five rows, with the final character ending at the
     * right margin and forcing a wrap onto a sixth row */
    compute("> ", "abc", 3, 0, &layout);
    CU_ASSERT_EQUAL(layout.rows, 6);
    CU_ASSERT_TRUE(layout.forced_wrap);

    compute("", "", 0, 80, &layout);
    CU_ASSERT_EQUAL(layout.rows, 1);
    CU_ASSERT_EQUAL(layout.cursor_col, 0);

}

/**
 * Verifies UTF-8 display width measurement.
 */
void test_layout__display_width(void) {

    CU_ASSERT_EQUAL(guac_dbshell_display_width("abc", 3), 3);
    CU_ASSERT_EQUAL(guac_dbshell_display_width("", 0), 0);

    /* "é" is one column in any locale interpretation (either proper width
     * or the one-column fallback) */
    CU_ASSERT_EQUAL(guac_dbshell_display_width("\xC3\xA9", 2), 1);

}
