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

#ifndef GUAC_RDP_KEYMAP_H
#define GUAC_RDP_KEYMAP_H

#include <winpr/wtypes.h>

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

    /**
     * Bitwise OR of the flags of all lock keys (ie: Caps lock, Num lock, etc.)
     * which must be active for this keysym to be properly typed. Legal flags
     * are KBD_SYNC_SCROLL_LOCK, KBD_SYNC_NUM_LOCK, KBD_SYNC_CAPS_LOCK, and
     * KBD_SYNC_KANA_LOCK.
     */
    int set_locks;

    /**
     * Bitwise OR of the flags of all lock keys (ie: Caps lock, Num lock, etc.)
     * which must be inactive for this keysym to be properly typed. Legal flags
     * are KBD_SYNC_SCROLL_LOCK, KBD_SYNC_NUM_LOCK, KBD_SYNC_CAPS_LOCK, and
     * KBD_SYNC_KANA_LOCK.
     */
    int clear_locks;

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
    const UINT32 freerdp_keyboard_layout;

};

/**
 * The name of the default keymap, which MUST exist.
 */
#define GUAC_DEFAULT_KEYMAP "en-us-qwerty"

/**
 * Keysym string containing only the left "shift" key.
 */
extern const int GUAC_KEYSYMS_SHIFT[];

/**
 * Keysym string containing both "shift" keys.
 */
extern const int GUAC_KEYSYMS_ALL_SHIFT[];

/**
 * Keysym string containing only the right "alt" key (AltGr).
 */
extern const int GUAC_KEYSYMS_ALTGR[];

/**
 * Keysym string containing the right "alt" key (AltGr) and
 * left shift.
 */
extern const int GUAC_KEYSYMS_SHIFT_ALTGR[];

/**
 * Keysym string containing the right "alt" key (AltGr) and
 * both shift keys.
 */
extern const int GUAC_KEYSYMS_ALL_SHIFT_ALTGR[];

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
 * Keysym string containing the left "alt" and left "ctrl" keys
 */
extern const int GUAC_KEYSYMS_CTRL_ALT[];

/**
 * Keysym string containing all modifier keys.
 */
extern const int GUAC_KEYSYMS_ALL_MODIFIERS[];

/**
 * NULL-terminated array of all keymaps.
 */
extern const guac_rdp_keymap* GUAC_KEYMAPS[];

/**
 * Return the keymap having the given name, if any, or NULL otherwise.
 *
 * @param name
 *     The name of the keymap to find.
 *
 * @return
 *     The keymap having the given name, or NULL if no such keymap exists.
 */
const guac_rdp_keymap* guac_rdp_keymap_find(const char* name);

#endif

