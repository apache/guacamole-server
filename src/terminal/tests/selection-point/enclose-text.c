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

#include "terminal/selection-point.h"

#include <CUnit/CUnit.h>
#include <stdlib.h>

/**
 * Verifies that guac_terminal_selection_points_enclose_text() correctly 
 * calculates if the range contains a full character.
 */
void test_selection_point__enclose_text(void) {

    guac_terminal_selection_point a = {
        .column = 1,
        .row = 1,
        .side = GUAC_TERMINAL_COLUMN_SIDE_LEFT,
        .char_starting_column = 1,
        .char_width = 1
    };
    guac_terminal_selection_point b = {
        .column = 1,
        .row = 1,
        .side = GUAC_TERMINAL_COLUMN_SIDE_LEFT,
        .char_starting_column = 1,
        .char_width = 1
    };

    CU_ASSERT(!guac_terminal_selection_points_enclose_text(&a, &b));

    b.side = GUAC_TERMINAL_COLUMN_SIDE_RIGHT;
    CU_ASSERT(guac_terminal_selection_points_enclose_text(&a, &b));

    a.side = GUAC_TERMINAL_COLUMN_SIDE_RIGHT;
    CU_ASSERT(!guac_terminal_selection_points_enclose_text(&a, &b));

    b.column = 2;
    b.char_starting_column = 2;
    b.side = GUAC_TERMINAL_COLUMN_SIDE_LEFT;
    CU_ASSERT(!guac_terminal_selection_points_enclose_text(&a, &b));

    b.side = GUAC_TERMINAL_COLUMN_SIDE_RIGHT;
    CU_ASSERT(guac_terminal_selection_points_enclose_text(&a, &b));

    b.column = 3;
    b.char_starting_column = 3;
    b.side = GUAC_TERMINAL_COLUMN_SIDE_LEFT;
    CU_ASSERT(guac_terminal_selection_points_enclose_text(&a, &b));

}

/**
 * Verifies that guac_terminal_selection_points_enclose_text() correctly 
 * calculates if the range contains a full character with wide characters.
 */
void test_selection_point__enclose_wide_text(void) {

    guac_terminal_selection_point a = {
        .column = 1,
        .row = 1,
        .side = GUAC_TERMINAL_COLUMN_SIDE_LEFT,
        .char_starting_column = 1,
        .char_width = 2 
    };
    guac_terminal_selection_point b = {
        .column = 1,
        .row = 1,
        .side = GUAC_TERMINAL_COLUMN_SIDE_LEFT,
        .char_starting_column = 1,
        .char_width = 2 
    };

    /* Check point within a single character */
    CU_ASSERT(!guac_terminal_selection_points_enclose_text(&a, &b));

    b.side = GUAC_TERMINAL_COLUMN_SIDE_RIGHT;
    CU_ASSERT(!guac_terminal_selection_points_enclose_text(&a, &b));

    b.column = 2;
    b.side = GUAC_TERMINAL_COLUMN_SIDE_LEFT;
    CU_ASSERT(!guac_terminal_selection_points_enclose_text(&a, &b));

    b.side = GUAC_TERMINAL_COLUMN_SIDE_RIGHT;
    CU_ASSERT(guac_terminal_selection_points_enclose_text(&a, &b));

    /* Check with points on neighboring characters */
    b.column = 3;
    b.char_starting_column = 3;
    CU_ASSERT(guac_terminal_selection_points_enclose_text(&a, &b));

    a.side = GUAC_TERMINAL_COLUMN_SIDE_RIGHT;
    CU_ASSERT(!guac_terminal_selection_points_enclose_text(&a, &b));

    a.column = 2;
    a.side = GUAC_TERMINAL_COLUMN_SIDE_LEFT;
    CU_ASSERT(!guac_terminal_selection_points_enclose_text(&a, &b));

    a.side = GUAC_TERMINAL_COLUMN_SIDE_RIGHT;
    CU_ASSERT(!guac_terminal_selection_points_enclose_text(&a, &b));

    b.column = 4;
    b.side = GUAC_TERMINAL_COLUMN_SIDE_LEFT;
    CU_ASSERT(!guac_terminal_selection_points_enclose_text(&a, &b));

    b.side = GUAC_TERMINAL_COLUMN_SIDE_RIGHT;
    CU_ASSERT(guac_terminal_selection_points_enclose_text(&a, &b));

}
