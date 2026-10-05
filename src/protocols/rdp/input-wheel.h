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

#ifndef GUAC_RDP_INPUT_WHEEL_H
#define GUAC_RDP_INPUT_WHEEL_H

#include <stdint.h>

/* Private Guac mouse-mask extension. Bits 0-4 remain clear, preserving the
 * standard button and wheel mask even when an older server ignores this
 * extension. Each axis is signed 9-bit two's complement. */
#define GUAC_RDP_MOUSE_SMOOTH_WHEEL_MASK UINT32_C(0x10000000)
#define GUAC_RDP_MOUSE_SMOOTH_WHEEL_X_SHIFT 9
#define GUAC_RDP_MOUSE_SMOOTH_WHEEL_Y_SHIFT 19
#define GUAC_RDP_MOUSE_SMOOTH_WHEEL_AXIS_MASK UINT32_C(0x1FF)

static inline int guac_rdp_mouse_mask_is_smooth_wheel(uint32_t mask) {
    return (mask & GUAC_RDP_MOUSE_SMOOTH_WHEEL_MASK) != 0;
}

static inline int guac_rdp_decode_smooth_wheel_axis(uint32_t mask,
        unsigned int shift) {

    int value = (mask >> shift) & GUAC_RDP_MOUSE_SMOOTH_WHEEL_AXIS_MASK;
    return value & 0x100 ? value - 0x200 : value;

}

static inline uint32_t guac_rdp_encode_smooth_wheel(int dx, int dy) {
    return GUAC_RDP_MOUSE_SMOOTH_WHEEL_MASK
            | (((uint32_t) dx & GUAC_RDP_MOUSE_SMOOTH_WHEEL_AXIS_MASK)
                    << GUAC_RDP_MOUSE_SMOOTH_WHEEL_X_SHIFT)
            | (((uint32_t) dy & GUAC_RDP_MOUSE_SMOOTH_WHEEL_AXIS_MASK)
                    << GUAC_RDP_MOUSE_SMOOTH_WHEEL_Y_SHIFT);
}

/* RDP wheel events represent direction separately from their unsigned
 * rotation magnitude. Keep each event within the sub-notch range emitted by
 * the frontend, even if a malformed client sends a larger signed delta. */
static inline int guac_rdp_smooth_wheel_magnitude(int delta) {
    if (delta < 0)
        delta = -delta;
    return delta > 127 ? 127 : delta;
}

#endif
