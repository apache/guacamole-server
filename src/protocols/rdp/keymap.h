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
 * Bitwise flag value representing the Shift modifier.
 */
#define GUAC_RDP_KEYMAP_MODIFIER_SHIFT 1

/**
 * Bitwise flag value representing the AltGr modifier.
 */
#define GUAC_RDP_KEYMAP_MODIFIER_ALTGR 2

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
     * Required RDP-specific flags that must be sent along with the scancode.
     */
    int flags;

    /**
     * Bitwise-OR of the flags of any modifiers that must be active for the
     * associated scancode to be interpreted as this keysym.
     *
     * If the associated keysym is pressed, and any of these modifiers are not
     * currently active, Guacamole's RDP support must send additional events to
     * activate these modifiers prior to sending the scancode for this keysym.
     *
     * @see GUAC_RDP_KEYMAP_MODIFIER_SHIFT
     * @see GUAC_RDP_KEYMAP_MODIFIER_ALTGR
     */
    const unsigned int set_modifiers;

    /**
     * Bitwise-OR of the flags of any modifiers that must NOT be active for the
     * associated scancode to be interpreted as this keysym.
     *
     * If the associated keysym is pressed, and any of these modifiers are
     * currently active, Guacamole's RDP support must send additional events to
     * deactivate these modifiers prior to sending the scancode for this
     * keysym.
     *
     * @see GUAC_RDP_KEYMAP_MODIFIER_SHIFT
     * @see GUAC_RDP_KEYMAP_MODIFIER_ALTGR
     */
    const unsigned int clear_modifiers;

    /**
     * Bitwise OR of the flags of all lock keys (ie: Caps lock, Num lock, etc.)
     * which must be active for this keysym to be properly typed. Legal flags
     * are KBD_SYNC_SCROLL_LOCK, KBD_SYNC_NUM_LOCK, KBD_SYNC_CAPS_LOCK, and
     * KBD_SYNC_KANA_LOCK.
      */
    const unsigned int set_locks;

    /**
     * Bitwise OR of the flags of all lock keys (ie: Caps lock, Num lock, etc.)
     * which must be inactive for this keysym to be properly typed. Legal flags
     * are KBD_SYNC_SCROLL_LOCK, KBD_SYNC_NUM_LOCK, KBD_SYNC_CAPS_LOCK, and
     * KBD_SYNC_KANA_LOCK.
     */
    const unsigned int clear_locks;

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

