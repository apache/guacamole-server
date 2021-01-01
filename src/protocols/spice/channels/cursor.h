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

#ifndef GUAC_SPICE_CURSOR_H
#define GUAC_SPICE_CURSOR_H

#include "config.h"

#include <spice-client-glib-2.0/spice-client.h>

/**
 * The callback function that is executed when the cursor hide signal is
 * received from the Spice server.
 * 
 * @param channel
 *     The channel which received the cursor hide event.
 * 
 * @param client
 *     The guac_client associated with this Spice session.
 */
void guac_spice_cursor_hide(SpiceCursorChannel* channel, guac_client* client);

/**
 * The callback function that is executed when the cursor move signal is
 * received from the Spice server.
 * 
 * @param channel
 *     The channel that received the cursor move event.
 * 
 * @param x
 *     The x position of the cursor.
 * 
 * @param y
 *     The y position of the cursor.
 * 
 * @param client
 *     The guac_client associated with this Spice session.
 */
void guac_spice_cursor_move(SpiceCursorChannel* channel, int x, int y,
        guac_client* client);

/**
 * The callback function that is executed in response to the cursor reset
 * signal, resetting the cursor to the default context.
 * 
 * @param channel
 *     The channel that received the cursor reset signal.
 * 
 * @param client
 *     The guac_client associated with this Spice session.
 */
void guac_spice_cursor_reset(SpiceCursorChannel* channel, guac_client* client);

/**
 * The callback function that is executed in response to receiving the cursor
 * set signal from the Spice server, which sets the width, height, and image
 * of the cursor, and the x and y coordinates of the cursor hotspot.
 * 
 * @param channel
 *     The channel that received the cursor set signal.
 * 
 * @param width
 *     The width of the cursor image.
 * 
 * @param height
 *     The height of the cursor image.
 * 
 * @param x
 *     The x coordinate of the cursor hotspot.
 * 
 * @param y
 *     The y coordinate of the cursor hotspot.
 * 
 * @param rgba
 *     A pointer to the memory region containing the image data for the cursor,
 *     or NULL if the default cursor image should be used.
 * 
 * @param client
 *     The guac_client associated with this Spice session.
 */
void guac_spice_cursor_set(SpiceCursorChannel* channel, int width, int height,
        int x, int y, gpointer* rgba, guac_client* client);

#endif /* GUAC_SPICE_CURSOR_H */

