
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
    { .keysym = 0xffe4, .scancode = 0x1D },

    /* Caps_Lock */
    { .keysym = 0xffe5, .scancode = 0x3A,
        .flags = KBD_FLAGS_EXTENDED },

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

const guac_rdp_keymap guac_rdp_keymap_base = {

    .name = "base",

    .parent = NULL,
    .mapping = __guac_rdp_keymap_mapping

};

