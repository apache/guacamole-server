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

#include "input-wheel.h"

/**
 * Clamps a signed delta to the supported sub-notch rotation range.
 *
 * @param delta
 *     The wheel delta to clamp.
 *
 * @return
 *     The delta, limited to the inclusive range -127 through 127.
 */
static int guac_rdp_clamp_smooth_wheel_delta(int delta) {

    if (delta < -127)
        return -127;
    if (delta > 127)
        return 127;

    return delta;

}

int guac_rdp_mouse_mask_is_smooth_wheel(uint32_t mask) {
    return (mask & GUAC_RDP_MOUSE_SMOOTH_WHEEL_MASK) != 0;
}

int guac_rdp_decode_smooth_wheel_axis(uint32_t mask, unsigned int shift) {

    uint32_t value = (mask >> shift) & GUAC_RDP_MOUSE_SMOOTH_WHEEL_AXIS_MASK;
    return (value & UINT32_C(0x100)) ? (int) value - 0x200 : (int) value;

}

uint32_t guac_rdp_encode_smooth_wheel(int dx, int dy) {

    uint32_t x = (uint32_t) guac_rdp_clamp_smooth_wheel_delta(dx);
    uint32_t y = (uint32_t) guac_rdp_clamp_smooth_wheel_delta(dy);

    return GUAC_RDP_MOUSE_SMOOTH_WHEEL_MASK
            | ((x & GUAC_RDP_MOUSE_SMOOTH_WHEEL_AXIS_MASK)
                    << GUAC_RDP_MOUSE_SMOOTH_WHEEL_X_SHIFT)
            | ((y & GUAC_RDP_MOUSE_SMOOTH_WHEEL_AXIS_MASK)
                    << GUAC_RDP_MOUSE_SMOOTH_WHEEL_Y_SHIFT);

}

uint16_t guac_rdp_smooth_wheel_rotation(int delta) {

    uint32_t clamped = (uint32_t) guac_rdp_clamp_smooth_wheel_delta(delta);
    return (uint16_t) (clamped & UINT32_C(0x1FF));

}
