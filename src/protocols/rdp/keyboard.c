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

#include "config.h"

#include "client.h"
#include "keyboard.h"
#include "rdp.h"
#include "rdp_keymap.h"

#include <freerdp/freerdp.h>
#include <freerdp/input.h>
#include <guacamole/client.h>

#include <pthread.h>
#include <stdlib.h>

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

        /* Copy mapping */
        GUAC_RDP_KEYSYM_LOOKUP(keyboard->keymap, mapping->keysym) = *mapping;

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

int guac_rdp_keyboard_send_event(guac_rdp_keyboard* keyboard,
        int keysym, int pressed) {

    guac_client* client = keyboard->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* If keysym can be in lookup table */
    if (GUAC_RDP_KEYSYM_STORABLE(keysym)) {

        /* Look up scancode mapping */
        const guac_rdp_keysym_desc* keysym_desc =
            &GUAC_RDP_KEYSYM_LOOKUP(keyboard->keymap, keysym);

        /* If defined, send event */
        if (keysym_desc->scancode != 0) {

            pthread_mutex_lock(&(rdp_client->rdp_lock));

            /* If defined, send any prerequesite keys that must be set */
            if (keysym_desc->set_keysyms != NULL)
                guac_rdp_keyboard_send_events(keyboard,
                        keysym_desc->set_keysyms, 0, 1);

            /* If defined, release any keys that must be cleared */
            if (keysym_desc->clear_keysyms != NULL)
                guac_rdp_keyboard_send_events(keyboard,
                        keysym_desc->clear_keysyms, 1, 0);

            /* Determine proper event flag for pressed state */
            int pressed_flags;
            if (pressed)
                pressed_flags = KBD_FLAGS_DOWN;
            else
                pressed_flags = KBD_FLAGS_RELEASE;

            /* Skip if not yet connected */
            freerdp* rdp_inst = rdp_client->rdp_inst;
            if (rdp_inst == NULL) {
                pthread_mutex_unlock(&(rdp_client->rdp_lock));
                return 0;
            }

            /* Send actual key */
            rdp_inst->input->KeyboardEvent(rdp_inst->input,
                    keysym_desc->flags | pressed_flags,
                    keysym_desc->scancode);

            /* If defined, release any keys that were originally released */
            if (keysym_desc->set_keysyms != NULL)
                guac_rdp_keyboard_send_events(keyboard,
                        keysym_desc->set_keysyms, 0, 0);

            /* If defined, send any keys that were originally set */
            if (keysym_desc->clear_keysyms != NULL)
                guac_rdp_keyboard_send_events(keyboard,
                        keysym_desc->clear_keysyms, 1, 1);

            pthread_mutex_unlock(&(rdp_client->rdp_lock));

            return 0;

        }
    }

    /* Fall back to unicode events if undefined inside current keymap */

    /* Only send when key pressed - Unicode events do not have
     * DOWN/RELEASE flags */
    if (pressed) {

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

        pthread_mutex_lock(&(rdp_client->rdp_lock));

        /* Skip if not yet connected */
        freerdp* rdp_inst = rdp_client->rdp_inst;
        if (rdp_inst == NULL) {
            pthread_mutex_unlock(&(rdp_client->rdp_lock));
            return 0;
        }

        /* Send Unicode event */
        rdp_inst->input->UnicodeKeyboardEvent(
                rdp_inst->input,
                0, codepoint);

        pthread_mutex_unlock(&(rdp_client->rdp_lock));

    }
    
    return 0;
}

void guac_rdp_keyboard_send_events(guac_rdp_keyboard* keyboard,
        const int* keysym_string, int from, int to) {

    int keysym;

    /* Send all keysyms in string, NULL terminated */
    while ((keysym = *keysym_string) != 0) {

        /* Get current keysym state */
        int current_state =
            GUAC_RDP_KEYSYM_LOOKUP(keyboard->keysym_state, keysym);

        /* If key is currently in given state, send event for changing it to
         * specified "to" state */
        if (current_state == from)
            guac_rdp_keyboard_send_event(keyboard, *keysym_string, to);

        /* Next keysym */
        keysym_string++;

    }

}

int guac_rdp_keyboard_update_keysym(guac_rdp_keyboard* keyboard,
        int keysym, int pressed) {

    /* Update keysym state */
    if (GUAC_RDP_KEYSYM_STORABLE(keysym))
        GUAC_RDP_KEYSYM_LOOKUP(keyboard->keysym_state, keysym) = pressed;

    return guac_rdp_keyboard_send_event(keyboard, keysym, pressed);

}

