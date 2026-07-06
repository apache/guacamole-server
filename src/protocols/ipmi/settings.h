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

#ifndef GUAC_IPMI_SETTINGS_H
#define GUAC_IPMI_SETTINGS_H

#include <guacamole/user.h>

#include <sys/types.h>
#include <stdbool.h>

/**
 * The RMCP port used for IPMI 2.0 out-of-band access. This is fixed at 623/UDP
 * by libipmiconsole and libfreeipmi; the "port" parameter is accepted for
 * forward compatibility and informational logging only.
 */
#define GUAC_IPMI_DEFAULT_PORT "623"

/**
 * The protocol label included in the process title (the first argument passed
 * to guac_process_title_set_endpoint()), as seen in `ps`/`top`.
 */
#define GUAC_IPMI_PROCESS_TITLE_NAME "ipmi"

/**
 * The default IPMI session timeout, in seconds, used when establishing the
 * Serial-over-LAN session.
 */
#define GUAC_IPMI_DEFAULT_TIMEOUT 60

/**
 * The default RMCP+ cipher suite ID. Suite 3 (HMAC-SHA1 / HMAC-SHA1-96 /
 * AES-CBC-128) is the most widely supported authenticated and encrypted
 * suite. Cipher suite 0 (no auth/integrity/confidentiality) is intentionally
 * NOT the default for security reasons.
 */
#define GUAC_IPMI_DEFAULT_CIPHER_SUITE 3

/**
 * The default SOL payload instance. Most BMCs support only a single instance.
 */
#define GUAC_IPMI_DEFAULT_SOL_PAYLOAD_INSTANCE 1

/**
 * The filename to use for the typescript, if not specified.
 */
#define GUAC_IPMI_DEFAULT_TYPESCRIPT_NAME "typescript"

/**
 * The filename to use for the screen recording, if not specified.
 */
#define GUAC_IPMI_DEFAULT_RECORDING_NAME "recording"

/**
 * The privilege level to authenticate with, expressed using the
 * IPMICONSOLE_PRIVILEGE_* constants from libipmiconsole. The values here
 * intentionally match those constants.
 */
typedef enum guac_ipmi_privilege_level {
    GUAC_IPMI_PRIVILEGE_USER     = 0,
    GUAC_IPMI_PRIVILEGE_OPERATOR = 1,
    GUAC_IPMI_PRIVILEGE_ADMIN    = 2
} guac_ipmi_privilege_level;

/**
 * A chassis power action which may be performed against the BMC, either
 * automatically when the connection is established or on demand via the
 * in-terminal control menu.
 */
typedef enum guac_ipmi_power_action {

    /**
     * No power action.
     */
    GUAC_IPMI_POWER_NONE,

    /**
     * Power the system on (IPMI_CHASSIS_CONTROL_POWER_UP).
     */
    GUAC_IPMI_POWER_ON,

    /**
     * Hard power the system off (IPMI_CHASSIS_CONTROL_POWER_DOWN).
     */
    GUAC_IPMI_POWER_OFF,

    /**
     * Power cycle the system (IPMI_CHASSIS_CONTROL_POWER_CYCLE).
     */
    GUAC_IPMI_POWER_CYCLE,

    /**
     * Hard reset the system (IPMI_CHASSIS_CONTROL_HARD_RESET).
     */
    GUAC_IPMI_POWER_RESET,

    /**
     * Request a graceful (ACPI) shutdown
     * (IPMI_CHASSIS_CONTROL_INITIATE_SOFT_SHUTDOWN).
     */
    GUAC_IPMI_POWER_SOFT_SHUTDOWN,

    /**
     * Pulse a diagnostic interrupt / NMI
     * (IPMI_CHASSIS_CONTROL_PULSE_DIAGNOSTIC_INTERRUPT).
     */
    GUAC_IPMI_POWER_DIAGNOSTIC_INTERRUPT

} guac_ipmi_power_action;

/**
 * A boot device override which may be applied for the next system boot.
 */
typedef enum guac_ipmi_boot_device {

    /**
     * No boot device override (leave the BMC's configured boot order).
     */
    GUAC_IPMI_BOOT_NONE,

    /**
     * Force the next boot from the network (PXE).
     */
    GUAC_IPMI_BOOT_PXE,

    /**
     * Force the next boot from the primary hard drive.
     */
    GUAC_IPMI_BOOT_DISK,

    /**
     * Force the next boot from removable media (CD/DVD).
     */
    GUAC_IPMI_BOOT_CDROM,

    /**
     * Force the next boot into the BIOS/UEFI setup utility.
     */
    GUAC_IPMI_BOOT_BIOS

} guac_ipmi_boot_device;

/**
 * Settings for the IPMI Serial-over-LAN connection. The values for this
 * structure are parsed from the arguments given during the Guacamole protocol
 * handshake using the guac_ipmi_parse_args() function.
 */
typedef struct guac_ipmi_settings {

    /**
     * The hostname or IP address of the BMC to connect to.
     */
    char* hostname;

    /**
     * The RMCP port, as a string. Accepted for forward compatibility only;
     * libipmiconsole always uses 623/UDP.
     */
    char* port;

    /**
     * The IPMI session timeout, in seconds.
     */
    int timeout;

    /**
     * The username to authenticate with, if any. NULL if unspecified (the BMC
     * default / null username is used).
     */
    char* username;

    /**
     * The password to authenticate with, if any. NULL if unspecified.
     */
    char* password;

    /**
     * The BMC key (K_g) for two-key authentication, if any. NULL if
     * unspecified, in which case the password is used as the BMC key.
     */
    char* k_g;

    /**
     * The privilege level to authenticate with.
     */
    guac_ipmi_privilege_level privilege_level;

    /**
     * The RMCP+ cipher suite ID determining the authentication, integrity, and
     * confidentiality algorithms used.
     */
    int cipher_suite;

    /**
     * The SOL payload instance to use (1-15).
     */
    int sol_payload_instance;

    /**
     * The chassis power action to perform automatically once the connection is
     * established, prior to attaching the console. GUAC_IPMI_POWER_NONE if no
     * action should be taken.
     */
    guac_ipmi_power_action power_on_connect;

    /**
     * The boot device override to apply (for the next boot) prior to any
     * power-on-connect action. GUAC_IPMI_BOOT_NONE if the BMC's configured
     * boot order should be left unchanged.
     */
    guac_ipmi_boot_device boot_device;

    /**
     * Whether this connection is read-only, and user input should be dropped.
     */
    bool read_only;

    /**
     * The maximum size of the scrollback buffer in rows.
     */
    int max_scrollback;

    /**
     * The name of the font to use for display rendering.
     */
    char* font_name;

    /**
     * The size of the font to use, in points.
     */
    int font_size;

    /**
     * The name of the color scheme to use.
     */
    char* color_scheme;

    /**
     * The desired width of the terminal display, in pixels.
     */
    int width;

    /**
     * The desired height of the terminal display, in pixels.
     */
    int height;

    /**
     * The desired screen resolution, in DPI.
     */
    int resolution;

    /**
     * Whether outbound clipboard access should be blocked.
     */
    bool disable_copy;

    /**
     * Whether inbound clipboard access should be blocked.
     */
    bool disable_paste;

    /**
     * The path in which the typescript should be saved, if enabled. NULL if no
     * typescript should be saved.
     */
    char* typescript_path;

    /**
     * The filename to use for the typescript, if enabled.
     */
    char* typescript_name;

    /**
     * Whether the typescript path should be automatically created if it does
     * not already exist.
     */
    bool create_typescript_path;

    /**
     * Whether existing files should be appended to when creating a new
     * typescript.
     */
    bool typescript_write_existing;

    /**
     * The path in which the screen recording should be saved, if enabled. NULL
     * if no screen recording should be saved.
     */
    char* recording_path;

    /**
     * The filename to use for the screen recording, if enabled.
     */
    char* recording_name;

    /**
     * Whether the screen recording path should be automatically created if it
     * does not already exist.
     */
    bool create_recording_path;

    /**
     * Whether output which is broadcast to each connected client should NOT be
     * included in the session recording.
     */
    bool recording_exclude_output;

    /**
     * Whether changes to mouse state should NOT be included in the session
     * recording.
     */
    bool recording_exclude_mouse;

    /**
     * Whether keys pressed and released should be included in the session
     * recording.
     */
    bool recording_include_keys;

    /**
     * Whether clipboard data should be included in the session recording.
     */
    bool recording_include_clipboard;

    /**
     * Whether existing files should be appended to when creating a new
     * recording.
     */
    bool recording_write_existing;

    /**
     * The ASCII code, as an integer, that the terminal will send when the
     * backspace key is pressed.
     */
    int backspace;

    /**
     * The terminal emulator type that is exposed to the remote system.
     */
    char* terminal_type;

} guac_ipmi_settings;

/**
 * Parses all given args, storing them in a newly-allocated settings object. If
 * the args fail to parse, NULL is returned.
 *
 * @param user
 *     The user who submitted the given arguments while joining the connection.
 *
 * @param argc
 *     The number of arguments within the argv array.
 *
 * @param argv
 *     The values of all arguments provided by the user.
 *
 * @return
 *     A newly-allocated settings object which must be freed with
 *     guac_ipmi_settings_free() when no longer needed. If the arguments fail
 *     to parse, NULL is returned.
 */
guac_ipmi_settings* guac_ipmi_parse_args(guac_user* user,
        int argc, const char** argv);

/**
 * Frees the given guac_ipmi_settings object, having been previously allocated
 * via guac_ipmi_parse_args().
 *
 * @param settings
 *     The settings object to free.
 */
void guac_ipmi_settings_free(guac_ipmi_settings* settings);

/**
 * NULL-terminated array of accepted client args.
 */
extern const char* GUAC_IPMI_CLIENT_ARGS[];

#endif
