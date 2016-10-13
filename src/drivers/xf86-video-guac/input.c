
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

#include "config.h"
#include "input.h"
#include "io.h"
#include "log.h"

#include <xorg-server.h>
#include <xf86.h>
#include <xf86_OSproc.h>
#include <xf86Xinput.h>
#include <xserver-properties.h>
#include <exevents.h>

#include <guacamole/error.h>
#include <guacamole/client.h>

#include <unistd.h>

InputDriverRec GUAC_INPUT = {
    .driverVersion = GUAC_DRV_VERSION,
    .driverName = GUAC_DRV_NAME,
    .Identify = NULL,
    .PreInit = guac_input_pre_init,
    .UnInit = NULL,
    .module = NULL,
    .default_options = NULL
};

InputInfoPtr GUAC_DRV_INPUT_DEVICE = NULL;

int GUAC_DRV_INPUT_READ_FD = -1;

int GUAC_DRV_INPUT_WRITE_FD = -1;

int guac_input_pre_init(InputDriverPtr driver, InputInfoPtr info,
        int flags) {

    int pipe_fd[2];

    /* Store device */
    GUAC_DRV_INPUT_DEVICE = info;
    xf86Msg(X_INFO, "guac: init input device\n");

    if (pipe(pipe_fd)) {
        xf86Msg(X_ERROR, "guac: cannot create event pipe\n");
        return BadAlloc;
    }

    /* Store pipe file descriptors */
    GUAC_DRV_INPUT_READ_FD = pipe_fd[0];
    GUAC_DRV_INPUT_WRITE_FD = pipe_fd[1];

    /* Init input info */
    info->private = NULL;
    info->type_name = "UNKNOWN";
    info->device_control = guac_input_device_control;
    info->read_input = guac_input_read_input;
    info->switch_mode = NULL;
    info->fd = GUAC_DRV_INPUT_READ_FD;

    return Success;
}

int guac_input_device_control(DeviceIntPtr device, int what) {

    InputInfoPtr info = device->public.devicePrivate;

    /* Initialize device */
    if (what == DEVICE_INIT) {

        BYTE map[GUAC_DRV_INPUT_BUTTONS+1] = {0, 1, 2, 3, 4, 5};
        Atom button_labels[GUAC_DRV_INPUT_BUTTONS];
        Atom axis_labels[2];

        /* Build list of button labels */
        button_labels[0] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_LEFT);
        button_labels[1] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_MIDDLE);
        button_labels[2] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_RIGHT);
        button_labels[3] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_UP);
        button_labels[4] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_DOWN);

        /* Build list of axis labels */
        axis_labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_X);
        axis_labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_Y);

        /* Init pointer device */
        InitPointerDeviceStruct((DevicePtr) device, map,
                GUAC_DRV_INPUT_BUTTONS, button_labels,
                (PtrCtrlProcPtr)NoopDDA, GetMotionHistorySize(),
                2, axis_labels);

        /* Init keyboard device */
        InitKeyboardDeviceStruct(device, NULL, NULL, NULL);

    }

    /* Enable device */
    else if (what == DEVICE_ON) {
        xf86AddEnabledDevice(info);
        device->public.on = TRUE;
    }

    /* Disable device */
    else if (what == DEVICE_OFF)
        device->public.on = FALSE;

    return Success;
}

void guac_input_read_input(InputInfoPtr info) {

    /* Wait for data */
    while (xf86WaitForInput(GUAC_DRV_INPUT_READ_FD, 0) > 0) {

        /* Read event */
        guac_drv_input_event event;
        guac_drv_read(GUAC_DRV_INPUT_READ_FD, &event, sizeof(event));

        /* Handle mouse events */
        if (event.type == GUAC_DRV_INPUT_EVENT_MOUSE) {

            int i;
            int mask = event.data.mouse.mask;
            int change = event.data.mouse.change_mask;

            /* Send motion event */
            xf86PostMotionEvent(GUAC_DRV_INPUT_DEVICE->dev, 1, 0, 2,
                    event.data.mouse.x, event.data.mouse.y);

            /* Send button event(s) */
            for (i=0; i<5; i++) {

                /* If changed, send button event */
                if (change & 0x1)
                    xf86PostButtonEvent(GUAC_DRV_INPUT_DEVICE->dev, 0, i+1,
                            mask & 0x1, 0, 0);

                change >>= 1;
                mask >>= 1;
            }

        } /* end if mouse event */

        /* Handle keyboard events */
        else if (event.type == GUAC_DRV_INPUT_EVENT_KEYBOARD) {

            /* STUB */
            xf86Msg(X_INFO, "guac: STUB: keysym=%#x pressed=%i\n",
                    event.data.keyboard.keysym, event.data.keyboard.pressed);

            /* MEGA STUB - Send the 'a' key */
            xf86PostKeyboardEvent(GUAC_DRV_INPUT_DEVICE->dev, 38,
                    event.data.keyboard.pressed);

        } /* end if keyboard event */

    } /* end while events available */

}

