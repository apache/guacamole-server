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

#include "decompose.h"
#include "keyboard.h"
#include "keymap.h"
#include "rdp.h"

#include <freerdp/freerdp.h>
#include <freerdp/input.h>
#include <guacamole/client.h>

#include <stdlib.h>

/**
 * Translates the given keysym into the corresponding lock flag, as would be
 * required by the RDP synchronize event. If the given keysym does not
 * represent a lock key, zero is returned.
 *
 * @param keysym
 *     The keysym to translate into a RDP lock flag.
 *
 * @return
 *     The RDP lock flag which corresponds to the given keysym, or zero if the
 *     given keysym does not represent a lock key.
 */
static int guac_rdp_keyboard_lock_flag(int keysym) {

    /* Translate keysym into corresponding lock flag */
    switch (keysym) {

        /* Scroll lock */
        case 0xFF14:
            return KBD_SYNC_SCROLL_LOCK;

        /* Kana lock */
        case 0xFF2D:
            return KBD_SYNC_KANA_LOCK;

        /* Num lock */
        case 0xFF7F:
            return KBD_SYNC_NUM_LOCK;

        /* Caps lock */
        case 0xFFE5:
            return KBD_SYNC_CAPS_LOCK;

    }

    /* Not a lock key */
    return 0;

}

/**
 * Immediately sends an RDP key event having the given scancode and flags.
 *
 * @param rdp_client
 *     The RDP client instance associated with the RDP session along which the
 *     key event should be sent.
 *
 * @param scancode
 *     The scancode of the key to press or release via the RDP key event.
 *
 * @param flags
 *     Any RDP-specific flags required for the provided scancode to have the
 *     intended meaning, such as KBD_FLAGS_EXTENDED. The possible flags and
 *     their meanings are dictated by RDP. KBD_FLAGS_DOWN and KBD_FLAGS_UP
 *     need not be specified here - they will automatically be added depending
 *     on the value specified for the pressed parameter.
 *
 * @param pressed
 *     Non-zero if the key is being pressed, zero if the key is being released.
 */
static void guac_rdp_send_key_event(guac_rdp_client* rdp_client,
        int scancode, int flags, int pressed) {

    /* Determine proper event flag for pressed state */
    int pressed_flags;
    if (pressed)
        pressed_flags = KBD_FLAGS_DOWN;
    else
        pressed_flags = KBD_FLAGS_RELEASE;

    /* Skip if not yet connected */
    freerdp* rdp_inst = rdp_client->rdp_inst;
    if (rdp_inst == NULL)
        return;

    /* Send actual key */
    rdp_inst->input->KeyboardEvent(rdp_inst->input,
            flags | pressed_flags, scancode);

}

/**
 * Immediately sends an RDP Unicode event having the given Unicode codepoint.
 * Unlike key events, RDP Unicode events do have not a pressed or released
 * state. They represent strictly the input of a single character, and are
 * technically independent of the keyboard.
 *
 * @param rdp_client
 *     The RDP client instance associated with the RDP session along which the
 *     Unicode event should be sent.
 *
 * @param codepoint
 *     The Unicode codepoint of the character being input via the Unicode
 *     event.
 */
static void guac_rdp_send_unicode_event(guac_rdp_client* rdp_client,
        int codepoint) {

    /* Skip if not yet connected */
    freerdp* rdp_inst = rdp_client->rdp_inst;
    if (rdp_inst == NULL)
        return;

    /* Send Unicode event */
    rdp_inst->input->UnicodeKeyboardEvent(
            rdp_inst->input,
            0, codepoint);

}

/**
 * Immediately sends an RDP synchonize event having the given flags. An RDP
 * synchronize event sets the state of remote lock keys absolutely, where a
 * lock key will be active only if its corresponding flag is set in the event.
 *
 * @param rdp_client
 *     The RDP client instance associated with the RDP session along which the
 *     synchronize event should be sent.
 *
 * @param flags
 *     Bitwise OR of the flags representing the lock keys which should be set,
 *     if any, as dictated by the RDP protocol. If no flags are set, then no
 *     lock keys will be active.
 */
static void guac_rdp_send_synchronize_event(guac_rdp_client* rdp_client,
        int flags) {

    /* Skip if not yet connected */
    freerdp* rdp_inst = rdp_client->rdp_inst;
    if (rdp_inst == NULL)
        return;

    /* Synchronize lock key states */
    rdp_inst->input->SynchronizeEvent(rdp_inst->input, flags);

}

/**
 * Given a keyboard instance and X11 keysym, returns a pointer to the key
 * structure that represents or can represent the key having that keysym within
 * the keyboard, regardless of whether the key is currently defined. If no such
 * key can exist (the keysym cannot be mapped or is out of range), NULL is
 * returned.
 *
 * @param keyboard
 *     The guac_rdp_keyboard associated with the current RDP session.
 *
 * @param keysym
 *     The keysym of the key to lookup within the given keyboard.
 *
 * @return
 *     A pointer to the guac_rdp_key structure which represents or can
 *     represent the key having the given keysym, or NULL if no such keysym can
 *     be defined within a guac_rdp_keyboard structure.
 */
static guac_rdp_key* guac_rdp_keyboard_map_key(guac_rdp_keyboard* keyboard,
        int keysym) {

    int index;

    /* Map keysyms between 0x0000 and 0xFFFF directly */
    if (keysym >= 0x0000 && keysym <= 0xFFFF)
        index = keysym;

    /* Map all Unicode keysyms from U+0000 to U+FFFF */
    else if (keysym >= 0x1000000 && keysym <= 0x100FFFF)
        index = 0x10000 + (keysym & 0xFFFF);

    /* All other keysyms are unmapped */
    else
        return NULL;

    /* Corresponding key mapping (defined or not) has been located */
    return &(keyboard->keys[index]);

}

/**
 * Returns a pointer to the guac_rdp_key structure representing the definition
 * and state of the key having the given keysym. If no such key is defined
 * within the keyboard layout of the RDP server, NULL is returned.
 *
 * @param keyboard
 *     The guac_rdp_keyboard associated with the current RDP session.
 *
 * @param keysym
 *     The keysym of the key to lookup within the given keyboard.
 *
 * @return
 *     A pointer to the guac_rdp_key structure representing the definition and
 *     state of the key having the given keysym, or NULL if no such key is
 *     defined within the keyboard layout of the RDP server.
 */
static guac_rdp_key* guac_rdp_keyboard_get_key(guac_rdp_keyboard* keyboard,
        int keysym) {

    /* Verify that the key is actually defined */
    guac_rdp_key* key = guac_rdp_keyboard_map_key(keyboard, keysym);
    if (key == NULL || key->definition == NULL)
        return NULL;

    /* Key is defined within keyboard */
    return key;

}

/**
 * Loads all keysym/scancode mappings declared within the given keymap and its
 * parent keymap, if any. These mappings are stored within the given
 * guac_rdp_keyboard structure for future use in translating keysyms to the
 * scancodes required by RDP key events.
 *
 * @param keyboard
 *     The guac_rdp_keyboard which should be initialized with the
 *     keysym/scancode mapping defined in the given keymap.
 *
 * @param keymap
 *     The keymap to use to populate the given client's keysym/scancode
 *     mapping.
 */
static void __guac_rdp_keyboard_load_keymap(guac_rdp_keyboard* keyboard,
        const guac_rdp_keymap* keymap) {

    /* Get mapping */
    const guac_rdp_keysym_desc* mapping = keymap->mapping;

    /* If parent exists, load parent first */
    if (keymap->parent != NULL)
        __guac_rdp_keyboard_load_keymap(keyboard, keymap->parent);

    /* Log load */
    guac_client_log(keyboard->client, GUAC_LOG_INFO,
            "Loading keymap \"%s\"", keymap->name);

    /* Load mapping into keymap */
    while (mapping->keysym != 0) {

        /* Locate corresponding key definition within keyboard */
        guac_rdp_key* key = guac_rdp_keyboard_map_key(keyboard,
                mapping->keysym);

        /* Copy mapping (if key is mappable) */
        if (key != NULL)
            key->definition = mapping;
        else
            guac_client_log(keyboard->client, GUAC_LOG_DEBUG,
                    "Ignoring unmappable keysym 0x%X", mapping->keysym);

        /* Next keysym */
        mapping++;

    }

}

guac_rdp_keyboard* guac_rdp_keyboard_alloc(guac_client* client,
        const guac_rdp_keymap* keymap) {

    guac_rdp_keyboard* keyboard = calloc(1, sizeof(guac_rdp_keyboard));
    keyboard->client = client;

    /* Load keymap into keyboard */
    __guac_rdp_keyboard_load_keymap(keyboard, keymap);

    return keyboard;

}

void guac_rdp_keyboard_free(guac_rdp_keyboard* keyboard) {
    free(keyboard);
}

int guac_rdp_keyboard_is_defined(guac_rdp_keyboard* keyboard, int keysym) {

    /* Return whether the mapping actually exists */
    return guac_rdp_keyboard_get_key(keyboard, keysym) != NULL;

}

int guac_rdp_keyboard_send_event(guac_rdp_keyboard* keyboard,
        int keysym, int pressed) {

    guac_client* client = keyboard->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* If keysym is actually defined within keyboard */
    guac_rdp_key* key = guac_rdp_keyboard_get_key(keyboard, keysym);
    if (key != NULL) {

        /* Look up scancode mapping */
        const guac_rdp_keysym_desc* keysym_desc = key->definition;

        /* If defined, send event */
        if (keysym_desc->scancode != 0) {

            /* Update remote lock state as necessary */
            guac_rdp_keyboard_update_locks(keyboard,
                    keysym_desc->set_locks,
                    keysym_desc->clear_locks);

            /* If defined, send any prerequesite keys that must be set */
            if (keysym_desc->set_keysyms != NULL)
                guac_rdp_keyboard_send_events(keyboard,
                        keysym_desc->set_keysyms,
                        GUAC_RDP_KEY_RELEASED,
                        GUAC_RDP_KEY_PRESSED);

            /* If defined, release any keys that must be cleared */
            if (keysym_desc->clear_keysyms != NULL)
                guac_rdp_keyboard_send_events(keyboard,
                        keysym_desc->clear_keysyms,
                        GUAC_RDP_KEY_PRESSED,
                        GUAC_RDP_KEY_RELEASED);

            /* Fire actual key event for target key */
            guac_rdp_send_key_event(rdp_client, keysym_desc->scancode,
                    keysym_desc->flags, pressed);

            /* If defined, release any keys that were originally released */
            if (keysym_desc->set_keysyms != NULL)
                guac_rdp_keyboard_send_events(keyboard,
                        keysym_desc->set_keysyms,
                        GUAC_RDP_KEY_RELEASED,
                        GUAC_RDP_KEY_RELEASED);

            /* If defined, send any keys that were originally set */
            if (keysym_desc->clear_keysyms != NULL)
                guac_rdp_keyboard_send_events(keyboard,
                        keysym_desc->clear_keysyms,
                        GUAC_RDP_KEY_PRESSED,
                        GUAC_RDP_KEY_PRESSED);

            return 0;

        }
    }

    /* Fall back to dead keys or Unicode events if otherwise undefined inside
     * current keymap (note that we only handle "pressed" here, as neither
     * Unicode events nor dead keys can have a pressed/released state) */
    if (pressed) {

        /* Attempt to type using dead keys */
        if (!guac_rdp_decompose_keysym(keyboard, keysym))
            return 0;

        guac_client_log(client, GUAC_LOG_DEBUG,
                "Sending keysym 0x%x as Unicode", keysym);

        /* Translate keysym into codepoint */
        int codepoint;
        if (keysym <= 0xFF)
            codepoint = keysym;
        else if (keysym >= 0x1000000)
            codepoint = keysym & 0xFFFFFF;
        else {
            guac_client_log(client, GUAC_LOG_DEBUG,
                    "Unmapped keysym has no equivalent unicode "
                    "value: 0x%x", keysym);
            return 0;
        }

        /* Send as Unicode event */
        guac_rdp_send_unicode_event(rdp_client, codepoint);

    }
    
    return 0;
}

void guac_rdp_keyboard_send_events(guac_rdp_keyboard* keyboard,
        const int* keysym_string, guac_rdp_key_state from,
        guac_rdp_key_state to) {

    int keysym;

    /* Send all keysyms in string, NULL terminated */
    while ((keysym = *keysym_string) != 0) {

        /* If key is currently in given state, send event for changing it to
         * specified "to" state */
        guac_rdp_key* key = guac_rdp_keyboard_get_key(keyboard, keysym);
        if (key != NULL && key->state == from)
            guac_rdp_keyboard_send_event(keyboard, *keysym_string, to);

        /* Next keysym */
        keysym_string++;

    }

}

void guac_rdp_keyboard_update_locks(guac_rdp_keyboard* keyboard,
        int set_flags, int clear_flags) {

    guac_client* client = keyboard->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Calculate updated lock flags */
    int lock_flags = (keyboard->lock_flags | set_flags) & ~clear_flags;

    /* Synchronize remote side only if lock flags have changed */
    if (lock_flags != keyboard->lock_flags) {
        guac_rdp_send_synchronize_event(rdp_client, lock_flags);
        keyboard->lock_flags = lock_flags;
    }

}

int guac_rdp_keyboard_update_keysym(guac_rdp_keyboard* keyboard,
        int keysym, int pressed) {

    /* Synchronize lock keys states, if this has not yet been done */
    if (!keyboard->synchronized) {

        guac_client* client = keyboard->client;
        guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

        /* Synchronize remote lock key states with local state */
        guac_rdp_send_synchronize_event(rdp_client, keyboard->lock_flags);
        keyboard->synchronized = 1;

    }

    /* Toggle lock flag, if any */
    if (pressed)
        keyboard->lock_flags ^= guac_rdp_keyboard_lock_flag(keysym);

    /* Update keysym state */
    guac_rdp_key* key = guac_rdp_keyboard_get_key(keyboard, keysym);
    if (key != NULL)
        key->state = pressed ? GUAC_RDP_KEY_PRESSED : GUAC_RDP_KEY_RELEASED;

    return guac_rdp_keyboard_send_event(keyboard, keysym, pressed);

}

