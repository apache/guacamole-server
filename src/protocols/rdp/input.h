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

#ifndef GUAC_RDP_INPUT_H
#define GUAC_RDP_INPUT_H

#include <guacamole/client.h>
#include <guacamole/user.h>

/**
 * Presses or releases the given keysym, sending an appropriate set of key
 * events to the RDP server. The key events sent will depend on the current
 * keymap.
 *
 * @param client The guac_client associated with the current RDP session.
 * @param keysym The keysym being pressed or released.
 * @param pressed Zero if the keysym is being released, non-zero otherwise.
 * @return Zero if the keys were successfully sent, non-zero otherwise.
 */
int guac_rdp_send_keysym(guac_client* client, int keysym, int pressed);

/**
 * For every keysym in the given NULL-terminated array of keysyms, update
 * the current state of that key conditionally. For each key in the "from"
 * state (0 being released and 1 being pressed), that key will be updated
 * to the "to" state.
 *
 * @param client The guac_client associated with the current RDP session.
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
void guac_rdp_update_keysyms(guac_client* client, const int* keysym_string,
        int from, int to);

/**
 * Handler for Guacamole user mouse events.
 */
guac_user_mouse_handler guac_rdp_user_mouse_handler;

/**
 * Handler for Guacamole user key events.
 */
guac_user_key_handler guac_rdp_user_key_handler;

/**
 * Handler for Guacamole user size events.
 */
guac_user_size_handler guac_rdp_user_size_handler;

#endif

