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
 * Verifies that guac_terminal_selection_point_round_down() and
 * guac_terminal_selection_point_round_up() both correctly
 * calculate normalized column values for a point.
 */
void test_selection_point__rounding(void) {

    guac_terminal_selection_point a = {
        .column = 1,
        .row = 1,
        .side = GUAC_TERMINAL_COLUMN_SIDE_LEFT,
        .char_starting_column = 1,
        .char_width = 1
    };
   
    CU_ASSERT_EQUAL(0, guac_terminal_selection_point_round_down(&a));
    CU_ASSERT_EQUAL(1, guac_terminal_selection_point_round_up(&a));

    a.side = GUAC_TERMINAL_COLUMN_SIDE_RIGHT;
    CU_ASSERT_EQUAL(1, guac_terminal_selection_point_round_down(&a));
    CU_ASSERT_EQUAL(2, guac_terminal_selection_point_round_up(&a));

}

/**
 * Verifies that guac_terminal_selection_point_round_down() and
 * guac_terminal_selection_point_round_up() both correctly
 * calculate normalized column values for points with wide characters.
 */
void test_selection_point__rounding_wide(void) {

    guac_terminal_selection_point a = {
        .column = 1,
        .row = 1,
        .side = GUAC_TERMINAL_COLUMN_SIDE_LEFT,
        .char_starting_column = 1,
        .char_width = 2 
    };
   
    CU_ASSERT_EQUAL(0, guac_terminal_selection_point_round_down(&a));
    CU_ASSERT_EQUAL(1, guac_terminal_selection_point_round_up(&a));

    a.side = GUAC_TERMINAL_COLUMN_SIDE_RIGHT;
    CU_ASSERT_EQUAL(0, guac_terminal_selection_point_round_down(&a));
    CU_ASSERT_EQUAL(3, guac_terminal_selection_point_round_up(&a));

    a.side = GUAC_TERMINAL_COLUMN_SIDE_LEFT;
    a.column = 2;
    CU_ASSERT_EQUAL(0, guac_terminal_selection_point_round_down(&a));
    CU_ASSERT_EQUAL(3, guac_terminal_selection_point_round_up(&a));

    a.side = GUAC_TERMINAL_COLUMN_SIDE_RIGHT;
    CU_ASSERT_EQUAL(2, guac_terminal_selection_point_round_down(&a));
    CU_ASSERT_EQUAL(3, guac_terminal_selection_point_round_up(&a));

}
