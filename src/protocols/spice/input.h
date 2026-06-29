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

#endif
