
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

#ifndef GUAC_DRV_INPUT_H
#define GUAC_DRV_INPUT_H

#include "config.h"
#include "drv.h"

#include <xorg-server.h>
#include <xf86.h>
#include <xf86Xinput.h>

#include <guacamole/client.h>

/**
 * The number of possible mouse buttons.
 */
#define GUAC_DRV_INPUT_BUTTONS 5

/**
 * Input driver record.
 */
extern InputDriverRec GUAC_INPUT;

/**
 * All possible event types.
 */
typedef enum guac_drv_input_event_type {

    /**
     * Mouse event.
     */
    GUAC_DRV_INPUT_EVENT_MOUSE,

    /**
     * Keyboard event.
     */
    GUAC_DRV_INPUT_EVENT_KEYBOARD,

} guac_drv_input_event_type;

/**
 * Mouse event packet.
 */
typedef struct guac_drv_input_mouse_event {

    /**
     * Current button mask.
     */
    int mask;

    /**
     * Mask describing which buttons changed.
     */
    int change_mask;

    /**
     * The X coordinate of the mouse event.
     */
    int x;

    /**
     * The Y cordinate of the mouse event.
     */
    int y;

} guac_drv_input_mouse_event;


/**
 * Keyboard event packet.
 */
typedef struct guac_drv_input_keyboard_event {

    /**
     * Whether the key is pressed.
     */
    int pressed;

    /**
     * The keysem of the key which was pressed or released.
     */
    int keysym;

} guac_drv_input_keyboard_event;

/**
 * Generic event packet, which can be either mouse or keyboard.
 */
typedef struct guac_drv_input_event {

    /**
     * The type of this event.
     */
    guac_drv_input_event_type type;

    /**
     * Data specific to the type of event.
     */
    union {

        /**
         * Keyboard-specific event data.
         */
        guac_drv_input_keyboard_event keyboard;

        /**
         * Mouse-specific event data.
         */
        guac_drv_input_mouse_event mouse;

    } data;

} guac_drv_input_event;

/**
 * The current state of the Guacamole X.Org driver input device.
 */
typedef struct guac_drv_input_device {

    /**
     * The file descriptor from which guac_drv_input_event structures should be
     * read when processing previously-signalled input events.
     */
    int read_fd;

    /**
     * The file descriptor to which guac_drv_input_event structures should be
     * written to signal a new input event.
     */
    int write_fd;

    /**
     * The X coordinate of the last mouse event.
     */
    int mouse_x;

    /**
     * The Y coordinate of the last mouse event.
     */
    int mouse_y;

} guac_drv_input_device;

/**
 * Sends the given event along the file descriptor used by the Guacamole X.Org
 * input driver. If the X.Org server has not yet finished initializing, and the
 * file descriptor is not yet defined, this function has no effect.
 *
 * @param event
 *     The input event to send.
 */
void guac_drv_input_send_event(guac_drv_input_event* event);

#endif

