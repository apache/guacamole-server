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

#include "../input-wheel.h"

#include <CUnit/CUnit.h>

void test_input_wheel__signed_sub_notch_round_trip(void) {

    const int deltas[] = { -127, -1, 0, 1, 127 };

    for (unsigned int i = 0; i < sizeof(deltas) / sizeof(deltas[0]); i++) {

        int dx = deltas[i];
        int dy = deltas[sizeof(deltas) / sizeof(deltas[0]) - i - 1];
        uint32_t mask = guac_rdp_encode_smooth_wheel(dx, dy);

        CU_ASSERT_TRUE(guac_rdp_mouse_mask_is_smooth_wheel(mask));
        CU_ASSERT_EQUAL(guac_rdp_decode_smooth_wheel_axis(mask,
                    GUAC_RDP_MOUSE_SMOOTH_WHEEL_X_SHIFT), dx);
        CU_ASSERT_EQUAL(guac_rdp_decode_smooth_wheel_axis(mask,
                    GUAC_RDP_MOUSE_SMOOTH_WHEEL_Y_SHIFT), dy);

    }

}

void test_input_wheel__payload_never_sets_standard_mouse_buttons(void) {

    uint32_t mask = guac_rdp_encode_smooth_wheel(-127, 127);

    CU_ASSERT_EQUAL(mask & UINT32_C(0x1F), 0);
    CU_ASSERT_EQUAL(mask & GUAC_RDP_MOUSE_SMOOTH_WHEEL_MASK,
            GUAC_RDP_MOUSE_SMOOTH_WHEEL_MASK);

}

void test_input_wheel__ordinary_mouse_mask_is_not_extension(void) {

    CU_ASSERT_FALSE(guac_rdp_mouse_mask_is_smooth_wheel(0x1F));
    CU_ASSERT_FALSE(guac_rdp_mouse_mask_is_smooth_wheel(0));

}

void test_input_wheel__rdp_rotation_uses_unsigned_magnitude(void) {

    CU_ASSERT_EQUAL(guac_rdp_smooth_wheel_magnitude(-127), 127);
    CU_ASSERT_EQUAL(guac_rdp_smooth_wheel_magnitude(-1), 1);
    CU_ASSERT_EQUAL(guac_rdp_smooth_wheel_magnitude(0), 0);
    CU_ASSERT_EQUAL(guac_rdp_smooth_wheel_magnitude(1), 1);
    CU_ASSERT_EQUAL(guac_rdp_smooth_wheel_magnitude(127), 127);

    /* Reject out-of-range magnitudes from malformed custom clients. */
    CU_ASSERT_EQUAL(guac_rdp_smooth_wheel_magnitude(-256), 127);
    CU_ASSERT_EQUAL(guac_rdp_smooth_wheel_magnitude(255), 127);

}
