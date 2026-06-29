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

#ifndef GUAC_SPICE_KEYMAP_H
#define GUAC_SPICE_KEYMAP_H

/**
 * Translates the given X11 keysym into the corresponding PC (AT set 1 / "XT")
 * scancode expected by the SPICE inputs channel. Extended scancodes (those
 * which are transmitted on the wire with a leading 0xE0 byte) are returned
 * with bit 8 (0x100) set, matching the encoding expected by
 * spice_inputs_channel_key_press() / spice_inputs_channel_key_release().
 *
 * @param keysym
 *     The X11 keysym to translate.
 *
 * @return
 *     The corresponding PC scancode (with bit 0x100 set for extended keys), or
 *     0 if the keysym has no known scancode mapping.
 */
unsigned int guac_spice_keysym_to_scancode(int keysym);

#endif
