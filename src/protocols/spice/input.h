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

#ifndef GUAC_SPICE_INPUT_H
#define GUAC_SPICE_INPUT_H

#include <guacamole/user.h>
#include <spice-client.h>

/**
 * Handler for Guacamole mouse events, translating them into SPICE pointer
 * motion and button events.
 */
guac_user_mouse_handler guac_spice_user_mouse_handler;

/**
 * Handler for Guacamole key events, translating X11 keysyms into PC scancodes
 * and sending them via the SPICE inputs channel.
 */
guac_user_key_handler guac_spice_user_key_handler;

/**
 * Handler for Guacamole size events, requesting that the SPICE guest resize its
 * primary display to match the connected client's resolution (unless display
 * resize has been disabled).
 */
guac_user_size_handler guac_spice_user_size_handler;

/**
 * Attempts to send any queued guest display-resize request, provided the guest
 * is ready (agent connected with monitors-config support and the display's
 * primary surface created). Must be called from the SPICE event-loop thread.
 * Invoked both when a resize is queued and when a readiness condition changes.
 *
 * @param client
 *     The guac_client associated with the SPICE connection.
 */
void guac_spice_resize_try(guac_client* client);

/**
 * Signal handler for the SPICE main channel "notify::agent-connected" event.
 * Updates display-resize readiness based on the agent's connection state and
 * monitors-config capability, then flushes any queued resize.
 */
void guac_spice_resize_agent_update(SpiceMainChannel* channel,
        GParamSpec* pspec, guac_client* client);

/**
 * Signal handler for the SPICE main channel "main-agent-update" event, invoked
 * when the guest agent's state or capabilities change (the monitors-config
 * capability is announced shortly after the agent connects). Re-evaluates
 * display-resize readiness and flushes any queued resize.
 */
void guac_spice_resize_agent_updated(SpiceMainChannel* channel,
        guac_client* client);

#endif
