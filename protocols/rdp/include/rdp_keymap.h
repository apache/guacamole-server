
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

#ifndef _GUAC_RDP_RDP_KEYMAP_H
#define _GUAC_RDP_RDP_KEYMAP_H

/**
 * Represents a keysym-to-scancode mapping for RDP, with extra information
 * about the state of prerequisite keysyms.
 */
typedef struct guac_rdp_scancode_map {

    /**
     * The scancode this keysym maps to.
     */
    int scancode;

    /**
     * Required RDP-specific flags
     */
    int flags;

    /**
     * Null-terminated list of keysyms which must be down for this keysym
     * to be properly typed.
     */
    int* set_keysyms;

    /**
     * Null-terminated list of keysyms which must be up for this keysym
     * to be properly typed.
     */
    int* clear_keysyms;

} guac_rdp_scancode_map;

/**
 * Represents the Alt-code which types a given keysym. This is used as a
 * fallback mapping, should a particular keymap not support a certain keysym.
 *
 * See: http://en.wikipedia.org/wiki/Alt_code
 */
typedef struct guac_rdp_altcode_map {

    /**
     * The 4-digit Alt-code which types this keysym.
     */
    const char* altcode;

} guac_rdp_altcode_map;

/**
 * Static mapping from keysyms to scancodes.
 */
typedef guac_rdp_scancode_map guac_rdp_keysym_scancode_map[256][256];

/**
 * Static mapping from keysyms to Alt-codes.
 */
typedef guac_rdp_altcode_map guac_rdp_keysym_altcode_map[256][256];

/**
 * Map of X11 keysyms to RDP scancodes (US English).
 */
extern const guac_rdp_keysym_scancode_map guac_rdp_keysym_scancode_en_us;

/**
 * Map of X11 keysyms to Windows Alt-codes.
 */
extern const guac_rdp_keysym_altcode_map guac_rdp_keysym_altcode;

/**
 * Simple macro for referencing the mapped value of an altcode or scancode for a given keysym.
 */
#define GUAC_RDP_KEYSYM_LOOKUP(keysym_mapping, keysym) (&((keysym_mapping)[((keysym) & 0xFF00) >> 8][(keysym) & 0xFF]))

#endif

