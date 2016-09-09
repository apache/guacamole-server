
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

#ifndef __GUAC_INPUT_H
#define __GUAC_INPUT_H

#include "config.h"
#include "guac_drv.h"

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
 * Statically-stored input device.
 */
extern InputInfoPtr GUAC_DRV_INPUT_DEVICE;

/**
 * The file descriptor to read input events from.
 */
extern int GUAC_DRV_INPUT_READ_FD;

/**
 * The file descriptor to write input events to.
 */
extern int GUAC_DRV_INPUT_WRITE_FD;

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
 * Called by Xorg to initialize the input driver.
 */
int guac_input_pre_init(InputDriverPtr driver, InputInfoPtr info, int flags);

/**
 * Called by Xorg to enable/disable the device.
 */
int guac_input_device_control(DeviceIntPtr device, int onoff);

/**
 * Called by Xorg when there is data to be read.
 */
void guac_input_read_input(InputInfoPtr info);

#endif

