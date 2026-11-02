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

#include "display-priv.h"

#include <CUnit/CUnit.h>

#include <string.h>

/**
 * Tests that a combined operation which has become a NOP is not reused by a
 * later horizontal combination through another cell that still references it.
 */
void test_display_plan__combine_skips_noop_operations(void) {

    static guac_display display;
    static guac_display_layer layer;
    static guac_display_layer_cell cells[4];
    static guac_display_plan_operation ops[3];
    static guac_display_plan plan;

    memset(&display, 0, sizeof(display));
    memset(&layer, 0, sizeof(layer));
    memset(cells, 0, sizeof(cells));
    memset(ops, 0, sizeof(ops));
    memset(&plan, 0, sizeof(plan));

    /* Set up a two-by-two cell grid containing three operations. The second
     * operation spans both rows, so its reference remains in the lower-right
     * cell after being absorbed into the first operation in the top row. */
    display.pending_frame.layers = &layer;
    layer.pending_frame_cells = cells;
    layer.pending_frame_cells_width = 2;
    layer.pending_frame_cells_height = 2;
    guac_rect_init(&layer.pending_frame.dirty, 0, 0, 2, 2);

    ops[0].layer = &layer;
    ops[0].type = GUAC_DISPLAY_PLAN_OPERATION_IMG;
    ops[0].dirty_size = 1;
    guac_rect_init(&ops[0].dest, 0, 0, 1, 1);

    ops[1].layer = &layer;
    ops[1].type = GUAC_DISPLAY_PLAN_OPERATION_IMG;
    ops[1].dirty_size = 2;
    guac_rect_init(&ops[1].dest, 1, 0, 1, 2);

    ops[2].layer = &layer;
    ops[2].type = GUAC_DISPLAY_PLAN_OPERATION_RECT;
    ops[2].dirty_size = 1;
    guac_rect_init(&ops[2].dest, 0, 1, 1, 1);

    cells[0].related_op = &ops[0];
    cells[1].related_op = &ops[1];
    cells[2].related_op = &ops[2];
    cells[3].related_op = &ops[1];

    plan.display = &display;
    PFW_guac_display_plan_combine_horizontally(&plan);

    /* The top-row image updates should combine, and the absorbed operation
     * must remain a NOP when encountered again in the lower row. */
    CU_ASSERT_EQUAL(ops[1].type, GUAC_DISPLAY_PLAN_OPERATION_NOP);
    CU_ASSERT_EQUAL(ops[0].dest.left, 0);
    CU_ASSERT_EQUAL(ops[0].dest.top, 0);
    CU_ASSERT_EQUAL(ops[0].dest.right, 2);
    CU_ASSERT_EQUAL(ops[0].dest.bottom, 2);
    CU_ASSERT_EQUAL(ops[2].type, GUAC_DISPLAY_PLAN_OPERATION_RECT);
    CU_ASSERT_EQUAL(ops[2].dest.left, 0);
    CU_ASSERT_EQUAL(ops[2].dest.top, 1);
    CU_ASSERT_EQUAL(ops[2].dest.right, 1);
    CU_ASSERT_EQUAL(ops[2].dest.bottom, 2);
    CU_ASSERT_EQUAL(ops[2].dirty_size, 1);

}
