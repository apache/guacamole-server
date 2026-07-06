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

#include "chassis.h"
#include "ipmi.h"
#include "settings.h"

#include <guacamole/client.h>

#include <freeipmi/freeipmi.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/**
 * The IPMI session timeout, in milliseconds, used for the short-lived
 * out-of-band sessions opened to issue chassis commands. Chassis commands are
 * simple request/response operations, so a bounded timeout (rather than the
 * library default) is used to limit how long a chassis command can stall when
 * the BMC is slow or unreachable. This matters because chassis commands
 * triggered from the in-terminal control menu run synchronously within the
 * user input handler.
 */
#define GUAC_IPMI_CHASSIS_SESSION_TIMEOUT 8000

/**
 * The IPMI packet retransmission timeout, in milliseconds, used for chassis
 * command sessions.
 */
#define GUAC_IPMI_CHASSIS_RETRANSMISSION_TIMEOUT 1000

/**
 * Maps a guac_ipmi_privilege_level to the corresponding libfreeipmi
 * IPMI_PRIVILEGE_LEVEL_* constant.
 *
 * @param privilege_level
 *     The privilege level to map.
 *
 * @return
 *     The equivalent IPMI_PRIVILEGE_LEVEL_* constant.
 */
static uint8_t guac_ipmi_map_privilege_level(
        guac_ipmi_privilege_level privilege_level) {

    switch (privilege_level) {
        case GUAC_IPMI_PRIVILEGE_USER:
            return IPMI_PRIVILEGE_LEVEL_USER;
        case GUAC_IPMI_PRIVILEGE_OPERATOR:
            return IPMI_PRIVILEGE_LEVEL_OPERATOR;
        case GUAC_IPMI_PRIVILEGE_ADMIN:
        default:
            return IPMI_PRIVILEGE_LEVEL_ADMIN;
    }

}

/**
 * Maps a guac_ipmi_power_action to the corresponding libfreeipmi
 * IPMI_CHASSIS_CONTROL_* constant.
 *
 * @param action
 *     The power action to map. Must not be GUAC_IPMI_POWER_NONE.
 *
 * @return
 *     The equivalent IPMI_CHASSIS_CONTROL_* constant.
 */
static uint8_t guac_ipmi_map_power_action(guac_ipmi_power_action action) {

    switch (action) {
        case GUAC_IPMI_POWER_ON:
            return IPMI_CHASSIS_CONTROL_POWER_UP;
        case GUAC_IPMI_POWER_OFF:
            return IPMI_CHASSIS_CONTROL_POWER_DOWN;
        case GUAC_IPMI_POWER_CYCLE:
            return IPMI_CHASSIS_CONTROL_POWER_CYCLE;
        case GUAC_IPMI_POWER_RESET:
            return IPMI_CHASSIS_CONTROL_HARD_RESET;
        case GUAC_IPMI_POWER_SOFT_SHUTDOWN:
            return IPMI_CHASSIS_CONTROL_INITIATE_SOFT_SHUTDOWN;
        case GUAC_IPMI_POWER_DIAGNOSTIC_INTERRUPT:
            return IPMI_CHASSIS_CONTROL_PULSE_DIAGNOSTIC_INTERRUPT;
        default:
            return IPMI_CHASSIS_CONTROL_POWER_DOWN;
    }

}

/**
 * Maps a guac_ipmi_boot_device to the corresponding libfreeipmi
 * IPMI_SYSTEM_BOOT_OPTION_BOOT_FLAG_BOOT_DEVICE_FORCE_* constant.
 *
 * @param device
 *     The boot device to map. Must not be GUAC_IPMI_BOOT_NONE.
 *
 * @return
 *     The equivalent boot device constant.
 */
static uint8_t guac_ipmi_map_boot_device(guac_ipmi_boot_device device) {

    switch (device) {
        case GUAC_IPMI_BOOT_PXE:
            return IPMI_SYSTEM_BOOT_OPTION_BOOT_FLAG_BOOT_DEVICE_FORCE_PXE;
        case GUAC_IPMI_BOOT_DISK:
            return IPMI_SYSTEM_BOOT_OPTION_BOOT_FLAG_BOOT_DEVICE_FORCE_HARD_DRIVE;
        case GUAC_IPMI_BOOT_CDROM:
            return IPMI_SYSTEM_BOOT_OPTION_BOOT_FLAG_BOOT_DEVICE_FORCE_CD_DVD;
        case GUAC_IPMI_BOOT_BIOS:
            return IPMI_SYSTEM_BOOT_OPTION_BOOT_FLAG_BOOT_DEVICE_FORCE_BIOS_SETUP;
        default:
            return IPMI_SYSTEM_BOOT_OPTION_BOOT_FLAG_BOOT_DEVICE_NO_OVERRIDE;
    }

}

/**
 * Opens a short-lived IPMI 2.0 out-of-band session against the BMC described
 * by the given client's settings. The returned context must be closed with
 * ipmi_ctx_close() and freed with ipmi_ctx_destroy() when no longer needed.
 *
 * @param client
 *     The guac_client whose settings describe the BMC to connect to.
 *
 * @return
 *     A connected ipmi_ctx_t on success, or NULL on failure (with an error
 *     logged on behalf of the client).
 */
static ipmi_ctx_t guac_ipmi_chassis_connect(guac_client* client) {

    guac_ipmi_client* ipmi_client = (guac_ipmi_client*) client->data;
    guac_ipmi_settings* settings = ipmi_client->settings;

    ipmi_ctx_t ctx = ipmi_ctx_create();
    if (ctx == NULL) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "Unable to allocate IPMI context for chassis command.");
        return NULL;
    }

    /* The BMC key (K_g) is optional; its length is required separately as it
     * may legitimately contain null bytes. */
    const unsigned char* k_g = (const unsigned char*) settings->k_g;
    unsigned int k_g_len = settings->k_g != NULL ? strlen(settings->k_g) : 0;

    /* Open an IPMI 2.0 (RMCP+) out-of-band session. Session and retransmission
     * timeouts of 0 request the library defaults. */
    if (ipmi_ctx_open_outofband_2_0(ctx,
                settings->hostname,
                settings->username,
                settings->password,
                k_g, k_g_len,
                guac_ipmi_map_privilege_level(settings->privilege_level),
                (uint8_t) settings->cipher_suite,
                GUAC_IPMI_CHASSIS_SESSION_TIMEOUT,
                GUAC_IPMI_CHASSIS_RETRANSMISSION_TIMEOUT,
                0 /* workaround flags */,
                0 /* flags */) < 0) {
        guac_client_log(client, GUAC_LOG_ERROR,
                "Unable to open IPMI session for chassis command: %s",
                ipmi_ctx_errormsg(ctx));
        ipmi_ctx_destroy(ctx);
        return NULL;
    }

    return ctx;

}

/**
 * Closes and destroys the given IPMI context.
 *
 * @param ctx
 *     The context to close and destroy.
 */
static void guac_ipmi_chassis_disconnect(ipmi_ctx_t ctx) {
    ipmi_ctx_close(ctx);
    ipmi_ctx_destroy(ctx);
}

int guac_ipmi_chassis_power(guac_client* client,
        guac_ipmi_power_action action) {

    /* A request for no action always succeeds */
    if (action == GUAC_IPMI_POWER_NONE)
        return 0;

    ipmi_ctx_t ctx = guac_ipmi_chassis_connect(client);
    if (ctx == NULL)
        return 1;

    int result = 1;
    fiid_obj_t obj_cmd_rs = fiid_obj_create(tmpl_cmd_chassis_control_rs);
    if (obj_cmd_rs != NULL) {

        if (ipmi_cmd_chassis_control(ctx,
                    guac_ipmi_map_power_action(action), obj_cmd_rs) < 0)
            guac_client_log(client, GUAC_LOG_ERROR,
                    "Chassis control command failed: %s",
                    ipmi_ctx_errormsg(ctx));
        else
            result = 0;

        fiid_obj_destroy(obj_cmd_rs);
    }

    guac_ipmi_chassis_disconnect(ctx);
    return result;

}

int guac_ipmi_chassis_set_boot_device(guac_client* client,
        guac_ipmi_boot_device device, bool persistent) {

    /* A request for no override always succeeds */
    if (device == GUAC_IPMI_BOOT_NONE)
        return 0;

    ipmi_ctx_t ctx = guac_ipmi_chassis_connect(client);
    if (ctx == NULL)
        return 1;

    int result = 1;
    fiid_obj_t obj_cmd_rs =
        fiid_obj_create(tmpl_cmd_set_system_boot_options_rs);
    if (obj_cmd_rs != NULL) {

        /* Issue a boot flags override. All boot-flag fields other than the
         * boot device, persistence, and "valid" markers are left at their
         * default/disabled (0) values. */
        if (ipmi_cmd_set_system_boot_options_boot_flags(ctx,
                    IPMI_SYSTEM_BOOT_OPTIONS_PARAMETER_VALID_UNLOCKED,
                    IPMI_SYSTEM_BOOT_OPTION_BOOT_FLAG_BOOT_TYPE_PC_COMPATIBLE,
                    persistent
                        ? IPMI_SYSTEM_BOOT_OPTION_BOOT_FLAG_VALID_PERSISTENTLY
                        : IPMI_SYSTEM_BOOT_OPTION_BOOT_FLAG_VALID_FOR_NEXT_BOOT,
                    IPMI_SYSTEM_BOOT_OPTION_BOOT_FLAG_VALID,
                    0 /* lock out reset button */,
                    0 /* screen blank */,
                    guac_ipmi_map_boot_device(device),
                    0 /* lock keyboard */,
                    0 /* CMOS clear */,
                    IPMI_SYSTEM_BOOT_OPTION_BOOT_FLAG_CONSOLE_REDIRECTION_DEFAULT,
                    0 /* lock out sleep button */,
                    0 /* user password bypass */,
                    0 /* force progress event traps */,
                    IPMI_SYSTEM_BOOT_OPTION_BOOT_FLAG_FIRMWARE_BIOS_VERBOSITY_DEFAULT,
                    0 /* lock out via power button */,
                    0 /* BIOS mux control override (recommended setting) */,
                    0 /* BIOS shared mode override (recommended setting) */,
                    0 /* device instance selector */,
                    obj_cmd_rs) < 0)
            guac_client_log(client, GUAC_LOG_ERROR,
                    "Set system boot options command failed: %s",
                    ipmi_ctx_errormsg(ctx));
        else
            result = 0;

        fiid_obj_destroy(obj_cmd_rs);
    }

    guac_ipmi_chassis_disconnect(ctx);
    return result;

}

int guac_ipmi_chassis_status(guac_client* client, char* buffer, int size) {

    ipmi_ctx_t ctx = guac_ipmi_chassis_connect(client);
    if (ctx == NULL)
        return 1;

    int result = 1;
    fiid_obj_t obj_cmd_rs = fiid_obj_create(tmpl_cmd_get_chassis_status_rs);
    if (obj_cmd_rs != NULL) {

        if (ipmi_cmd_get_chassis_status(ctx, obj_cmd_rs) < 0)
            guac_client_log(client, GUAC_LOG_ERROR,
                    "Get chassis status command failed: %s",
                    ipmi_ctx_errormsg(ctx));

        else {

            uint64_t power_is_on = 0;
            uint64_t power_fault = 0;
            uint64_t power_overload = 0;

            /* The power state field is mandatory; treat its absence as a
             * failure rather than silently reporting a fabricated "OFF". The
             * fault/overload fields are informational and tolerated if
             * unreadable. */
            if (fiid_obj_get(obj_cmd_rs, "current_power_state.power_is_on",
                        &power_is_on) < 0) {
                guac_client_log(client, GUAC_LOG_ERROR,
                        "Chassis status response missing power state.");
            }
            else {

                fiid_obj_get(obj_cmd_rs, "current_power_state.power_fault",
                        &power_fault);
                fiid_obj_get(obj_cmd_rs, "current_power_state.power_overload",
                        &power_overload);

                snprintf(buffer, size, "Power: %s%s%s",
                        power_is_on ? "ON" : "OFF",
                        power_fault ? " (fault)" : "",
                        power_overload ? " (overload)" : "");

                result = 0;
            }
        }

        fiid_obj_destroy(obj_cmd_rs);
    }

    guac_ipmi_chassis_disconnect(ctx);
    return result;

}

int guac_ipmi_chassis_identify(guac_client* client, int interval, bool force) {

    ipmi_ctx_t ctx = guac_ipmi_chassis_connect(client);
    if (ctx == NULL)
        return 1;

    int result = 1;
    fiid_obj_t obj_cmd_rs = fiid_obj_create(tmpl_cmd_chassis_identify_rs);
    if (obj_cmd_rs != NULL) {

        uint8_t interval_value = (uint8_t) interval;
        uint8_t force_value = force
            ? IPMI_CHASSIS_FORCE_IDENTIFY_ON
            : IPMI_CHASSIS_FORCE_IDENTIFY_OFF;

        /* When forcing the LED on indefinitely, the interval is not meaningful
         * and is passed as NULL. */
        if (ipmi_cmd_chassis_identify(ctx,
                    force ? NULL : &interval_value,
                    &force_value, obj_cmd_rs) < 0)
            guac_client_log(client, GUAC_LOG_ERROR,
                    "Chassis identify command failed: %s",
                    ipmi_ctx_errormsg(ctx));
        else
            result = 0;

        fiid_obj_destroy(obj_cmd_rs);
    }

    guac_ipmi_chassis_disconnect(ctx);
    return result;

}
