/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#include <freerdp/input.h>

#include "rdp_keymap.h"

static guac_rdp_keysym_desc __guac_rdp_keymap_mapping[] = {

    /* BackSpace */
    { .keysym = 0xff08, .scancode = 0x0E },

    /* Tab */
    { .keysym = 0xff09, .scancode = 0x0F },

    /* Return */
    { .keysym = 0xff0d, .scancode = 0x1C },

    /* Scroll_Lock */
    { .keysym = 0xff14, .scancode = 0x46 },

    /* Escape */
    { .keysym = 0xff1b, .scancode = 0x01 },

    /* Home */
    { .keysym = 0xff50, .scancode = 0x47,
        .flags = KBD_FLAGS_EXTENDED },

    /* Left */
    { .keysym = 0xff51, .scancode = 0x4B,
        .flags = KBD_FLAGS_EXTENDED },

    /* Up */
    { .keysym = 0xff52, .scancode = 0x48,
        .flags = KBD_FLAGS_EXTENDED },

    /* Right */
    { .keysym = 0xff53, .scancode = 0x4D,
        .flags = KBD_FLAGS_EXTENDED },

    /* Down */
    { .keysym = 0xff54, .scancode = 0x50,
        .flags = KBD_FLAGS_EXTENDED },

    /* Page_Up */
    { .keysym = 0xff55, .scancode = 0x49,
        .flags = KBD_FLAGS_EXTENDED },

    /* Menu */
    { .keysym = 0xff67, .scancode = 0x5D,
        .flags = KBD_FLAGS_EXTENDED },

    /* Page_Down */
    { .keysym = 0xff56, .scancode = 0x51,
        .flags = KBD_FLAGS_EXTENDED },

    /* End */
    { .keysym = 0xff57, .scancode = 0x4F,
        .flags = KBD_FLAGS_EXTENDED },

    /* Insert */
    { .keysym = 0xff63, .scancode = 0x52,
        .flags = KBD_FLAGS_EXTENDED },

    /* Num_Lock */
    { .keysym = 0xff7f, .scancode = 0x45 },

    /* KP_0 */
    { .keysym = 0xffb0, .scancode = 0x52 },

    /* KP_1 */
    { .keysym = 0xffb1, .scancode = 0x4F },

    /* KP_2 */
    { .keysym = 0xffb2, .scancode = 0x50 },

    /* KP_3 */
    { .keysym = 0xffb3, .scancode = 0x51 },

    /* KP_4 */
    { .keysym = 0xffb4, .scancode = 0x4B },

    /* KP_5 */
    { .keysym = 0xffb5, .scancode = 0x4C },

    /* KP_6 */
    { .keysym = 0xffb6, .scancode = 0x4D },

    /* KP_7 */
    { .keysym = 0xffb7, .scancode = 0x47 },

    /* KP_8 */
    { .keysym = 0xffb8, .scancode = 0x48 },

    /* KP_9 */
    { .keysym = 0xffb9, .scancode = 0x49 },

    /* F1 */
    { .keysym = 0xffbe, .scancode = 0x3B },

    /* F2 */
    { .keysym = 0xffbf, .scancode = 0x3C },

    /* F3 */
    { .keysym = 0xffc0, .scancode = 0x3D },

    /* F4 */
    { .keysym = 0xffc1, .scancode = 0x3E },

    /* F5 */
    { .keysym = 0xffc2, .scancode = 0x3F },

    /* F6 */
    { .keysym = 0xffc3, .scancode = 0x40 },

    /* F7 */
    { .keysym = 0xffc4, .scancode = 0x41 },

    /* F8 */
    { .keysym = 0xffc5, .scancode = 0x42 },

    /* F9 */
    { .keysym = 0xffc6, .scancode = 0x43 },

    /* F10 */
    { .keysym = 0xffc7, .scancode = 0x44 },

    /* F11 */
    { .keysym = 0xffc8, .scancode = 0x57 },

    /* F12 */
    { .keysym = 0xffc9, .scancode = 0x58 },

    /* Shift_L */
    { .keysym = 0xffe1, .scancode = 0x2A },

    /* Shift_R */
    { .keysym = 0xffe2, .scancode = 0x36 },

    /* Control_L */
    { .keysym = 0xffe3, .scancode = 0x1D },

    /* Control_R */
    { .keysym = 0xffe4, .scancode = 0x9D },

    /* Caps_Lock */
    { .keysym = 0xffe5, .scancode = 0x3A,
        .flags = KBD_FLAGS_EXTENDED },

    /* Alt_L */
    { .keysym = 0xffe9, .scancode = 0x38 },

    /* Alt_R */
    { .keysym = 0xffea, .scancode = 0x38,
        .flags = KBD_FLAGS_EXTENDED },

    /* ISO_Level3_Shift */
    { .keysym = 0xfe03, .scancode = 0x38,
        .flags = KBD_FLAGS_EXTENDED },

    /* Super_L */
    { .keysym = 0xffeb, .scancode = 0x5B,
        .flags = KBD_FLAGS_EXTENDED },

    /* Super_R */
    { .keysym = 0xffec, .scancode = 0x5C,
        .flags = KBD_FLAGS_EXTENDED },

    /* Delete */
    { .keysym = 0xffff, .scancode = 0x53,
        .flags = KBD_FLAGS_EXTENDED },

    {0}

};

const guac_rdp_keymap guac_rdp_keymap_base = {

    .name = "base",

    .parent = NULL,
    .mapping = __guac_rdp_keymap_mapping

};

