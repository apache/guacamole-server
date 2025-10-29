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
 * Verifies that guac_terminal_selection_point_is_after() correctly
 * determins the order of two points.
 */
void test_selection_point__is_point_after(void) {

    guac_terminal_selection_point a = {
        .column = 1,
        .row = 1,
        .side = GUAC_TERMINAL_COLUMN_SIDE_LEFT,
    };
    guac_terminal_selection_point b = {
        .column = 1,
        .row = 1,
        .side = GUAC_TERMINAL_COLUMN_SIDE_LEFT,
    };

    /* a and b are the same, so neither are after the other */
    CU_ASSERT(!guac_terminal_selection_point_is_after(&a, &b));
    CU_ASSERT(!guac_terminal_selection_point_is_after(&b, &a));

    /* b is after a but with same column */
    b.side = GUAC_TERMINAL_COLUMN_SIDE_RIGHT;
    CU_ASSERT(!guac_terminal_selection_point_is_after(&a, &b));
    CU_ASSERT(guac_terminal_selection_point_is_after(&b, &a));

    /* b is after a but with different columns */
    b.column = 2;
    CU_ASSERT(!guac_terminal_selection_point_is_after(&a, &b));
    CU_ASSERT(guac_terminal_selection_point_is_after(&b, &a));

}
