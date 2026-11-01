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

/**
 * Marker identifying a private mouse-mask extension for two-axis wheel input.
 * This nonstandard extension requires a matching client implementation.
 */
#define GUAC_RDP_MOUSE_SMOOTH_WHEEL_MASK UINT32_C(0x10000000)

/**
 * Bit offset of the signed nine-bit horizontal wheel delta in the mouse mask.
 */
#define GUAC_RDP_MOUSE_SMOOTH_WHEEL_X_SHIFT 9

/**
 * Bit offset of the signed nine-bit vertical wheel delta in the mouse mask.
 */
#define GUAC_RDP_MOUSE_SMOOTH_WHEEL_Y_SHIFT 19

/**
 * Bit mask of a nine-bit, two's-complement wheel delta, before shifting it
 * into its axis-specific location in the mouse mask.
 */
#define GUAC_RDP_MOUSE_SMOOTH_WHEEL_AXIS_MASK UINT32_C(0x1FF)

/**
 * Returns whether the given mouse mask contains a private wheel payload.
 *
 * @param mask
 *     The mouse mask to examine.
 *
 * @return
 *     Nonzero if the private extension marker is set, zero otherwise.
 */
int guac_rdp_mouse_mask_is_smooth_wheel(uint32_t mask);

/**
 * Decodes one signed axis from a private wheel mouse-mask payload.
 *
 * @param mask
 *     The mouse mask containing the encoded axis.
 *
 * @param shift
 *     The axis bit offset, either GUAC_RDP_MOUSE_SMOOTH_WHEEL_X_SHIFT or
 *     GUAC_RDP_MOUSE_SMOOTH_WHEEL_Y_SHIFT.
 *
 * @return
 *     The signed nine-bit delta, in the range -256 through 255.
 */
int guac_rdp_decode_smooth_wheel_axis(uint32_t mask, unsigned int shift);

/**
 * Encodes a signed wheel delta for the RDP pointer event rotation field.
 * Negative values must be represented in nine-bit two's complement, not as
 * a negative flag combined with an unsigned magnitude. Deltas are clamped to
 * -127 through 127 to avoid overflowing the sub-notch event range.
 *
 * @param delta
 *     The signed wheel delta.
 *
 * @return
 *     The low nine RDP rotation bits, including the sign bit when negative.
 */
uint16_t guac_rdp_smooth_wheel_rotation(int delta);

#endif
