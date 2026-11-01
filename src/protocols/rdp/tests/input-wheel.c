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
#include <limits.h>

/**
 * Constructs a private wheel payload for decoder tests.
 *
 * @param dx
 *     Signed horizontal wheel delta to encode in nine-bit two's complement.
 *
 * @param dy
 *     Signed vertical wheel delta to encode in nine-bit two's complement.
 *
 * @return
 *     A test mouse mask containing the marker and both axis payloads.
 */
static uint32_t test_encode_smooth_wheel(int dx, int dy) {

    uint32_t x = (uint32_t) dx & GUAC_RDP_MOUSE_SMOOTH_WHEEL_AXIS_MASK;
    uint32_t y = (uint32_t) dy & GUAC_RDP_MOUSE_SMOOTH_WHEEL_AXIS_MASK;

    return GUAC_RDP_MOUSE_SMOOTH_WHEEL_MASK
            | (x << GUAC_RDP_MOUSE_SMOOTH_WHEEL_X_SHIFT)
            | (y << GUAC_RDP_MOUSE_SMOOTH_WHEEL_Y_SHIFT);

}

/**
 * Verifies that supported signed horizontal and vertical wheel deltas survive
 * encoding into, and decoding from, the private mouse-mask extension.
 */
void test_input_wheel__signed_sub_notch_round_trip(void) {

    const int deltas[] = { -127, -1, 0, 1, 127 };

    for (unsigned int i = 0; i < sizeof(deltas) / sizeof(deltas[0]); i++) {

        int dx = deltas[i];
        int dy = deltas[sizeof(deltas) / sizeof(deltas[0]) - i - 1];
        uint32_t mask = test_encode_smooth_wheel(dx, dy);

        CU_ASSERT_TRUE(guac_rdp_mouse_mask_is_smooth_wheel(mask));
        CU_ASSERT_EQUAL(guac_rdp_decode_smooth_wheel_axis(mask,
                    GUAC_RDP_MOUSE_SMOOTH_WHEEL_X_SHIFT), dx);
        CU_ASSERT_EQUAL(guac_rdp_decode_smooth_wheel_axis(mask,
                    GUAC_RDP_MOUSE_SMOOTH_WHEEL_Y_SHIFT), dy);

    }

}

/**
 * Verifies that extension payloads never set ordinary Guacamole mouse-button
 * or wheel-event bits in the low five positions of the mouse mask.
 */
void test_input_wheel__payload_never_sets_standard_mouse_buttons(void) {

    uint32_t mask = test_encode_smooth_wheel(-127, 127);

    CU_ASSERT_EQUAL(mask & UINT32_C(0x1F), 0);
    CU_ASSERT_EQUAL(mask & GUAC_RDP_MOUSE_SMOOTH_WHEEL_MASK,
            GUAC_RDP_MOUSE_SMOOTH_WHEEL_MASK);

}

/**
 * Verifies that standard mouse-button and wheel-event masks never trigger
 * the private wheel-extension path.
 */
void test_input_wheel__ordinary_mouse_mask_is_not_extension(void) {

    for (uint32_t mask = 0; mask <= UINT32_C(0x1F); mask++)
        CU_ASSERT_FALSE(guac_rdp_mouse_mask_is_smooth_wheel(mask));

}

/**
 * Verifies that out-of-range deltas are clamped without wrapping across the
 * nine-bit signed payload, including the minimum and maximum C int values.
 */
void test_input_wheel__out_of_range_deltas_are_clamped(void) {

    uint32_t mask = test_encode_smooth_wheel(-256, 255);

    CU_ASSERT_EQUAL(guac_rdp_decode_smooth_wheel_axis(mask,
                GUAC_RDP_MOUSE_SMOOTH_WHEEL_X_SHIFT), -256);
    CU_ASSERT_EQUAL(guac_rdp_decode_smooth_wheel_axis(mask,
                GUAC_RDP_MOUSE_SMOOTH_WHEEL_Y_SHIFT), 255);
    CU_ASSERT_EQUAL(guac_rdp_smooth_wheel_rotation(-256), UINT16_C(0x181));
    CU_ASSERT_EQUAL(guac_rdp_smooth_wheel_rotation(255), UINT16_C(0x07F));

    mask = test_encode_smooth_wheel(-128, 128);
    CU_ASSERT_EQUAL(guac_rdp_decode_smooth_wheel_axis(mask,
                GUAC_RDP_MOUSE_SMOOTH_WHEEL_X_SHIFT), -128);
    CU_ASSERT_EQUAL(guac_rdp_decode_smooth_wheel_axis(mask,
                GUAC_RDP_MOUSE_SMOOTH_WHEEL_Y_SHIFT), 128);
    CU_ASSERT_EQUAL(guac_rdp_smooth_wheel_rotation(-128), UINT16_C(0x181));
    CU_ASSERT_EQUAL(guac_rdp_smooth_wheel_rotation(128), UINT16_C(0x07F));

}

/**
 * Verifies that RDP rotation uses the signed nine-bit two's-complement field,
 * including its 0x100 sign bit, rather than sign-plus-absolute-magnitude.
 */
void test_input_wheel__rdp_rotation_uses_signed_nine_bit_encoding(void) {

    CU_ASSERT_EQUAL(guac_rdp_smooth_wheel_rotation(-127), UINT16_C(0x181));
    CU_ASSERT_EQUAL(guac_rdp_smooth_wheel_rotation(-1), UINT16_C(0x1FF));
    CU_ASSERT_EQUAL(guac_rdp_smooth_wheel_rotation(0), UINT16_C(0x000));
    CU_ASSERT_EQUAL(guac_rdp_smooth_wheel_rotation(1), UINT16_C(0x001));
    CU_ASSERT_EQUAL(guac_rdp_smooth_wheel_rotation(127), UINT16_C(0x07F));

    CU_ASSERT_EQUAL(guac_rdp_smooth_wheel_rotation(-128), UINT16_C(0x181));
    CU_ASSERT_EQUAL(guac_rdp_smooth_wheel_rotation(255), UINT16_C(0x07F));
    CU_ASSERT_EQUAL(guac_rdp_smooth_wheel_rotation(INT_MIN), UINT16_C(0x181));
    CU_ASSERT_EQUAL(guac_rdp_smooth_wheel_rotation(INT_MAX), UINT16_C(0x07F));

}

/**
 * Verifies that every permitted RDP wheel delta can be recovered by sign
 * extending the low nine rotation bits, as specified for TS_POINTER_EVENT.
 */
void test_input_wheel__rdp_rotation_sign_extension_round_trip(void) {

    for (int delta = -127; delta <= 127; delta++) {

        uint16_t rotation = guac_rdp_smooth_wheel_rotation(delta);
        int decoded = (rotation & UINT16_C(0x100))
                ? (int) rotation - 0x200 : (int) rotation;

        CU_ASSERT_EQUAL(decoded, delta);

    }

}
