
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is libguac-client-rdp.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Matt Hortman
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <freerdp/input.h>

#include "rdp_keymap.h"

static guac_rdp_keysym_desc __guac_rdp_keymap_mapping[] = {

    /* space */
    { .keysym = 0x0020, .scancode = 0x39 },

    /* exclam */
    { .keysym = 0x0021, .scancode = 0x02,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* quotedbl */
    { .keysym = 0x0022, .scancode = 0x28,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* numbersign */
    { .keysym = 0x0023, .scancode = 0x04,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* dollar */
    { .keysym = 0x0024, .scancode = 0x05,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* percent */
    { .keysym = 0x0025, .scancode = 0x06,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* ampersand */
    { .keysym = 0x0026, .scancode = 0x08,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* quoteright */
    { .keysym = 0x0027, .scancode = 0x28 },

    /* parenleft */
    { .keysym = 0x0028, .scancode = 0x0A,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* parenright */
    { .keysym = 0x0029, .scancode = 0x0B,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* asterisk */
    { .keysym = 0x002a, .scancode = 0x09,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* plus */
    { .keysym = 0x002b, .scancode = 0x0D,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* comma */
    { .keysym = 0x002c, .scancode = 0x33 },

    /* minus */
    { .keysym = 0x002d, .scancode = 0x0C },

    /* period */
    { .keysym = 0x002e, .scancode = 0x34 },

    /* slash */
    { .keysym = 0x002f, .scancode = 0x35 },

    /* 0 */
    { .keysym = 0x0030, .scancode = 0x0B },

    /* 1 */
    { .keysym = 0x0031, .scancode = 0x02 },

    /* 2 */
    { .keysym = 0x0032, .scancode = 0x03 },

    /* 3 */
    { .keysym = 0x0033, .scancode = 0x04 },

    /* 4 */
    { .keysym = 0x0034, .scancode = 0x05 },

    /* 5 */
    { .keysym = 0x0035, .scancode = 0x06 },

    /* 6 */
    { .keysym = 0x0036, .scancode = 0x07 },

    /* 7 */
    { .keysym = 0x0037, .scancode = 0x08 },

    /* 8 */
    { .keysym = 0x0038, .scancode = 0x09 },

    /* 9 */
    { .keysym = 0x0039, .scancode = 0x0A },

    /* colon */
    { .keysym = 0x003a, .scancode = 0x27,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* semicolon */
    { .keysym = 0x003b, .scancode = 0x27 },

    /* less */
    { .keysym = 0x003c, .scancode = 0x33,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* equal */
    { .keysym = 0x003d, .scancode = 0x0D },

    /* greater */
    { .keysym = 0x003e, .scancode = 0x34,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* question */
    { .keysym = 0x003f, .scancode = 0x35,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* at */
    { .keysym = 0x0040, .scancode = 0x03,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* A */
    { .keysym = 0x0041, .scancode = 0x1E,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* B */
    { .keysym = 0x0042, .scancode = 0x30,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* C */
    { .keysym = 0x0043, .scancode = 0x2E,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* D */
    { .keysym = 0x0044, .scancode = 0x20,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* E */
    { .keysym = 0x0045, .scancode = 0x12,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* F */
    { .keysym = 0x0046, .scancode = 0x21,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* G */
    { .keysym = 0x0047, .scancode = 0x22,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* H */
    { .keysym = 0x0048, .scancode = 0x23,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* I */
    { .keysym = 0x0049, .scancode = 0x17,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* J */
    { .keysym = 0x004a, .scancode = 0x24,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* K */
    { .keysym = 0x004b, .scancode = 0x25,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* L */
    { .keysym = 0x004c, .scancode = 0x26,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* M */
    { .keysym = 0x004d, .scancode = 0x32,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* N */
    { .keysym = 0x004e, .scancode = 0x31,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* O */
    { .keysym = 0x004f, .scancode = 0x18,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* P */
    { .keysym = 0x0050, .scancode = 0x19,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* Q */
    { .keysym = 0x0051, .scancode = 0x10,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* R */
    { .keysym = 0x0052, .scancode = 0x13,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* S */
    { .keysym = 0x0053, .scancode = 0x1F,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* T */
    { .keysym = 0x0054, .scancode = 0x14,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* U */
    { .keysym = 0x0055, .scancode = 0x16,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* V */
    { .keysym = 0x0056, .scancode = 0x2F,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* W */
    { .keysym = 0x0057, .scancode = 0x11,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* X */
    { .keysym = 0x0058, .scancode = 0x2D,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* Y */
    { .keysym = 0x0059, .scancode = 0x15,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* Z */
    { .keysym = 0x005a, .scancode = 0x2C,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* bracketleft */
    { .keysym = 0x005b, .scancode = 0x1A },

    /* backslash */
    { .keysym = 0x005c, .scancode = 0x2B },

    /* bracketright */
    { .keysym = 0x005d, .scancode = 0x1B },

    /* asciicircum */
    { .keysym = 0x005e, .scancode = 0x29,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* underscore */
    { .keysym = 0x005f, .scancode = 0x0C,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* quoteleft */
    { .keysym = 0x0060, .scancode = 0x29 },

    /* a */
    { .keysym = 0x0061, .scancode = 0x1E },

    /* b */
    { .keysym = 0x0062, .scancode = 0x30 },

    /* c */
    { .keysym = 0x0063, .scancode = 0x2E },

    /* d */
    { .keysym = 0x0064, .scancode = 0x20 },

    /* e */
    { .keysym = 0x0065, .scancode = 0x12 },

    /* f */
    { .keysym = 0x0066, .scancode = 0x21 },

    /* g */
    { .keysym = 0x0067, .scancode = 0x22 },

    /* h */
    { .keysym = 0x0068, .scancode = 0x23 },

    /* i */
    { .keysym = 0x0069, .scancode = 0x17 },

    /* j */
    { .keysym = 0x006a, .scancode = 0x24 },

    /* k */
    { .keysym = 0x006b, .scancode = 0x25 },

    /* l */
    { .keysym = 0x006c, .scancode = 0x26 },

    /* m */
    { .keysym = 0x006d, .scancode = 0x32 },

    /* n */
    { .keysym = 0x006e, .scancode = 0x31 },

    /* o */
    { .keysym = 0x006f, .scancode = 0x18 },

    /* p */
    { .keysym = 0x0070, .scancode = 0x19 },

    /* q */
    { .keysym = 0x0071, .scancode = 0x10 },

    /* r */
    { .keysym = 0x0072, .scancode = 0x13 },

    /* s */
    { .keysym = 0x0073, .scancode = 0x1F },

    /* t */
    { .keysym = 0x0074, .scancode = 0x14 },

    /* u */
    { .keysym = 0x0075, .scancode = 0x16 },

    /* v */
    { .keysym = 0x0076, .scancode = 0x2F },

    /* w */
    { .keysym = 0x0077, .scancode = 0x11 },

    /* x */
    { .keysym = 0x0078, .scancode = 0x2D },

    /* y */
    { .keysym = 0x0079, .scancode = 0x15 },

    /* z */
    { .keysym = 0x007a, .scancode = 0x2C },

    /* braceleft */
    { .keysym = 0x007b, .scancode = 0x1A,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* bar */
    { .keysym = 0x007c, .scancode = 0x2B,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* braceright */
    { .keysym = 0x007d, .scancode = 0x1B,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* asciitilde */
    { .keysym = 0x007e, .scancode = 0x29,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* BackSpace */
    { .keysym = 0xff08, .scancode = 0x0E },

    /* Tab */
    { .keysym = 0xff09, .scancode = 0x0F },

    /* Return */
    { .keysym = 0xff0d, .scancode = 0x1C },

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

    /* Menu */
    { .keysym = 0xff67, .scancode = 0x5D,
        .flags = KBD_FLAGS_EXTENDED },

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

    /* Shift_L */
    { .keysym = 0xffe1, .scancode = 0x2A },

    /* Shift_R */
    { .keysym = 0xffe2, .scancode = 0x36 },

    /* Control_L */
    { .keysym = 0xffe3, .scancode = 0x1D },

    /* Control_R */
    { .keysym = 0xffe4, .scancode = 0x1D },

    /* Alt_L */
    { .keysym = 0xffe9, .scancode = 0x38 },

    /* Alt_R */
    { .keysym = 0xffea, .scancode = 0x38 },

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

const guac_rdp_keymap guac_rdp_keymap_en_us = {

    .name = "en-us-qwerty",

    .parent = NULL,
    .mapping = __guac_rdp_keymap_mapping

};

