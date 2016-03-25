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

#ifndef GUAC_RDP_INPUT_H
#define GUAC_RDP_INPUT_H

#include <guacamole/client.h>
#include <guacamole/user.h>

/**
 * Presses or releases the given keysym, sending an appropriate set of key
 * events to the RDP server. The key events sent will depend on the current
 * keymap.
 *
 * @param client
 *     The guac_client associated with the current RDP session.
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
int guac_rdp_send_keysym(guac_client* client, int keysym, int pressed);

/**
 * For every keysym in the given NULL-terminated array of keysyms, update
 * the current state of that key conditionally. For each key in the "from"
 * state (0 being released and 1 being pressed), that key will be updated
 * to the "to" state.
 *
 * @param client
 *     The guac_client associated with the current RDP session.
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

