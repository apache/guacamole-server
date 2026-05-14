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

#include "display-plan.h"
#include "display-priv.h"
#include "guacamole/mem.h"

#include <CUnit/CUnit.h>
#include <stdint.h>

/**
 * Verifies that identical pixels are repainted only when the layer is resized,
 * regardless of whether a dirty region is reported.
 *
 * @param old_size
 *     The width and height of the previous frame, in pixels.
 *
 * @param new_size
 *     The width and height of the pending frame, in pixels.
 *
 * @param report_dirty
 *     Non-zero to mark a one-pixel region dirty, or zero to leave it empty.
 */
static void check_resize(int old_size, int new_size, int report_dirty) {

    uint32_t pixels[128 * 128] = { 0 };
    guac_display_layer_cell cells[4] = { 0 };
    guac_display_layer layer = {
        .last_frame = {
            .width = old_size,
            .height = old_size,
            .buffer = (unsigned char*) pixels,
            .buffer_stride = 128 * sizeof(uint32_t)
        },
        .pending_frame = {
            .width = new_size,
            .height = new_size,
            .buffer = (unsigned char*) pixels,
            .buffer_stride = 128 * sizeof(uint32_t)
        },
        .pending_frame_cells = cells,
        .pending_frame_cells_width = 2,
        .pending_frame_cells_height = 2
    };
    guac_display* display = guac_mem_zalloc(sizeof(guac_display));
    display->pending_frame.layers = &layer;

    if (report_dirty)
        guac_rect_init(&layer.pending_frame.dirty, 0, 0, 1, 1);

    guac_display_plan* plan = PFW_LFR_guac_display_plan_create(display);
    if (old_size == new_size) {
        CU_ASSERT_PTR_NULL(plan);
        if (plan != NULL)
            guac_display_plan_free(plan);
        guac_mem_free(display);
        return;
    }

    CU_ASSERT_PTR_NOT_NULL_FATAL(plan);
    size_t pixels_sent = 0;
    for (size_t i = 0; i < plan->length; i++) {
        guac_rect* rect = &plan->ops[i].dest;
        CU_ASSERT_EQUAL(plan->ops[i].type, GUAC_DISPLAY_PLAN_OPERATION_IMG);
        CU_ASSERT(rect->left >= 0 && rect->top >= 0);
        CU_ASSERT(rect->right <= new_size && rect->bottom <= new_size);
        pixels_sent += guac_rect_width(rect) * guac_rect_height(rect);
    }

    CU_ASSERT_EQUAL(pixels_sent, new_size * new_size);
    guac_display_plan_free(plan);
    guac_mem_free(display);

}

/**
 * Test which verifies that growing or shrinking a layer repaints all pixels,
 * even when their values are unchanged.
 */
void test_display__resize_repaints_unchanged_pixels(void) {

    check_resize(64, 128, 0);
    check_resize(64, 128, 1);
    check_resize(128, 64, 0);
    check_resize(128, 64, 1);

}

/**
 * Test which verifies that a frame with unchanged dimensions and pixels needs
 * no repaint, even when a dirty region is reported.
 */
void test_display__unchanged_frame_needs_no_repaint(void) {

    check_resize(128, 128, 0);
    check_resize(128, 128, 1);

}
