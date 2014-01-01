
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
 * Alexandre Devely
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

#include "config.h"

#include "rdp_keymap.h"

#include <freerdp/input.h>

#ifdef HAVE_FREERDP_LOCALE_KEYBOARD_H
#include <freerdp/locale/keyboard.h>
#else
#include <freerdp/kbd/layouts.h>
#endif

static guac_rdp_keysym_desc __guac_rdp_keymap_mapping[] = {

    /* space */
    { .keysym = 0x0020, .scancode = 0x39 },

    /* exclam */
    { .keysym = 0x0021, .scancode = 0x35,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* quotedbl */
    { .keysym = 0x0022, .scancode = 0x04,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* numbersign */
    { .keysym = 0x0023, .scancode = 0x04,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT,
        .set_keysyms = GUAC_KEYSYMS_ALTGR },

    /* dollar */
    { .keysym = 0x0024, .scancode = 0x1b,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* percent */
    { .keysym = 0x0025, .scancode = 0x28,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* ampersand */
    { .keysym = 0x0026, .scancode = 0x02,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR  },

    /* quoteright */
    { .keysym = 0x0027, .scancode = 0x05,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* parenleft */
    { .keysym = 0x0028, .scancode = 0x06,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* parenright */
    { .keysym = 0x0029, .scancode = 0x0c,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* asterisk */
    { .keysym = 0x002a, .scancode = 0x2b,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* plus */
    { .keysym = 0x002b, .scancode = 0x0D,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* comma */
    { .keysym = 0x002c, .scancode = 0x32,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* minus */
    { .keysym = 0x002d, .scancode = 0x07,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* period */
    { .keysym = 0x002e, .scancode = 0x33,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT  },

    /* slash */
    { .keysym = 0x002f, .scancode = 0x34,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* 0 */
    { .keysym = 0x0030, .scancode = 0x0B,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* 1 */
    { .keysym = 0x0031, .scancode = 0x02,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* 2 */
    { .keysym = 0x0032, .scancode = 0x03,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* 3 */
    { .keysym = 0x0033, .scancode = 0x04,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* 4 */
    { .keysym = 0x0034, .scancode = 0x05,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* 5 */
    { .keysym = 0x0035, .scancode = 0x06,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* 6 */
    { .keysym = 0x0036, .scancode = 0x07,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* 7 */
    { .keysym = 0x0037, .scancode = 0x08,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* 8 */
    { .keysym = 0x0038, .scancode = 0x09,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* 9 */
    { .keysym = 0x0039, .scancode = 0x0A,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* colon */
    { .keysym = 0x003a, .scancode = 0x34,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* semicolon */
    { .keysym = 0x003b, .scancode = 0x33,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* less */
    { .keysym = 0x003c, .scancode = 0x56,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* equal */
    { .keysym = 0x003d, .scancode = 0x0D,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* greater */
    { .keysym = 0x003e, .scancode = 0x56,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT  },

    /* question */
    { .keysym = 0x003f, .scancode = 0x32,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* at */
    { .keysym = 0x0040, .scancode = 0x0B,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT,
        .set_keysyms = GUAC_KEYSYMS_ALTGR },

    /* A */
    { .keysym = 0x0041, .scancode = 0x10,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* B */
    { .keysym = 0x0042, .scancode = 0x30,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* C */
    { .keysym = 0x0043, .scancode = 0x2E,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* D */
    { .keysym = 0x0044, .scancode = 0x20,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* E */
    { .keysym = 0x0045, .scancode = 0x12,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* F */
    { .keysym = 0x0046, .scancode = 0x21,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* G */
    { .keysym = 0x0047, .scancode = 0x22,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* H */
    { .keysym = 0x0048, .scancode = 0x23,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* I */
    { .keysym = 0x0049, .scancode = 0x17,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* J */
    { .keysym = 0x004a, .scancode = 0x24,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* K */
    { .keysym = 0x004b, .scancode = 0x25,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* L */
    { .keysym = 0x004c, .scancode = 0x26,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* M */
    { .keysym = 0x004d, .scancode = 0x27,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* N */
    { .keysym = 0x004e, .scancode = 0x31,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* O */
    { .keysym = 0x004f, .scancode = 0x18,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* P */
    { .keysym = 0x0050, .scancode = 0x19,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* Q */
    { .keysym = 0x0051, .scancode = 0x1E,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* R */
    { .keysym = 0x0052, .scancode = 0x13,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* S */
    { .keysym = 0x0053, .scancode = 0x1F,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* T */
    { .keysym = 0x0054, .scancode = 0x14,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* U */
    { .keysym = 0x0055, .scancode = 0x16,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* V */
    { .keysym = 0x0056, .scancode = 0x2F,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* W */
    { .keysym = 0x0057, .scancode = 0x2C,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* X */
    { .keysym = 0x0058, .scancode = 0x2D,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* Y */
    { .keysym = 0x0059, .scancode = 0x15,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* Z */
    { .keysym = 0x005a, .scancode = 0x11,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* bracketleft */
    { .keysym = 0x005b, .scancode = 0x06,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT,
        .set_keysyms = GUAC_KEYSYMS_ALTGR },

    /* backslash */
    { .keysym = 0x005c, .scancode = 0x09,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT,
        .set_keysyms = GUAC_KEYSYMS_ALTGR },

    /* bracketright */
    { .keysym = 0x005d, .scancode = 0x0c,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT,
        .set_keysyms = GUAC_KEYSYMS_ALTGR },

    /* asciicircum */
    { .keysym = 0x005e, .scancode = 0x0a,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT,
        .set_keysyms = GUAC_KEYSYMS_ALTGR },

    /* underscore */
    { .keysym = 0x005f, .scancode = 0x09,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* quoteleft */
    { .keysym = 0x0060, .scancode = 0x08,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT,
        .set_keysyms = GUAC_KEYSYMS_ALTGR },

    /* a */
    { .keysym = 0x0061, .scancode = 0x10,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* b */
    { .keysym = 0x0062, .scancode = 0x30,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* c */
    { .keysym = 0x0063, .scancode = 0x2E,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* d */
    { .keysym = 0x0064, .scancode = 0x20,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* e */
    { .keysym = 0x0065, .scancode = 0x12,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* f */
    { .keysym = 0x0066, .scancode = 0x21,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* g */
    { .keysym = 0x0067, .scancode = 0x22,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* h */
    { .keysym = 0x0068, .scancode = 0x23,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* i */
    { .keysym = 0x0069, .scancode = 0x17,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* j */
    { .keysym = 0x006a, .scancode = 0x24,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* k */
    { .keysym = 0x006b, .scancode = 0x25,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* l */
    { .keysym = 0x006c, .scancode = 0x26,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* m */
    { .keysym = 0x006d, .scancode = 0x27,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* n */
    { .keysym = 0x006e, .scancode = 0x31,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* o */
    { .keysym = 0x006f, .scancode = 0x18,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* p */
    { .keysym = 0x0070, .scancode = 0x19,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* q */
    { .keysym = 0x0071, .scancode = 0x1E,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* r */
    { .keysym = 0x0072, .scancode = 0x13,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* s */
    { .keysym = 0x0073, .scancode = 0x1F,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* t */
    { .keysym = 0x0074, .scancode = 0x14,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* u */
    { .keysym = 0x0075, .scancode = 0x16,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* v */
    { .keysym = 0x0076, .scancode = 0x2F,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* w */
    { .keysym = 0x0077, .scancode = 0x2C,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* x */
    { .keysym = 0x0078, .scancode = 0x2D,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* y */
    { .keysym = 0x0079, .scancode = 0x15,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* z */
    { .keysym = 0x007a, .scancode = 0x11,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* braceleft */
    { .keysym = 0x007b, .scancode = 0x05,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT,
        .set_keysyms = GUAC_KEYSYMS_ALTGR },

    /* bar */
    { .keysym = 0x007c, .scancode = 0x07,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT,
        .set_keysyms = GUAC_KEYSYMS_ALTGR },

    /* braceright */
    { .keysym = 0x007d, .scancode = 0x0D,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT,
        .set_keysyms = GUAC_KEYSYMS_ALTGR },

    /* asciitilde */
    { .keysym = 0x007e, .scancode = 0x03,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT,
        .set_keysyms = GUAC_KEYSYMS_ALTGR },

    /* pound */
    { .keysym = 0x00a3, .scancode = 0x1b,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* currency */
    { .keysym = 0x00a4, .scancode = 0x1b,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT,
        .set_keysyms = GUAC_KEYSYMS_ALTGR },

    /* section */
    { .keysym = 0x00a7, .scancode = 0x35,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* degree */
    { .keysym = 0x00b0, .scancode = 0x0c,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* micro */
    { .keysym = 0x00b5, .scancode = 0x2b,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* dead umlaut */
    { .keysym = 0xfe57, .scancode = 0x1a,
        .clear_keysyms = GUAC_KEYSYMS_ALTGR,
        .set_keysyms = GUAC_KEYSYMS_SHIFT },

    /* dead circum */
    { .keysym = 0xfe52, .scancode = 0x1a,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* à */
    { .keysym = 0x00e0, .scancode = 0x0b,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* ç */
    { .keysym = 0x00e7, .scancode = 0x0a,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* è */
    { .keysym = 0x00e8, .scancode = 0x08,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* é */
    { .keysym = 0x00e9, .scancode = 0x03,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* ù */
    { .keysym = 0x00f9, .scancode = 0x28,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    /* euro */
    { .keysym = 0x10020ac, .scancode = 0x12,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT,
        .set_keysyms = GUAC_KEYSYMS_ALTGR },

    /* two superior */
    { .keysym = 0x00b2, .scancode = 0x29,
        .clear_keysyms = GUAC_KEYSYMS_ALL_SHIFT_ALTGR },

    {0}

};

const guac_rdp_keymap guac_rdp_keymap_fr_fr = {

    .name = "fr-fr-azerty",

    .parent = &guac_rdp_keymap_base,
    .mapping = __guac_rdp_keymap_mapping,
    .freerdp_keyboard_layout = KBD_FRENCH

};

