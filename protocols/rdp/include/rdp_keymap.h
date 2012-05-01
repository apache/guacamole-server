
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

#include <freerdp/kbd/layouts.h>

/**
 * Represents a keysym-to-scancode mapping for RDP, with extra information
 * about the state of prerequisite keysyms.
 */
typedef struct guac_rdp_keysym_desc {

    /**
     * The keysym being mapped.
     */
    int keysym;

    /**
     * The scancode this keysym maps to.
     */
    int scancode;

    /**
     * Required RDP-specific flags.
     */
    int flags;

    /**
     * Null-terminated list of keysyms which must be down for this keysym
     * to be properly typed.
     */
    const int* set_keysyms;

    /**
     * Null-terminated list of keysyms which must be up for this keysym
     * to be properly typed.
     */
    const int* clear_keysyms;

} guac_rdp_keysym_desc;

/**
 * Hierarchical keysym mapping
 */
typedef struct guac_rdp_keymap guac_rdp_keymap;
struct guac_rdp_keymap {

    /**
     * The parent mapping this map will inherit its initial mapping from.
     * Any other mapping information will add to or override the mapping
     * inherited from the parent.
     */
    const guac_rdp_keymap* parent;

    /**
     * Descriptive name of this keymap
     */
    const char* name;

    /**
     * Null-terminated array of scancode mappings.
     */
    const guac_rdp_keysym_desc* mapping;

    /**
     * FreeRDP keyboard layout associated with this
     * keymap. If this keymap is selected, this layout
     * will be requested from the server.
     */
    const uint32 freerdp_keyboard_layout;

};

/**
 * Static mapping from keysyms to scancodes.
 */
typedef guac_rdp_keysym_desc guac_rdp_static_keymap[256][256];

/**
 * Mapping from keysym to current state
 */
typedef int guac_rdp_keysym_state_map[256][256];

/**
 * Map of X11 keysyms to RDP scancodes (US English).
 */
extern const guac_rdp_keymap guac_rdp_keymap_en_us;

/**
 * Map of X11 keysyms to RDP scancodes (common non-printable keys).
 */
extern const guac_rdp_keymap guac_rdp_keymap_base;

/**
 * Simple macro for referencing the mapped value of an altcode or scancode for a given keysym.
 */
#define GUAC_RDP_KEYSYM_LOOKUP(keysym_mapping, keysym) ((keysym_mapping)[((keysym) & 0xFF00) >> 8][(keysym) & 0xFF])

/**
 * Keysym string containing only the left "shift" key.
 */
extern const int GUAC_KEYSYMS_SHIFT[];

/**
 * Keysym string containing both "shift" keys.
 */
extern const int GUAC_KEYSYMS_ALL_SHIFT[];

/**
 * Keysym string containing only the left "ctrl" key.
 */
extern const int GUAC_KEYSYMS_CTRL[];

/**
 * Keysym string containing both "ctrl" keys.
 */
extern const int GUAC_KEYSYMS_ALL_CTRL[];

/**
 * Keysym string containing only the left "alt" key.
 */
extern const int GUAC_KEYSYMS_ALT[];

/**
 * Keysym string containing both "alt" keys.
 */
extern const int GUAC_KEYSYMS_ALL_ALT[];

/**
 * Keysym string containing all modifier keys.
 */
extern const int GUAC_KEYSYMS_ALL_MODIFIERS[];

#endif

