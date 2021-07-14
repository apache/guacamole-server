
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
#include "drv.h"
#include "input.h"
#include "screen.h"

#include <xorg-server.h>
#include <xf86.h>
#include <xf86str.h>

/*
 * Driver function to report back to respond to X probing the driver.
 */
static Bool guac_drv_driver_func(ScrnInfoPtr, xorgDriverFuncOp, pointer);

_X_EXPORT DriverRec GUAC = {

    .driverVersion = GUAC_DRV_VERSION,
    .driverName = GUAC_DRV_NAME,

    .Identify = guac_drv_identify,
    .Probe = guac_drv_probe,
    .AvailableOptions = guac_drv_available_options,

    .module = NULL,
    .refCount = 0,
    .driverFunc = guac_drv_driver_func

};

/*
 * Options.
 */

const OptionInfoRec GUAC_OPTIONS[GUAC_DRV_OPTIONINFOREC_SIZE] = {

    /* Listen host/address */
    {
        GUAC_DRV_OPTION_LISTEN_ADDRESS,
        "ListenAddress", OPTV_STRING,  { .str = NULL }, FALSE
    },

    /* Listen port */
    {
        GUAC_DRV_OPTION_LISTEN_PORT,
        "ListenPort", OPTV_STRING, { .str = "4823" }, FALSE
    },

    /* PulseAudio server name */
    {
        GUAC_DRV_OPTION_PULSE_AUDIO_SERVER_NAME,
        "PulseAudioServerName", OPTV_STRING, { .str = NULL }, FALSE
    },

    /* Driver log level */
    {
        GUAC_DRV_OPTION_LOG_LEVEL,
        "LogLevel", OPTV_STRING, { .str = "info" }, FALSE
    },

    /* End of options */
    { -1, NULL, OPTV_NONE, {0}, FALSE }

};

/*
 * Version information.
 */

static XF86ModuleVersionInfo guac_drv_version_info = {
    .modname = GUAC_DRV_NAME,
    .vendor = GUAC_DRV_VENDOR,
    ._modinfo1_ = MODINFOSTRING1,
    ._modinfo2_ = MODINFOSTRING2,
    .xf86version = XORG_VERSION_CURRENT,
    .majorversion = GUAC_MAJOR,
    .minorversion = GUAC_MINOR,
    .patchlevel   = GUAC_PATCH,
    .abiclass = ABI_CLASS_VIDEODRV,
    .abiversion = ABI_VIDEODRV_VERSION,
    .moduleclass = MOD_CLASS_VIDEODRV,
    .checksum = {0, 0, 0, 0}
};

/*
 * Module data.
 */

_X_EXPORT XF86ModuleData guacModuleData = {
    .vers = &guac_drv_version_info,
    .setup = guac_drv_setup,
    .teardown = NULL
};

void guac_drv_identify(int flags) {
    xf86Msg(X_INFO, GUAC_DRV_NAME " version " PACKAGE_VERSION "\n");
    xf86Msg(X_INFO, GUAC_DRV_NAME " Guacamole protocol video driver\n");
}

const OptionInfoRec* guac_drv_available_options(int chipid, int busid) {
    return GUAC_OPTIONS;
}

pointer guac_drv_setup(pointer module, pointer opts,
        int* errmaj, int* errmin) {

    static Bool setup_complete = FALSE;

    /* Ensure setup only runs once */
    if (!setup_complete) {
        xf86AddDriver(&GUAC, module, HaveDriverFuncs);
        xf86AddInputDriver(&GUAC_INPUT, module, 0);
        setup_complete = TRUE;
        return (pointer) 1;
    }

    /* Return error if run more than once */
    if (errmaj)
        *errmaj = LDR_ONCEONLY;

    return NULL;

}

Bool guac_drv_probe(DriverPtr drv, int flags) {

    int i;
    int num_sections;
    GDevPtr* device_sections;
    Bool screen_found;

    if (flags & PROBE_DETECT)
        return FALSE;

    /* Find device sections, stop if none */
    num_sections = xf86MatchDevice(GUAC_DRV_NAME, &device_sections);
    if (num_sections <= 0)
        return FALSE;

    /* For each device section */
    for (i=0; i<num_sections; i++) {

        ScrnInfoPtr screen;

        /* Get entity index */
        int entity_index = xf86ClaimNoSlot(drv, 0, device_sections[i], TRUE);

        /* Allocate screen */
        if ((screen = xf86AllocateScreen(drv, 0))) {

            /* Add entity to screen */
            xf86AddEntityToScreen(screen, entity_index);

            /* Set info */
            screen->driverVersion = GUAC_DRV_VERSION;
            screen->driverName = GUAC_DRV_NAME;
            screen->name = GUAC_DRV_NAME;

            /* Set handlers */
            screen->Probe       = guac_drv_probe;
            screen->PreInit     = guac_drv_pre_init;
            screen->ScreenInit  = guac_drv_screen_init;
            screen->SwitchMode  = guac_drv_switch_mode;
            screen->AdjustFrame = guac_drv_adjust_frame;
            screen->EnterVT     = guac_drv_enter_vt;
            screen->LeaveVT     = guac_drv_leave_vt;
            screen->FreeScreen  = guac_drv_free_screen;
            screen->ValidMode   = guac_drv_valid_mode;

            /* At least one screen was found */
            screen_found = TRUE;

        }

    } /* end for each device */

    /* Return whether screen was found */
    return screen_found;

}

/*
 * When X checks with this driver what devices are needed this will report back
 * to HW_SKIP_CONSOLE so X does not require being connected to a virtual
 * console.
 */
static Bool guac_drv_driver_func(ScrnInfoPtr p_scrn, xorgDriverFuncOp op,
            pointer ptr) {

    CARD32 *flag;
    switch (op) {
        case GET_REQUIRED_HW_INTERFACES:
            flag = (CARD32*) ptr;
            (*flag) = HW_SKIP_CONSOLE;
            return TRUE;
        default:
            return FALSE;
    }
}
