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
#include <guacamole/mem.h>
#include <guacamole/rwlock.h>
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
        case GUAC_RDP_KEYSYM_SCROLL_LOCK:
            return KBD_SYNC_SCROLL_LOCK;

        /* Kana lock */
        case GUAC_RDP_KEYSYM_KANA_LOCK:
            return KBD_SYNC_KANA_LOCK;

        /* Num lock */
        case GUAC_RDP_KEYSYM_NUM_LOCK:
            return KBD_SYNC_NUM_LOCK;

        /* Caps lock */
        case GUAC_RDP_KEYSYM_CAPS_LOCK:
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
    pthread_mutex_lock(&(rdp_client->message_lock));
    GUAC_RDP_CONTEXT(rdp_inst)->input->KeyboardEvent(
            GUAC_RDP_CONTEXT(rdp_inst)->input, flags | pressed_flags, scancode);
    pthread_mutex_unlock(&(rdp_client->message_lock));

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
    pthread_mutex_lock(&(rdp_client->message_lock));
    GUAC_RDP_CONTEXT(rdp_inst)->input->UnicodeKeyboardEvent(
            GUAC_RDP_CONTEXT(rdp_inst)->input, 0, codepoint);
    pthread_mutex_unlock(&(rdp_client->message_lock));

}

/**
 * Immediately sends an RDP synchronize event having the given flags. An RDP
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
        UINT32 flags) {

    /* Skip if not yet connected */
    freerdp* rdp_inst = rdp_client->rdp_inst;
    if (rdp_inst == NULL)
        return;

    /* Synchronize lock key states */
    pthread_mutex_lock(&(rdp_client->message_lock));
    GUAC_RDP_CONTEXT(rdp_inst)->input->SynchronizeEvent(
            GUAC_RDP_CONTEXT(rdp_inst)->input, flags);
    pthread_mutex_unlock(&(rdp_client->message_lock));

}

/**
 * Given a keyboard instance and X11 keysym, returns a pointer to the
 * keys_by_keysym entry that represents the key having that keysym within the
 * keyboard, regardless of whether the key is currently defined. If no such key
 * can exist (the keysym cannot be mapped or is out of range), NULL is
 * returned.
 *
 * @param keyboard
 *     The guac_rdp_keyboard associated with the current RDP session.
 *
 * @param keysym
 *     The keysym of the key to lookup within the given keyboard.
 *
 * @return
 *     A pointer to the keys_by_keysym entry which represents or can represent
 *     the key having the given keysym, or NULL if no such keysym can be
 *     defined within a guac_rdp_keyboard structure.
 */
static guac_rdp_key** guac_rdp_keyboard_map_key(guac_rdp_keyboard* keyboard,
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
    return &(keyboard->keys_by_keysym[index]);

}

/**
 * Returns the number of bits that are set within the given integer (the number
 * of 1s in the binary expansion of the given integer).
 *
 * @param value
 *     The integer to read.
 *
 * @return
 *     The number of bits that are set within the given integer.
 */
static int guac_rdp_count_bits(unsigned int value) {

    int bits = 0;

    while (value) {
        bits += value & 1;
        value >>= 1;
    }

    return bits;

}

/**
 * Returns an estimated cost for sending the necessary RDP events to type the
 * key described by the given guac_rdp_keysym_desc, given the current lock and
 * modifier state of the keyboard. A higher cost value indicates that a greater
 * number of events are expected to be required.
 *
 * Lower-cost approaches should be preferred when multiple alternatives exist
 * for typing a particular key, as the lower cost implies fewer additional key
 * events required to produce the expected behavior. For example, if Caps Lock
 * is enabled, typing an uppercase "A" by pressing the "A" key has a lower cost
 * than disabling Caps Lock and pressing Shift+A.
 *
 * @param keyboard
 *     The guac_rdp_keyboard associated with the current RDP session.
 *
 * @param def
 *     The guac_rdp_keysym_desc that describes the key being pressed, as well
 *     as any requirements that must be satisfied for the key to be interpreted
 *     as expected.
 *
 * @return
 *     An arbitrary integer value which indicates the overall estimated
 *     complexity of typing the given key.
 */
static int guac_rdp_keyboard_get_cost(guac_rdp_keyboard* keyboard,
        const guac_rdp_keysym_desc* def) {

    unsigned int modifier_flags = guac_rdp_keyboard_get_modifier_flags(keyboard);

    /* Each change to any key requires one event, by definition */
    int cost = 1;

    /* Each change to a lock requires roughly two key events */
    unsigned int update_locks = (def->set_locks & ~keyboard->lock_flags) | (def->clear_locks & keyboard->lock_flags);
    cost += guac_rdp_count_bits(update_locks) * 2;

    /* Each change to a modifier requires one key event */
    unsigned int update_modifiers = (def->clear_modifiers & modifier_flags) | (def->set_modifiers & ~modifier_flags);
    cost += guac_rdp_count_bits(update_modifiers);

    return cost;

}

/**
 * Returns a pointer to the guac_rdp_key structure representing the
 * definition(s) and state of the key having the given keysym. If no such key
 * is defined within the keyboard layout of the RDP server, NULL is returned.
 *
 * @param keyboard
 *     The guac_rdp_keyboard associated with the current RDP session.
 *
 * @param keysym
 *     The keysym of the key to lookup within the given keyboard.
 *
 * @return
 *     A pointer to the guac_rdp_key structure representing the definition(s)
 *     and state of the key having the given keysym, or NULL if no such key is
 *     defined within the keyboard layout of the RDP server.
 */
static guac_rdp_key* guac_rdp_keyboard_get_key(guac_rdp_keyboard* keyboard,
        int keysym) {

    /* Verify that the key is actually defined */
    guac_rdp_key** key_by_keysym = guac_rdp_keyboard_map_key(keyboard, keysym);
    if (key_by_keysym == NULL)
        return NULL;

    return *key_by_keysym;

}

/**
 * Given a key which may have multiple possible definitions, returns the
 * definition that currently has the lowest cost, taking into account the
 * current keyboard lock and modifier states.
 *
 * @param keyboard
 *     The guac_rdp_keyboard associated with the current RDP session.
 *
 * @param key
 *     The key whose lowest-cost possible definition should be retrieved.
 *
 * @return
 *     A pointer to the guac_rdp_keysym_desc which defines the current
 *     lowest-cost method of typing the given key.
 */
static const guac_rdp_keysym_desc* guac_rdp_keyboard_get_definition(guac_rdp_keyboard* keyboard,
        guac_rdp_key* key) {

    /* Consistently map the same entry so long as the key is held */
    if (key->pressed != NULL)
        return key->pressed;

    /* Calculate cost of first definition of key (there must always be at least
     * one definition) */
    const guac_rdp_keysym_desc* best_def = key->definitions[0];
    int best_cost = guac_rdp_keyboard_get_cost(keyboard, best_def);

    /* If further definitions exist, choose the definition with the lowest
     * overall cost */
    for (int i = 1; i < key->num_definitions; i++) {

        const guac_rdp_keysym_desc* def = key->definitions[i];
        int cost = guac_rdp_keyboard_get_cost(keyboard, def);

        if (cost < best_cost) {
            best_def = def;
            best_cost = cost;
        }

    }

    return best_def;

}

/**
 * Adds the keysym/scancode mapping described by the given guac_rdp_keysym_desc
 * to the internal mapping of the keyboard. If insufficient space remains for
 * additional keysyms, or the given keysym has already reached the maximum
 * number of possible definitions, the mapping is ignored and the failure is
 * logged.
 *
 * @param keyboard
 *     The guac_rdp_keyboard associated with the current RDP session.
 *
 * @param mapping
 *     The keysym/scancode mapping that should be added to the given keyboard.
 */
static void guac_rdp_keyboard_add_mapping(guac_rdp_keyboard* keyboard,
        const guac_rdp_keysym_desc* mapping) {

    /* Locate corresponding keysym-to-key translation entry within keyboard
     * structure */
    guac_rdp_key** key_by_keysym = guac_rdp_keyboard_map_key(keyboard, mapping->keysym);
    if (key_by_keysym == NULL) {
        guac_client_log(keyboard->client, GUAC_LOG_DEBUG, "Ignoring unmappable keysym 0x%X", mapping->keysym);
        return;
    }

    /* If not yet pointing to a key, point keysym-to-key translation entry at
     * next available storage */
    if (*key_by_keysym == NULL) {

        if (keyboard->num_keys == GUAC_RDP_KEYBOARD_MAX_KEYSYMS) {
            guac_client_log(keyboard->client, GUAC_LOG_DEBUG, "Key definition "
                    "for keysym 0x%X dropped: Keymap exceeds maximum "
                    "supported number of keysyms",
                    mapping->keysym);
            return;
        }

        *key_by_keysym = &keyboard->keys[keyboard->num_keys++];

    }

    guac_rdp_key* key = *key_by_keysym;

    /* Add new definition only if sufficient space remains */
    if (key->num_definitions == GUAC_RDP_KEY_MAX_DEFINITIONS) {
        guac_client_log(keyboard->client, GUAC_LOG_DEBUG, "Key definition "
                "for keysym 0x%X dropped: Maximum number of possible "
                "definitions has been reached for this keysym",
                mapping->keysym);
        return;
    }

    /* Store new possible definition of key */
    key->definitions[key->num_definitions++] = mapping;

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
static void guac_rdp_keyboard_load_keymap(guac_rdp_keyboard* keyboard,
        const guac_rdp_keymap* keymap) {

    /* If parent exists, load parent first */
    if (keymap->parent != NULL)
        guac_rdp_keyboard_load_keymap(keyboard, keymap->parent);

    /* Log load */
    guac_client_log(keyboard->client, GUAC_LOG_INFO,
            "Loading keymap \"%s\"", keymap->name);

    /* Copy mapping into keymap */
    const guac_rdp_keysym_desc* mapping = keymap->mapping;
    while (mapping->keysym != 0) {
        guac_rdp_keyboard_add_mapping(keyboard, mapping++);
    }

}

guac_rdp_keyboard* guac_rdp_keyboard_alloc(guac_client* client,
        const guac_rdp_keymap* keymap) {

    guac_rdp_keyboard* keyboard = guac_mem_zalloc(sizeof(guac_rdp_keyboard));
    keyboard->client = client;

    /* Load keymap into keyboard */
    guac_rdp_keyboard_load_keymap(keyboard, keymap);

    return keyboard;

}

void guac_rdp_keyboard_free(guac_rdp_keyboard* keyboard) {
    guac_mem_free(keyboard);
}

int guac_rdp_keyboard_is_defined(guac_rdp_keyboard* keyboard, int keysym) {

    /* Return whether the mapping actually exists */
    return guac_rdp_keyboard_get_key(keyboard, keysym) != NULL;

}

int guac_rdp_keyboard_is_pressed(guac_rdp_keyboard* keyboard, int keysym) {

    guac_rdp_key* key = guac_rdp_keyboard_get_key(keyboard, keysym);
    return key != NULL && key->pressed != NULL;

}

unsigned int guac_rdp_keyboard_get_modifier_flags(guac_rdp_keyboard* keyboard) {

    unsigned int modifier_flags = 0;

    /* Shift */
    if (guac_rdp_keyboard_is_pressed(keyboard, GUAC_RDP_KEYSYM_LSHIFT)
            || guac_rdp_keyboard_is_pressed(keyboard, GUAC_RDP_KEYSYM_RSHIFT))
        modifier_flags |= GUAC_RDP_KEYMAP_MODIFIER_SHIFT;

    /* Dedicated AltGr key */
    if (guac_rdp_keyboard_is_pressed(keyboard, GUAC_RDP_KEYSYM_RALT)
            || guac_rdp_keyboard_is_pressed(keyboard, GUAC_RDP_KEYSYM_ALTGR))
        modifier_flags |= GUAC_RDP_KEYMAP_MODIFIER_ALTGR;

    /* AltGr via Ctrl+Alt */
    if (guac_rdp_keyboard_is_pressed(keyboard, GUAC_RDP_KEYSYM_LALT)
            && (guac_rdp_keyboard_is_pressed(keyboard, GUAC_RDP_KEYSYM_RCTRL)
                || guac_rdp_keyboard_is_pressed(keyboard, GUAC_RDP_KEYSYM_LCTRL)))
        modifier_flags |= GUAC_RDP_KEYMAP_MODIFIER_ALTGR;

    return modifier_flags;

}

/**
 * Presses/releases the requested key by sending one or more RDP key events, as
 * defined within the keymap defining that key.
 *
 * @param keyboard
 *     The guac_rdp_keyboard associated with the current RDP session.
 *
 * @param key
 *     The guac_rdp_keysym_desc of the key being pressed or released, as
 *     retrieved from the relevant keymap.
 *
 * @param pressed
 *     Zero if the key is being released, non-zero otherwise.
 *
 * @return
 *     Zero if the key was successfully pressed/released, non-zero if the key
 *     cannot be sent using RDP key events.
 */
static const guac_rdp_keysym_desc* guac_rdp_keyboard_send_defined_key(guac_rdp_keyboard* keyboard,
        guac_rdp_key* key, int pressed) {

    guac_client* client = keyboard->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    const guac_rdp_keysym_desc* keysym_desc = guac_rdp_keyboard_get_definition(keyboard, key);
    if (keysym_desc->scancode == 0)
        return NULL;

    /* Update state of required locks and modifiers only when key is just
     * now being pressed */
    if (pressed) {
        guac_rdp_keyboard_update_locks(keyboard,
                keysym_desc->set_locks,
                keysym_desc->clear_locks);

        guac_rdp_keyboard_update_modifiers(keyboard,
                keysym_desc->set_modifiers,
                keysym_desc->clear_modifiers);
    }

    /* Fire actual key event for target key */
    guac_rdp_send_key_event(rdp_client, keysym_desc->scancode,
            keysym_desc->flags, pressed);

    return keysym_desc;

}

/**
 * Presses and releases the requested key by sending one or more RDP events,
 * without relying on a keymap for that key. This will typically involve either
 * sending the key using a Unicode event or decomposing the key into a series
 * of keypresses involving deadkeys.
 *
 * @param keyboard
 *     The guac_rdp_keyboard associated with the current RDP session.
 *
 * @param keysym
 *     The keysym of the key to press and release.
 */
static void guac_rdp_keyboard_send_missing_key(guac_rdp_keyboard* keyboard,
        int keysym) {

    guac_client* client = keyboard->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
     
    /* If system modifiers (Ctrl/Alt) are held, prefer sending US scancode so
     * that shortcuts like Ctrl+C/V/A work even in Unicode/failsafe layout. */
    int ctrl_or_alt_pressed =
          guac_rdp_keyboard_is_pressed(keyboard, GUAC_RDP_KEYSYM_LCTRL)
       || guac_rdp_keyboard_is_pressed(keyboard, GUAC_RDP_KEYSYM_RCTRL)
       || guac_rdp_keyboard_is_pressed(keyboard, GUAC_RDP_KEYSYM_LALT)
       || guac_rdp_keyboard_is_pressed(keyboard, GUAC_RDP_KEYSYM_RALT);

    /* Normalize Cyrillic letters to Latin equivalents when Ctrl/Alt pressed
     * so that shortcuts like Ctrl+C, Ctrl+V, etc. work even in Russian layout.
     */
    if (ctrl_or_alt_pressed) {

       
/* Treat Ctrl or Alt (but NOT AltGr) as shortcut modifiers */
int ctrl_pressed =
      guac_rdp_keyboard_is_pressed(keyboard, GUAC_RDP_KEYSYM_LCTRL)
   || guac_rdp_keyboard_is_pressed(keyboard, GUAC_RDP_KEYSYM_RCTRL);

int alt_pressed =
      guac_rdp_keyboard_is_pressed(keyboard, GUAC_RDP_KEYSYM_LALT)
   || guac_rdp_keyboard_is_pressed(keyboard, GUAC_RDP_KEYSYM_RALT);

/* AltGr appears as Ctrl+Alt together; don’t hijack that. */
int altgr = ctrl_pressed && alt_pressed;

/* We only force scancodes for real shortcuts: Ctrl-only OR Alt-only. */
int shortcut_mod = !altgr && (ctrl_pressed || alt_pressed);

if (shortcut_mod) {
    /* Map both Latin and Russian keysyms for C/V to US scancodes.
     * US Set 1 scancodes: C=0x2E, V=0x2F.
     * This makes Ctrl+C / Ctrl+V work in ru-RU and en-US equally.
     */
    int sc = 0;

    switch (keysym) {
            /* --- Ctrl+C --- */
            case 'c': case 'C':
            case 0x441:  /* Cyrillic small 'с' */
            case 0x421:  /* Cyrillic capital 'С' */
                sc = 0x2E; /* US 'C' key */
                break;

            /* --- Ctrl+V --- */
            case 'v': case 'V':
            case 0x43C:  /* Cyrillic small 'м' */
            case 0x41C:  /* Cyrillic capital 'М' */
                sc = 0x2F; /* US 'V' key */
                break;

            default:
                sc = 0;
        }

    if (sc) {
        guac_client_log(client, GUAC_LOG_DEBUG,
            "Shortcut: modifiers active, sending US scancode 0x%X for keysym 0x%X",
            sc, keysym);
        /* press + release with no extended flags */
        guac_rdp_send_key_event(rdp_client, sc, 0, 1);
        guac_rdp_send_key_event(rdp_client, sc, 0, 0);
        return;
    }
}
        switch (keysym) {
            /* Lowercase Cyrillic */
            case 0x430: keysym = 'f'; break; // а
            case 0x431: keysym = ','; break; // б
            case 0x432: keysym = 'd'; break; // в
            case 0x433: keysym = 'u'; break; // г
            case 0x434: keysym = 'l'; break; // д
            case 0x435: keysym = 't'; break; // е
            case 0x436: keysym = ';'; break; // ж
            case 0x437: keysym = 'p'; break; // з
            case 0x438: keysym = 'b'; break; // и
            case 0x439: keysym = 'q'; break; // й
            case 0x43A: keysym = 'r'; break; // к
            case 0x43B: keysym = 'k'; break; // л
            case 0x43C: keysym = 'v'; break; // м
            case 0x43D: keysym = 'y'; break; // н
            case 0x43E: keysym = 'j'; break; // о
            case 0x43F: keysym = 'g'; break; // п
            case 0x440: keysym = 'h'; break; // р
            case 0x441: keysym = 'c'; break; // с
            case 0x442: keysym = 'n'; break; // т
            case 0x443: keysym = 'e'; break; // у
            case 0x444: keysym = 'a'; break; // ф
            case 0x445: keysym = '['; break; // х
            case 0x446: keysym = 'w'; break; // ц
            case 0x447: keysym = 'x'; break; // ч
            case 0x448: keysym = 'i'; break; // ш
            case 0x449: keysym = 'o'; break; // щ
            case 0x44A: keysym = ']'; break; // ъ
            case 0x44B: keysym = 's'; break; // ы
            case 0x44C: keysym = 'm'; break; // ь
            case 0x44D: keysym = '\''; break; // э
            case 0x44E: keysym = '.'; break; // ю
            case 0x44F: keysym = 'z'; break; // я

            /* Uppercase Cyrillic */
            case 0x410: keysym = 'F'; break; // А
            case 0x411: keysym = '<'; break; // Б
            case 0x412: keysym = 'D'; break; // В
            case 0x413: keysym = 'U'; break; // Г
            case 0x414: keysym = 'L'; break; // Д
            case 0x415: keysym = 'T'; break; // Е
            case 0x416: keysym = ':'; break; // Ж
            case 0x417: keysym = 'P'; break; // З
            case 0x418: keysym = 'B'; break; // И
            case 0x419: keysym = 'Q'; break; // Й
            case 0x41A: keysym = 'R'; break; // К
            case 0x41B: keysym = 'K'; break; // Л
            case 0x41C: keysym = 'V'; break; // М
            case 0x41D: keysym = 'Y'; break; // Н
            case 0x41E: keysym = 'J'; break; // О
            case 0x41F: keysym = 'G'; break; // П
            case 0x420: keysym = 'H'; break; // Р
            case 0x421: keysym = 'C'; break; // С
            case 0x422: keysym = 'N'; break; // Т
            case 0x423: keysym = 'E'; break; // У
            case 0x424: keysym = 'A'; break; // Ф
            case 0x425: keysym = '{'; break; // Х
            case 0x426: keysym = 'W'; break; // Ц
            case 0x427: keysym = 'X'; break; // Ч
            case 0x428: keysym = 'I'; break; // Ш
            case 0x429: keysym = 'O'; break; // Щ
            case 0x42A: keysym = '}'; break; // Ъ
            case 0x42B: keysym = 'S'; break; // Ы
            case 0x42C: keysym = 'M'; break; // Ь
            case 0x42D: keysym = '"'; break; // Э
            case 0x42E: keysym = '>'; break; // Ю
            case 0x42F: keysym = 'Z'; break; // Я
        }
    }

    if (ctrl_or_alt_pressed) {
        /* Map ASCII letters/digits to PC/AT set 1 scancodes (US layout).
         * This is enough for common shortcuts (A, C, V, X, Z, Y, ...). */
        int sc = 0;
           /* Normalize X11-style keysyms (0x1000000 + Unicode) to plain Unicode */
        if ((keysym & 0xFF000000) == 0x01000000)
            keysym &= 0x00FFFFFF;

        switch (keysym) {
            /* digits 0-9 */
            case '1': sc = 0x02; break; case '2': sc = 0x03; break;
            case '3': sc = 0x04; break; case '4': sc = 0x05; break;
            case '5': sc = 0x06; break; case '6': sc = 0x07; break;
            case '7': sc = 0x08; break; case '8': sc = 0x09; break;
            case '9': sc = 0x0A; break; case '0': sc = 0x0B; break;
            
            /* Latin letters A-Z (both cases) */
            case 'a': case 'A': sc = 0x1E; break;
            case 'b': case 'B': sc = 0x30; break;
            case 'c': case 'C': sc = 0x2E; break;
            case 'd': case 'D': sc = 0x20; break;
            case 'e': case 'E': sc = 0x12; break;
            case 'f': case 'F': sc = 0x21; break;
            case 'g': case 'G': sc = 0x22; break;
            case 'h': case 'H': sc = 0x23; break;
            case 'i': case 'I': sc = 0x17; break;
            case 'j': case 'J': sc = 0x24; break;
            case 'k': case 'K': sc = 0x25; break;
            case 'l': case 'L': sc = 0x26; break;
            case 'm': case 'M': sc = 0x32; break;
            case 'n': case 'N': sc = 0x31; break;
            case 'o': case 'O': sc = 0x18; break;
            case 'p': case 'P': sc = 0x19; break;
            case 'q': case 'Q': sc = 0x10; break;
            case 'r': case 'R': sc = 0x13; break;
            case 's': case 'S': sc = 0x1F; break;
            case 't': case 'T': sc = 0x14; break;
            case 'u': case 'U': sc = 0x16; break;
            case 'v': case 'V': sc = 0x2F; break;
            case 'w': case 'W': sc = 0x11; break;
            case 'x': case 'X': sc = 0x2D; break;
            case 'y': case 'Y': sc = 0x15; break;
            case 'z': case 'Z': sc = 0x2C; break;
            
            /* Russian Cyrillic letters - most common shortcuts */
            /* Lowercase Cyrillic */
            case 0x0444: case 0x0424: sc = 0x1E; break; /* ф/Ф -> A */
            case 0x0438: case 0x0418: sc = 0x30; break; /* и/И -> B */
            case 0x0441: case 0x0421: sc = 0x2E; break; /* с/С -> C */
            case 0x0432: case 0x0412: sc = 0x20; break; /* в/В -> D */
            case 0x0443: case 0x0423: sc = 0x12; break; /* у/У -> E */
            case 0x0430: case 0x0410: sc = 0x21; break; /* а/А -> F */
            case 0x043f: case 0x041f: sc = 0x22; break; /* п/П -> G */
            case 0x0440: case 0x0420: sc = 0x23; break; /* р/Р -> H */
            case 0x0448: case 0x0428: sc = 0x17; break; /* ш/Ш -> I */
            case 0x043e: case 0x041e: sc = 0x24; break; /* о/О -> J */
            case 0x043b: case 0x041b: sc = 0x25; break; /* л/Л -> K */
            case 0x0434: case 0x0414: sc = 0x26; break; /* д/Д -> L */
            case 0x044c: case 0x042c: sc = 0x32; break; /* ь/Ь -> M */
            case 0x0442: case 0x0422: sc = 0x31; break; /* т/Т -> N */
            case 0x0449: case 0x0429: sc = 0x18; break; /* щ/Щ -> O */
            case 0x0437: case 0x0417: sc = 0x19; break; /* з/З -> P */
            case 0x0439: case 0x0419: sc = 0x10; break; /* й/Й -> Q */
            case 0x043a: case 0x041a: sc = 0x13; break; /* к/К -> R */
            case 0x044b: case 0x042b: sc = 0x1F; break; /* ы/Ы -> S */
            case 0x0435: case 0x0415: sc = 0x14; break; /* е/Е -> T */
            case 0x0433: case 0x0413: sc = 0x16; break; /* г/Г -> U */
            case 0x043c: case 0x041c: sc = 0x2F; break; /* м/М -> V */
            case 0x0446: case 0x0426: sc = 0x11; break; /* ц/Ц -> W */
            case 0x0447: case 0x0427: sc = 0x2D; break; /* ч/Ч -> X */
            case 0x043d: case 0x041d: sc = 0x15; break; /* н/Н -> Y */
            case 0x044f: case 0x042f: sc = 0x2C; break; /* я/Я -> Z */ 
            
            
            default: sc = 0; break;
        }
        if (sc) {
            guac_client_log(client, GUAC_LOG_DEBUG,
                "Sending keysym 0x%x as US scancode 0x%X due to modifiers",
                keysym, sc);
            /* send press+release; 'flags' = 0 for regular keys */
            guac_rdp_send_key_event(rdp_client, sc, 0, 1);
            guac_rdp_send_key_event(rdp_client, sc, 0, 0);
            return;
        }
    }
    /* Attempt to type using dead keys */
    if (!guac_rdp_decompose_keysym(keyboard, keysym))
        return;

    guac_client_log(client, GUAC_LOG_DEBUG, "Sending keysym 0x%x as "
            "Unicode", keysym);

    /* Translate keysym into codepoint */
    int codepoint;
    if (keysym <= 0xFF)
        codepoint = keysym;
    else if (keysym >= 0x1000000)
        codepoint = keysym & 0xFFFFFF;
    else {
        guac_client_log(client, GUAC_LOG_DEBUG, "Unmapped keysym has no "
                "equivalent unicode value: 0x%x", keysym);
        return;
    }

    /* Send as Unicode event */
    guac_rdp_send_unicode_event(rdp_client, codepoint);

}

void guac_rdp_keyboard_update_locks(guac_rdp_keyboard* keyboard,
        unsigned int set_flags, unsigned int clear_flags) {

    guac_client* client = keyboard->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Calculate updated lock flags */
    unsigned int lock_flags = (keyboard->lock_flags | set_flags) & ~clear_flags;

    /* Synchronize remote side only if lock flags have changed */
    if (lock_flags != keyboard->lock_flags) {
        guac_rdp_send_synchronize_event(rdp_client, lock_flags);
        keyboard->lock_flags = lock_flags;
    }

}

void guac_rdp_keyboard_update_modifiers(guac_rdp_keyboard* keyboard,
        unsigned int set_flags, unsigned int clear_flags) {

    unsigned int modifier_flags = guac_rdp_keyboard_get_modifier_flags(keyboard);

    /* Only clear modifiers that are set */
    clear_flags &= modifier_flags;

    /* Only set modifiers that are currently cleared */
    set_flags &= ~modifier_flags;

    /* Press/release Shift as needed */
    if (set_flags & GUAC_RDP_KEYMAP_MODIFIER_SHIFT) {
        guac_rdp_keyboard_update_keysym(keyboard, GUAC_RDP_KEYSYM_LSHIFT, 1, GUAC_RDP_KEY_SOURCE_SYNTHETIC);
    }
    else if (clear_flags & GUAC_RDP_KEYMAP_MODIFIER_SHIFT) {
        guac_rdp_keyboard_update_keysym(keyboard, GUAC_RDP_KEYSYM_LSHIFT, 0, GUAC_RDP_KEY_SOURCE_SYNTHETIC);
        guac_rdp_keyboard_update_keysym(keyboard, GUAC_RDP_KEYSYM_RSHIFT, 0, GUAC_RDP_KEY_SOURCE_SYNTHETIC);
    }

    /* Press/release AltGr as needed */
    if (set_flags & GUAC_RDP_KEYMAP_MODIFIER_ALTGR) {
        guac_rdp_keyboard_update_keysym(keyboard, GUAC_RDP_KEYSYM_ALTGR, 1, GUAC_RDP_KEY_SOURCE_SYNTHETIC);
    }
    else if (clear_flags & GUAC_RDP_KEYMAP_MODIFIER_ALTGR) {
        guac_rdp_keyboard_update_keysym(keyboard, GUAC_RDP_KEYSYM_ALTGR, 0, GUAC_RDP_KEY_SOURCE_SYNTHETIC);
        guac_rdp_keyboard_update_keysym(keyboard, GUAC_RDP_KEYSYM_LALT, 0, GUAC_RDP_KEY_SOURCE_SYNTHETIC);
        guac_rdp_keyboard_update_keysym(keyboard, GUAC_RDP_KEYSYM_RALT, 0, GUAC_RDP_KEY_SOURCE_SYNTHETIC);
        guac_rdp_keyboard_update_keysym(keyboard, GUAC_RDP_KEYSYM_LCTRL, 0, GUAC_RDP_KEY_SOURCE_SYNTHETIC);
        guac_rdp_keyboard_update_keysym(keyboard, GUAC_RDP_KEYSYM_RCTRL, 0, GUAC_RDP_KEY_SOURCE_SYNTHETIC);
    }

}

int guac_rdp_keyboard_update_keysym(guac_rdp_keyboard* keyboard,
        int keysym, int pressed, guac_rdp_key_source source) {

    /* Synchronize lock keys states, if this has not yet been done */
    if (!keyboard->synchronized) {

        guac_client* client = keyboard->client;
        guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

        /* Synchronize remote lock key states with local state */
        guac_rdp_send_synchronize_event(rdp_client, keyboard->lock_flags);
        keyboard->synchronized = 1;

    }

    guac_rdp_key* key = guac_rdp_keyboard_get_key(keyboard, keysym);

    /* Update tracking of client-side keyboard state but only for keys which
     * are tracked server-side, as well (to ensure that the key count remains
     * correct, even if a user sends extra unbalanced or excessive press and
     * release events) */
    if (source == GUAC_RDP_KEY_SOURCE_CLIENT && key != NULL) {
        if (pressed && !key->user_pressed) {
            keyboard->user_pressed_keys++;
            key->user_pressed = 1;
        }
        else if (!pressed && key->user_pressed) {
            keyboard->user_pressed_keys--;
            key->user_pressed = 0;
        }
    }

    /* Send events and update server-side lock state only if server-side key
     * state is changing (or if server-side state of this key is untracked) */
    if (key == NULL || (pressed && key->pressed == NULL) || (!pressed && key->pressed != NULL)) {

        /* Toggle locks on keydown */
        if (pressed)
            keyboard->lock_flags ^= guac_rdp_keyboard_lock_flag(keysym);

        /* If key is known, update state and attempt to send using normal RDP key
         * events */
        const guac_rdp_keysym_desc* definition = NULL;
        if (key != NULL) {
            definition = guac_rdp_keyboard_send_defined_key(keyboard, key, pressed);
            key->pressed = pressed ? definition : NULL;
        }

        /* Fall back to dead keys or Unicode events if otherwise undefined inside
         * current keymap (note that we only handle "pressed" here, as neither
         * Unicode events nor dead keys can have a pressed/released state) */
        if (definition == NULL && pressed) {
            guac_rdp_keyboard_send_missing_key(keyboard, keysym);
        }

    }

    /* Reset RDP server keyboard state (releasing any automatically
     * pressed keys) once all keys have been released on the client
     * side */
    if (source == GUAC_RDP_KEY_SOURCE_CLIENT && keyboard->user_pressed_keys == 0)
        guac_rdp_keyboard_reset(keyboard);

    return 0;

}

void guac_rdp_keyboard_reset(guac_rdp_keyboard* keyboard) {

    /* Release all pressed keys */
    for (int i = 0; i < keyboard->num_keys; i++) {
        guac_rdp_key* key = &keyboard->keys[i];
        if (key->pressed != NULL)
            guac_rdp_keyboard_update_keysym(keyboard, key->pressed->keysym, 0,
                    GUAC_RDP_KEY_SOURCE_SYNTHETIC);
    }

}

BOOL guac_rdp_keyboard_set_indicators(rdpContext* context, UINT16 flags) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    guac_rwlock_acquire_read_lock(&(rdp_client->lock));

    /* Skip if keyboard not yet ready */
    guac_rdp_keyboard* keyboard = rdp_client->keyboard;
    if (keyboard == NULL)
        goto complete;

    /* Update with received locks */
    guac_client_log(client, GUAC_LOG_DEBUG, "Received updated keyboard lock flags from RDP server: 0x%X", flags);
    keyboard->lock_flags = flags;

complete:
    guac_rwlock_release_lock(&(rdp_client->lock));
    return TRUE;

}

