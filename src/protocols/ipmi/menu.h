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

#ifndef GUAC_IPMI_MENU_H
#define GUAC_IPMI_MENU_H

#include <guacamole/client.h>

#include <stdbool.h>

/**
 * The keysym (Ctrl + ']') which opens the in-terminal IPMI control menu. This
 * mirrors the classic Telnet escape character and is unlikely to be needed by
 * a serial console.
 */
#define GUAC_IPMI_MENU_KEYSYM 0x5D

/**
 * Opens the in-terminal IPMI control menu, rendering it into the terminal and
 * marking the menu as open within the client's data. While open, keystrokes
 * should be routed to guac_ipmi_menu_handle_key() rather than forwarded to the
 * serial console.
 *
 * @param client
 *     The guac_client associated with the IPMI connection.
 */
void guac_ipmi_menu_open(guac_client* client);

/**
 * Handles a keystroke received while the control menu is open, performing the
 * selected action (power control, status, identify, etc.) and updating the
 * menu state. When a selection completes or the menu is dismissed, the menu is
 * closed.
 *
 * @param client
 *     The guac_client associated with the IPMI connection.
 *
 * @param keysym
 *     The X11 keysym of the pressed key.
 */
void guac_ipmi_menu_handle_key(guac_client* client, int keysym);

#endif
