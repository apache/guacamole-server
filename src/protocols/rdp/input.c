/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"

#include "client.h"
#include "input.h"
#include "rdp.h"
#include "rdp_keymap.h"

#include <freerdp/freerdp.h>
#include <freerdp/input.h>
#include <guacamole/client.h>

#ifdef HAVE_FREERDP_DISPLAY_UPDATE_SUPPORT
#include "rdp_disp.h"
#endif

#include <pthread.h>
#include <stdlib.h>

int guac_rdp_send_keysym(guac_client* client, int keysym, int pressed) {

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    freerdp* rdp_inst = rdp_client->rdp_inst;

    /* Skip if not yet connected */
    if (rdp_inst == NULL)
        return 0;

    /* If keysym can be in lookup table */
    if (GUAC_RDP_KEYSYM_STORABLE(keysym)) {

        int pressed_flags;

        /* Look up scancode mapping */
        const guac_rdp_keysym_desc* keysym_desc =
            &GUAC_RDP_KEYSYM_LOOKUP(rdp_client->keymap, keysym);

        /* If defined, send event */
        if (keysym_desc->scancode != 0) {

            pthread_mutex_lock(&(rdp_client->rdp_lock));

            /* If defined, send any prerequesite keys that must be set */
            if (keysym_desc->set_keysyms != NULL)
                guac_rdp_update_keysyms(client, keysym_desc->set_keysyms, 0, 1);

            /* If defined, release any keys that must be cleared */
            if (keysym_desc->clear_keysyms != NULL)
                guac_rdp_update_keysyms(client, keysym_desc->clear_keysyms, 1, 0);

            /* Determine proper event flag for pressed state */
            if (pressed)
                pressed_flags = KBD_FLAGS_DOWN;
            else
                pressed_flags = KBD_FLAGS_RELEASE;

            /* Send actual key */
            rdp_inst->input->KeyboardEvent(rdp_inst->input, keysym_desc->flags | pressed_flags,
                    keysym_desc->scancode);

            /* If defined, release any keys that were originally released */
            if (keysym_desc->set_keysyms != NULL)
                guac_rdp_update_keysyms(client, keysym_desc->set_keysyms, 0, 0);

            /* If defined, send any keys that were originally set */
            if (keysym_desc->clear_keysyms != NULL)
                guac_rdp_update_keysyms(client, keysym_desc->clear_keysyms, 1, 1);

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

        /* Send Unicode event */
        rdp_inst->input->UnicodeKeyboardEvent(
                rdp_inst->input,
                0, codepoint);

        pthread_mutex_unlock(&(rdp_client->rdp_lock));

    }
    
    return 0;
}

void guac_rdp_update_keysyms(guac_client* client, const int* keysym_string, int from, int to) {

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    int keysym;

    /* Send all keysyms in string, NULL terminated */
    while ((keysym = *keysym_string) != 0) {

        /* Get current keysym state */
        int current_state = GUAC_RDP_KEYSYM_LOOKUP(rdp_client->keysym_state, keysym);

        /* If key is currently in given state, send event for changing it to specified "to" state */
        if (current_state == from)
            guac_rdp_send_keysym(client, *keysym_string, to);

        /* Next keysym */
        keysym_string++;

    }

}

int guac_rdp_user_mouse_handler(guac_user* user, int x, int y, int mask) {

    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    freerdp* rdp_inst = rdp_client->rdp_inst;

    /* Store current mouse location */
    guac_common_cursor_move(rdp_client->display->cursor, user, x, y);

    /* Skip if not yet connected */
    if (rdp_inst == NULL)
        return 0;

    pthread_mutex_lock(&(rdp_client->rdp_lock));

    /* If button mask unchanged, just send move event */
    if (mask == rdp_client->mouse_button_mask)
        rdp_inst->input->MouseEvent(rdp_inst->input, PTR_FLAGS_MOVE, x, y);

    /* Otherwise, send events describing button change */
    else {

        /* Mouse buttons which have JUST become released */
        int released_mask =  rdp_client->mouse_button_mask & ~mask;

        /* Mouse buttons which have JUST become pressed */
        int pressed_mask  = ~rdp_client->mouse_button_mask &  mask;

        /* Release event */
        if (released_mask & 0x07) {

            /* Calculate flags */
            int flags = 0;
            if (released_mask & 0x01) flags |= PTR_FLAGS_BUTTON1;
            if (released_mask & 0x02) flags |= PTR_FLAGS_BUTTON3;
            if (released_mask & 0x04) flags |= PTR_FLAGS_BUTTON2;

            rdp_inst->input->MouseEvent(rdp_inst->input, flags, x, y);

        }

        /* Press event */
        if (pressed_mask & 0x07) {

            /* Calculate flags */
            int flags = PTR_FLAGS_DOWN;
            if (pressed_mask & 0x01) flags |= PTR_FLAGS_BUTTON1;
            if (pressed_mask & 0x02) flags |= PTR_FLAGS_BUTTON3;
            if (pressed_mask & 0x04) flags |= PTR_FLAGS_BUTTON2;
            if (pressed_mask & 0x08) flags |= PTR_FLAGS_WHEEL | 0x78;
            if (pressed_mask & 0x10) flags |= PTR_FLAGS_WHEEL | PTR_FLAGS_WHEEL_NEGATIVE | 0x88;

            /* Send event */
            rdp_inst->input->MouseEvent(rdp_inst->input, flags, x, y);

        }

        /* Scroll event */
        if (pressed_mask & 0x18) {

            /* Down */
            if (pressed_mask & 0x08)
                rdp_inst->input->MouseEvent(
                        rdp_inst->input,
                        PTR_FLAGS_WHEEL | 0x78,
                        x, y);

            /* Up */
            if (pressed_mask & 0x10)
                rdp_inst->input->MouseEvent(
                        rdp_inst->input,
                        PTR_FLAGS_WHEEL | PTR_FLAGS_WHEEL_NEGATIVE | 0x88,
                        x, y);

        }

        rdp_client->mouse_button_mask = mask;
    }

    pthread_mutex_unlock(&(rdp_client->rdp_lock));

    return 0;
}

int guac_rdp_user_key_handler(guac_user* user, int keysym, int pressed) {

    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Update keysym state */
    if (GUAC_RDP_KEYSYM_STORABLE(keysym))
        GUAC_RDP_KEYSYM_LOOKUP(rdp_client->keysym_state, keysym) = pressed;

    return guac_rdp_send_keysym(client, keysym, pressed);

}

int guac_rdp_user_size_handler(guac_user* user, int width, int height) {

#ifdef HAVE_FREERDP_DISPLAY_UPDATE_SUPPORT
    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    freerdp* rdp_inst = rdp_client->rdp_inst;

    /* Skip if not yet connected */
    if (rdp_inst == NULL)
        return 0;

    /* Convert client pixels to remote pixels */
    width  = width  * rdp_client->settings->resolution
                    / user->info.optimal_resolution;

    height = height * rdp_client->settings->resolution
                    / user->info.optimal_resolution;

    /* Send display update */
    pthread_mutex_lock(&(rdp_client->rdp_lock));
    guac_rdp_disp_set_size(rdp_client->disp, rdp_inst->context, width, height);
    pthread_mutex_unlock(&(rdp_client->rdp_lock));
#endif

    return 0;

}

