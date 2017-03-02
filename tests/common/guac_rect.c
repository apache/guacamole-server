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

#include "config.h"

#include "common_suite.h"
#include "common/rect.h"

#include <stdlib.h>
#include <stdio.h>
#include <CUnit/Basic.h>

void test_guac_rect() {

    guac_common_rect max;

    /*
     *  Test init method
     */
    guac_common_rect_init(&max, 0, 0, 100, 100);
    CU_ASSERT_EQUAL(0, max.x);
    CU_ASSERT_EQUAL(0, max.y);
    CU_ASSERT_EQUAL(100, max.width);
    CU_ASSERT_EQUAL(100, max.height);

    /*
     * Test constrain method
     */
    guac_common_rect rect;
    guac_common_rect_init(&rect, -10, -10, 110, 110);
    guac_common_rect_init(&max, 0, 0, 100, 100);
    guac_common_rect_constrain(&rect, &max);
    CU_ASSERT_EQUAL(0, rect.x);
    CU_ASSERT_EQUAL(0, rect.y);
    CU_ASSERT_EQUAL(100, rect.width);
    CU_ASSERT_EQUAL(100, rect.height);

    /*
     * Test extend method
     */
    guac_common_rect_init(&rect, 10, 10, 90, 90);
    guac_common_rect_init(&max, 0, 0, 100, 100);
    guac_common_rect_extend(&rect, &max);
    CU_ASSERT_EQUAL(0, rect.x);
    CU_ASSERT_EQUAL(0, rect.y);
    CU_ASSERT_EQUAL(100, rect.width);
    CU_ASSERT_EQUAL(100, rect.height);

    /*
     * Test adjust method
     */
    int cell_size = 16;

    /* Simple adjustment */
    guac_common_rect_init(&rect, 0, 0, 25, 25);
    guac_common_rect_init(&max, 0, 0, 100, 100);
    guac_common_rect_expand_to_grid(cell_size, &rect, &max);
    CU_ASSERT_EQUAL(0, rect.x);
    CU_ASSERT_EQUAL(0, rect.y);
    CU_ASSERT_EQUAL(32, rect.width);
    CU_ASSERT_EQUAL(32, rect.height);

    /* Adjustment with moving of rect */
    guac_common_rect_init(&rect, 75, 75, 25, 25);
    guac_common_rect_init(&max, 0, 0, 100, 100);
    guac_common_rect_expand_to_grid(cell_size, &rect, &max);
    CU_ASSERT_EQUAL(max.width - 32, rect.x);
    CU_ASSERT_EQUAL(max.height - 32, rect.y);
    CU_ASSERT_EQUAL(32, rect.width);
    CU_ASSERT_EQUAL(32, rect.height);

    guac_common_rect_init(&rect, -5, -5, 25, 25);
    guac_common_rect_init(&max, 0, 0, 100, 100);
    guac_common_rect_expand_to_grid(cell_size, &rect, &max);
    CU_ASSERT_EQUAL(0, rect.x);
    CU_ASSERT_EQUAL(0, rect.y);
    CU_ASSERT_EQUAL(32, rect.width);
    CU_ASSERT_EQUAL(32, rect.height);

    /* Adjustment with moving and clamping of rect */
    guac_common_rect_init(&rect, 0, 0, 25, 15);
    guac_common_rect_init(&max, 0, 5, 32, 15);
    guac_common_rect_expand_to_grid(cell_size, &rect, &max);
    CU_ASSERT_EQUAL(max.x, rect.x);
    CU_ASSERT_EQUAL(max.y, rect.y);
    CU_ASSERT_EQUAL(max.width, rect.width);
    CU_ASSERT_EQUAL(max.height, rect.height);

    /*
     *  Rectangle intersection tests
     */
    guac_common_rect min;
    guac_common_rect_init(&min, 10, 10, 10, 10);

    /* Rectangle intersection - empty
     * rectangle is outside */
    guac_common_rect_init(&rect, 25, 25, 5, 5);
    int res = guac_common_rect_intersects(&rect, &min);
    CU_ASSERT_EQUAL(0, res);

    /* Rectangle intersection - complete
     * rectangle is completely inside */
    guac_common_rect_init(&rect, 11, 11, 5, 5);
    res = guac_common_rect_intersects(&rect, &min);
    CU_ASSERT_EQUAL(2, res);

    /* Rectangle intersection - partial
     * rectangle intersects UL */
    guac_common_rect_init(&rect, 8, 8, 5, 5);
    res = guac_common_rect_intersects(&rect, &min);
    CU_ASSERT_EQUAL(1, res);

    /* Rectangle intersection - partial
     * rectangle intersects LR */
    guac_common_rect_init(&rect, 18, 18, 5, 5);
    res = guac_common_rect_intersects(&rect, &min);
    CU_ASSERT_EQUAL(1, res);

    /* Rectangle intersection - complete
     * rect intersects along UL but inside */
    guac_common_rect_init(&rect, 10, 10, 5, 5);
    res = guac_common_rect_intersects(&rect, &min);
    CU_ASSERT_EQUAL(2, res);

    /* Rectangle intersection - partial
     * rectangle intersects along L but outside */
    guac_common_rect_init(&rect, 5, 10, 5, 5);
    res = guac_common_rect_intersects(&rect, &min);
    CU_ASSERT_EQUAL(1, res);

    /* Rectangle intersection - complete
     * rectangle intersects along LR but rest is inside */
    guac_common_rect_init(&rect, 15, 15, 5, 5);
    res = guac_common_rect_intersects(&rect, &min);
    CU_ASSERT_EQUAL(2, res);

    /* Rectangle intersection - partial
     * rectangle intersects along R but rest is outside */
    guac_common_rect_init(&rect, 20, 10, 5, 5);
    res = guac_common_rect_intersects(&rect, &min);
    CU_ASSERT_EQUAL(1, res);

    /* Rectangle intersection - partial
     * rectangle encloses min; which is a partial intersection */
    guac_common_rect_init(&rect, 5, 5, 20, 20);
    res = guac_common_rect_intersects(&rect, &min);
    CU_ASSERT_EQUAL(1, res);

    /*
     * Basic test of clip and split method
     */
    guac_common_rect_init(&min, 10, 10, 10, 10);
    guac_common_rect cut;

    /* Clip top */
    guac_common_rect_init(&rect, 10, 5, 10, 10);
    res = guac_common_rect_clip_and_split(&rect, &min, &cut);
    CU_ASSERT_EQUAL(1, res);
    CU_ASSERT_EQUAL(10, cut.x);
    CU_ASSERT_EQUAL(5, cut.y);
    CU_ASSERT_EQUAL(10, cut.width);
    CU_ASSERT_EQUAL(5, cut.height);

    CU_ASSERT_EQUAL(10, rect.x);
    CU_ASSERT_EQUAL(10, rect.y);
    CU_ASSERT_EQUAL(10, rect.width);
    CU_ASSERT_EQUAL(5, rect.height);

    /* Clip bottom */
    guac_common_rect_init(&rect, 10, 15, 10, 10);
    res = guac_common_rect_clip_and_split(&rect, &min, &cut);
    CU_ASSERT_EQUAL(1, res);
    CU_ASSERT_EQUAL(10, cut.x);
    CU_ASSERT_EQUAL(20, cut.y);
    CU_ASSERT_EQUAL(10, cut.width);
    CU_ASSERT_EQUAL(5, cut.height);

    CU_ASSERT_EQUAL(10, rect.x);
    CU_ASSERT_EQUAL(15, rect.y);
    CU_ASSERT_EQUAL(10, rect.width);
    CU_ASSERT_EQUAL(5, rect.height);

    /* Clip left */
    guac_common_rect_init(&rect, 5, 10, 10, 10);
    res = guac_common_rect_clip_and_split(&rect, &min, &cut);
    CU_ASSERT_EQUAL(1, res);
    CU_ASSERT_EQUAL(5, cut.x);
    CU_ASSERT_EQUAL(10, cut.y);
    CU_ASSERT_EQUAL(5, cut.width);
    CU_ASSERT_EQUAL(10, cut.height);

    CU_ASSERT_EQUAL(10, rect.x);
    CU_ASSERT_EQUAL(10, rect.y);
    CU_ASSERT_EQUAL(5, rect.width);
    CU_ASSERT_EQUAL(10, rect.height);

    /* Clip right */
    guac_common_rect_init(&rect, 15, 10, 10, 10);
    res = guac_common_rect_clip_and_split(&rect, &min, &cut);
    CU_ASSERT_EQUAL(1, res);
    CU_ASSERT_EQUAL(20, cut.x);
    CU_ASSERT_EQUAL(10, cut.y);
    CU_ASSERT_EQUAL(5, cut.width);
    CU_ASSERT_EQUAL(10, cut.height);

    CU_ASSERT_EQUAL(15, rect.x);
    CU_ASSERT_EQUAL(10, rect.y);
    CU_ASSERT_EQUAL(5, rect.width);
    CU_ASSERT_EQUAL(10, rect.height);

    /*
     * Test a rectangle which completely covers the hole.
     * Clip and split until done.
     */
    guac_common_rect_init(&rect, 5, 5, 20, 20);

    /* Clip top */
    res = guac_common_rect_clip_and_split(&rect, &min, &cut);
    CU_ASSERT_EQUAL(1, res);
    CU_ASSERT_EQUAL(5, cut.x);
    CU_ASSERT_EQUAL(5, cut.y);
    CU_ASSERT_EQUAL(20, cut.width);
    CU_ASSERT_EQUAL(5, cut.height);

    CU_ASSERT_EQUAL(5, rect.x);
    CU_ASSERT_EQUAL(10, rect.y);
    CU_ASSERT_EQUAL(20, rect.width);
    CU_ASSERT_EQUAL(15, rect.height);

    /* Clip left */
    res = guac_common_rect_clip_and_split(&rect, &min, &cut);
    CU_ASSERT_EQUAL(1, res);
    CU_ASSERT_EQUAL(5, cut.x);
    CU_ASSERT_EQUAL(10, cut.y);
    CU_ASSERT_EQUAL(5, cut.width);
    CU_ASSERT_EQUAL(15, cut.height);

    CU_ASSERT_EQUAL(10, rect.x);
    CU_ASSERT_EQUAL(10, rect.y);
    CU_ASSERT_EQUAL(15, rect.width);
    CU_ASSERT_EQUAL(15, rect.height);

    /* Clip bottom */
    res = guac_common_rect_clip_and_split(&rect, &min, &cut);
    CU_ASSERT_EQUAL(1, res);
    CU_ASSERT_EQUAL(10, cut.x);
    CU_ASSERT_EQUAL(20, cut.y);
    CU_ASSERT_EQUAL(15, cut.width);
    CU_ASSERT_EQUAL(5, cut.height);

    CU_ASSERT_EQUAL(10, rect.x);
    CU_ASSERT_EQUAL(10, rect.y);
    CU_ASSERT_EQUAL(15, rect.width);
    CU_ASSERT_EQUAL(10, rect.height);

    /* Clip right */
    res = guac_common_rect_clip_and_split(&rect, &min, &cut);
    CU_ASSERT_EQUAL(20, cut.x);
    CU_ASSERT_EQUAL(10, cut.y);
    CU_ASSERT_EQUAL(5, cut.width);
    CU_ASSERT_EQUAL(10, cut.height);

    CU_ASSERT_EQUAL(10, rect.x);
    CU_ASSERT_EQUAL(10, rect.y);
    CU_ASSERT_EQUAL(10, rect.width);
    CU_ASSERT_EQUAL(10, rect.height);

    /* Make sure nothing is left to do */
    res = guac_common_rect_clip_and_split(&rect, &min, &cut);
    CU_ASSERT_EQUAL(0, res);

}

