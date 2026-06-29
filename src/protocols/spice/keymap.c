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

#include "keymap.h"

#include <stddef.h>

/**
 * Bit which, when set within a scancode value, indicates that the scancode is
 * an "extended" scancode, transmitted on the wire with a leading 0xE0 byte.
 * This matches the encoding expected by spice-gtk's
 * spice_inputs_channel_key_press()/key_release() functions.
 */
#define GUAC_SPICE_SCANCODE_EXTENDED 0x100

/**
 * A single mapping from an X11 keysym to its corresponding PC (AT set 1)
 * scancode.
 */
typedef struct guac_spice_keysym_mapping {

    /**
     * The X11 keysym being mapped.
     */
    int keysym;

    /**
     * The corresponding PC scancode, with GUAC_SPICE_SCANCODE_EXTENDED set if
     * the scancode is an extended scancode.
     */
    unsigned int scancode;

} guac_spice_keysym_mapping;

/**
 * Static mapping of X11 keysyms to PC scancodes for the US keyboard layout.
 * Guacamole transmits the keysym of the resulting character along with
 * separate press/release events for modifier keys (Shift, Control, etc.), so
 * both the unshifted and shifted keysyms produced by a given physical key map
 * to that key's scancode.
 */
static const guac_spice_keysym_mapping guac_spice_keysym_scancodes[] = {

    /* Number row (digits and their shifted symbols) */
    { 0x0031, 0x02 }, { 0x0021, 0x02 }, /* 1 ! */
    { 0x0032, 0x03 }, { 0x0040, 0x03 }, /* 2 @ */
    { 0x0033, 0x04 }, { 0x0023, 0x04 }, /* 3 # */
    { 0x0034, 0x05 }, { 0x0024, 0x05 }, /* 4 $ */
    { 0x0035, 0x06 }, { 0x0025, 0x06 }, /* 5 % */
    { 0x0036, 0x07 }, { 0x005e, 0x07 }, /* 6 ^ */
    { 0x0037, 0x08 }, { 0x0026, 0x08 }, /* 7 & */
    { 0x0038, 0x09 }, { 0x002a, 0x09 }, /* 8 * */
    { 0x0039, 0x0a }, { 0x0028, 0x0a }, /* 9 ( */
    { 0x0030, 0x0b }, { 0x0029, 0x0b }, /* 0 ) */
    { 0x002d, 0x0c }, { 0x005f, 0x0c }, /* - _ */
    { 0x003d, 0x0d }, { 0x002b, 0x0d }, /* = + */

    /* QWERTY row */
    { 0x0071, 0x10 }, { 0x0051, 0x10 }, /* q Q */
    { 0x0077, 0x11 }, { 0x0057, 0x11 }, /* w W */
    { 0x0065, 0x12 }, { 0x0045, 0x12 }, /* e E */
    { 0x0072, 0x13 }, { 0x0052, 0x13 }, /* r R */
    { 0x0074, 0x14 }, { 0x0054, 0x14 }, /* t T */
    { 0x0079, 0x15 }, { 0x0059, 0x15 }, /* y Y */
    { 0x0075, 0x16 }, { 0x0055, 0x16 }, /* u U */
    { 0x0069, 0x17 }, { 0x0049, 0x17 }, /* i I */
    { 0x006f, 0x18 }, { 0x004f, 0x18 }, /* o O */
    { 0x0070, 0x19 }, { 0x0050, 0x19 }, /* p P */
    { 0x005b, 0x1a }, { 0x007b, 0x1a }, /* [ { */
    { 0x005d, 0x1b }, { 0x007d, 0x1b }, /* ] } */

    /* ASDF row */
    { 0x0061, 0x1e }, { 0x0041, 0x1e }, /* a A */
    { 0x0073, 0x1f }, { 0x0053, 0x1f }, /* s S */
    { 0x0064, 0x20 }, { 0x0044, 0x20 }, /* d D */
    { 0x0066, 0x21 }, { 0x0046, 0x21 }, /* f F */
    { 0x0067, 0x22 }, { 0x0047, 0x22 }, /* g G */
    { 0x0068, 0x23 }, { 0x0048, 0x23 }, /* h H */
    { 0x006a, 0x24 }, { 0x004a, 0x24 }, /* j J */
    { 0x006b, 0x25 }, { 0x004b, 0x25 }, /* k K */
    { 0x006c, 0x26 }, { 0x004c, 0x26 }, /* l L */
    { 0x003b, 0x27 }, { 0x003a, 0x27 }, /* ; : */
    { 0x0027, 0x28 }, { 0x0022, 0x28 }, /* ' " */
    { 0x0060, 0x29 }, { 0x007e, 0x29 }, /* ` ~ */
    { 0x005c, 0x2b }, { 0x007c, 0x2b }, /* \ | */

    /* ZXCV row */
    { 0x007a, 0x2c }, { 0x005a, 0x2c }, /* z Z */
    { 0x0078, 0x2d }, { 0x0058, 0x2d }, /* x X */
    { 0x0063, 0x2e }, { 0x0043, 0x2e }, /* c C */
    { 0x0076, 0x2f }, { 0x0056, 0x2f }, /* v V */
    { 0x0062, 0x30 }, { 0x0042, 0x30 }, /* b B */
    { 0x006e, 0x31 }, { 0x004e, 0x31 }, /* n N */
    { 0x006d, 0x32 }, { 0x004d, 0x32 }, /* m M */
    { 0x002c, 0x33 }, { 0x003c, 0x33 }, /* , < */
    { 0x002e, 0x34 }, { 0x003e, 0x34 }, /* . > */
    { 0x002f, 0x35 }, { 0x003f, 0x35 }, /* / ? */

    { 0x0020, 0x39 },                   /* Space */

    /* Editing / control keys */
    { 0xff1b, 0x01 },                   /* Escape */
    { 0xff08, 0x0e },                   /* BackSpace */
    { 0xff09, 0x0f },                   /* Tab */
    { 0xff0d, 0x1c },                   /* Return */
    { 0xffe5, 0x3a },                   /* Caps_Lock */
    { 0xff7f, 0x45 },                   /* Num_Lock */
    { 0xff14, 0x46 },                   /* Scroll_Lock */

    /* Modifier keys */
    { 0xffe1, 0x2a },                   /* Shift_L */
    { 0xffe2, 0x36 },                   /* Shift_R */
    { 0xffe3, 0x1d },                   /* Control_L */
    { 0xffe4, 0x1d | GUAC_SPICE_SCANCODE_EXTENDED }, /* Control_R */
    { 0xffe9, 0x38 },                   /* Alt_L */
    { 0xffea, 0x38 | GUAC_SPICE_SCANCODE_EXTENDED }, /* Alt_R */
    { 0xffe7, 0x38 },                   /* Meta_L (treat as Alt) */
    { 0xffe8, 0x38 | GUAC_SPICE_SCANCODE_EXTENDED }, /* Meta_R */
    { 0xffeb, 0x5b | GUAC_SPICE_SCANCODE_EXTENDED }, /* Super_L (left "Windows") */
    { 0xffec, 0x5c | GUAC_SPICE_SCANCODE_EXTENDED }, /* Super_R (right "Windows") */
    { 0xff67, 0x5d | GUAC_SPICE_SCANCODE_EXTENDED }, /* Menu */

    /* Function keys */
    { 0xffbe, 0x3b },                   /* F1 */
    { 0xffbf, 0x3c },                   /* F2 */
    { 0xffc0, 0x3d },                   /* F3 */
    { 0xffc1, 0x3e },                   /* F4 */
    { 0xffc2, 0x3f },                   /* F5 */
    { 0xffc3, 0x40 },                   /* F6 */
    { 0xffc4, 0x41 },                   /* F7 */
    { 0xffc5, 0x42 },                   /* F8 */
    { 0xffc6, 0x43 },                   /* F9 */
    { 0xffc7, 0x44 },                   /* F10 */
    { 0xffc8, 0x57 },                   /* F11 */
    { 0xffc9, 0x58 },                   /* F12 */

    /* Navigation cluster (extended) */
    { 0xff63, 0x52 | GUAC_SPICE_SCANCODE_EXTENDED }, /* Insert */
    { 0xffff, 0x53 | GUAC_SPICE_SCANCODE_EXTENDED }, /* Delete */
    { 0xff50, 0x47 | GUAC_SPICE_SCANCODE_EXTENDED }, /* Home */
    { 0xff57, 0x4f | GUAC_SPICE_SCANCODE_EXTENDED }, /* End */
    { 0xff55, 0x49 | GUAC_SPICE_SCANCODE_EXTENDED }, /* Page_Up (Prior) */
    { 0xff56, 0x51 | GUAC_SPICE_SCANCODE_EXTENDED }, /* Page_Down (Next) */
    { 0xff52, 0x48 | GUAC_SPICE_SCANCODE_EXTENDED }, /* Up */
    { 0xff54, 0x50 | GUAC_SPICE_SCANCODE_EXTENDED }, /* Down */
    { 0xff51, 0x4b | GUAC_SPICE_SCANCODE_EXTENDED }, /* Left */
    { 0xff53, 0x4d | GUAC_SPICE_SCANCODE_EXTENDED }, /* Right */

    { 0xff61, 0x37 | GUAC_SPICE_SCANCODE_EXTENDED }, /* Print */

    /* Keypad */
    { 0xff8d, 0x1c | GUAC_SPICE_SCANCODE_EXTENDED }, /* KP_Enter */
    { 0xffaf, 0x35 | GUAC_SPICE_SCANCODE_EXTENDED }, /* KP_Divide */
    { 0xffaa, 0x37 },                   /* KP_Multiply */
    { 0xffad, 0x4a },                   /* KP_Subtract */
    { 0xffab, 0x4e },                   /* KP_Add */
    { 0xffae, 0x53 },                   /* KP_Decimal */
    { 0xffb0, 0x52 },                   /* KP_0 */
    { 0xffb1, 0x4f },                   /* KP_1 */
    { 0xffb2, 0x50 },                   /* KP_2 */
    { 0xffb3, 0x51 },                   /* KP_3 */
    { 0xffb4, 0x4b },                   /* KP_4 */
    { 0xffb5, 0x4c },                   /* KP_5 */
    { 0xffb6, 0x4d },                   /* KP_6 */
    { 0xffb7, 0x47 },                   /* KP_7 */
    { 0xffb8, 0x48 },                   /* KP_8 */
    { 0xffb9, 0x49 },                   /* KP_9 */

    /* End of table */
    { 0, 0 }

};

unsigned int guac_spice_keysym_to_scancode(int keysym) {

    /* Search the static mapping table for the given keysym */
    const guac_spice_keysym_mapping* current = guac_spice_keysym_scancodes;
    while (current->keysym != 0) {

        if (current->keysym == keysym)
            return current->scancode;

        current++;

    }

    /* No known mapping */
    return 0;

}
