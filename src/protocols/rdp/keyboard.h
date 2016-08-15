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

#ifndef GUAC_RDP_KEYBOARD_H
#define GUAC_RDP_KEYBOARD_H

#include "rdp_keymap.h"

#include <guacamole/client.h>

/**
 * The current keyboard state of an RDP session.
 */
typedef struct guac_rdp_keyboard {

    /**
     * The guac_client associated with the RDP session whose keyboard state is
     * being managed by this guac_rdp_keyboard.
     */
    guac_client* client;

    /**
     * The local state of all known lock keys, as a bitwise OR of all RDP lock
     * key flags. Legal flags are KBD_SYNC_SCROLL_LOCK, KBD_SYNC_NUM_LOCK,
     * KBD_SYNC_CAPS_LOCK, and KBD_SYNC_KANA_LOCK.
     */
    int lock_flags;

    /**
     * Whether the states of remote lock keys (Caps lock, Num lock, etc.) have
     * been synchronized with local lock key states.
     */
    int synchronized;

    /**
     * The keymap to use when translating keysyms into scancodes or sequences
     * of scancodes for RDP.
     */
    guac_rdp_static_keymap keymap;

    /**
     * The local state of all keys, based on whether Guacamole key events for
     * pressing/releasing particular keysyms have been received. This is used
     * together with the associated keymap to determine the sequence of RDP key
     * events sent to duplicate the effect of a particular keysym.
     */
    guac_rdp_keysym_state_map keysym_state;

} guac_rdp_keyboard;

/**
 * Allocates a new guac_rdp_keyboard which manages the keyboard state of the
 * RDP session associated with the given guac_client. Keyboard events will be
 * dynamically translated from keysym to RDP scancode according to the provided
 * keymap. The returned guac_rdp_keyboard must eventually be freed with
 * guac_rdp_keyboard_free().
 *
 * @param client
 *     The guac_client associated with the RDP session whose keyboard state is
 *     to be managed by the newly-allocated guac_rdp_keyboard.
 *
 * @param keymap
 *     The keymap which should be used to translate keyboard events.
 *
 * @return
 *     A newly-allocated guac_rdp_keyboard which manages the keyboard state
 *     for the RDP session associated given guac_client.
 */
guac_rdp_keyboard* guac_rdp_keyboard_alloc(guac_client* client,
        const guac_rdp_keymap* keymap);

/**
 * Frees all memory allocated for the given guac_rdp_keyboard. The
 * guac_rdp_keyboard must have been previously allocated via
 * guac_rdp_keyboard_alloc().
 *
 * @param keyboard
 *     The guac_rdp_keyboard instance which should be freed.
 */
void guac_rdp_keyboard_free(guac_rdp_keyboard* keyboard);

/**
 * Sends one or more RDP key events, effectively pressing or releasing the
 * given keysym on the remote side. The key events sent will depend on the
 * current keymap. The locally-stored state of each key is remains untouched.
 *
 * @param keyboard
 *     The guac_rdp_keyboard associated with the current RDP session.
 *
 * @param keysym
 *     The keysym being pressed or released.
 *
 * @param pressed
 *     Zero if the keysym is being released, non-zero otherwise.
 *
 * @return
 *     Zero if the keys were successfully sent, non-zero otherwise.
 */
int guac_rdp_keyboard_send_event(guac_rdp_keyboard* keyboard,
        int keysym, int pressed);

/**
 * For every keysym in the given NULL-terminated array of keysyms, send the RDP
 * key events required to update the remote state of those keys as specified,
 * depending on the current local state of those keysyms.  For each key in the
 * "from" state (0 being released and 1 being pressed), that key will be
 * updated to the "to" state. The locally-stored state of each key is remains
 * untouched.
 *
 * @param keyboard
 *     The guac_rdp_keyboard associated with the current RDP session.
 *
 * @param keysym_string
 *     A NULL-terminated array of keysyms, each of which will be updated.
 *
 * @param from
 *     0 if the state of currently-released keys should be updated, or 1 if
 *     the state of currently-pressed keys should be updated.
 *
 * @param to 
 *     0 if the keys being updated should be marked as released, or 1 if
 *     the keys being updated should be marked as pressed.
 */
void guac_rdp_keyboard_send_events(guac_rdp_keyboard* keyboard,
        const int* keysym_string, int from, int to);

/**
 * Updates the local state of the lock keys (such as Caps lock or Num lock),
 * synchronizing the remote state of those keys if it is expected to differ.
 *
 * @param keyboard
 *     The guac_rdp_keyboard associated with the current RDP session.
 *
 * @param set_flags
 *     The lock key flags which should be set. Legal flags are
 *     KBD_SYNC_SCROLL_LOCK, KBD_SYNC_NUM_LOCK, KBD_SYNC_CAPS_LOCK, and
 *     KBD_SYNC_KANA_LOCK.
 *
 * @param clear_flags
 *     The lock key flags which should be cleared. Legal flags are
 *     KBD_SYNC_SCROLL_LOCK, KBD_SYNC_NUM_LOCK, KBD_SYNC_CAPS_LOCK, and
 *     KBD_SYNC_KANA_LOCK.
 */
void guac_rdp_keyboard_update_locks(guac_rdp_keyboard* keyboard,
        int set_flags, int clear_flags);

/**
 * Updates the local state of the given keysym, sending the key events required
 * to replicate that state remotely (on the RDP server). The key events sent
 * will depend on the current keymap.
 *
 * @param keyboard
 *     The guac_rdp_keyboard associated with the current RDP session.
 *
 * @param keysym
 *     The keysym being pressed or released.
 *
 * @param pressed
 *     Zero if the keysym is being released, non-zero otherwise.
 *
 * @return
 *     Zero if the keys were successfully sent, non-zero otherwise.
 */
int guac_rdp_keyboard_update_keysym(guac_rdp_keyboard* keyboard,
        int keysym, int pressed);

#endif

