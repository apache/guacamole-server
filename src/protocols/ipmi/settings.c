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

#include "argv.h"
#include "settings.h"
#include "terminal/terminal.h"

#include <guacamole/mem.h>
#include <guacamole/string.h>
#include <guacamole/user.h>

#include <sys/types.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* Client plugin arguments */
const char* GUAC_IPMI_CLIENT_ARGS[] = {
    "hostname",
    "port",
    "timeout",
    "username",
    "password",
    "k-g",
    "privilege-level",
    "cipher-suite",
    "sol-payload-instance",
    "power-on-connect",
    "boot-device",
    GUAC_IPMI_ARGV_FONT_NAME,
    GUAC_IPMI_ARGV_FONT_SIZE,
    GUAC_IPMI_ARGV_COLOR_SCHEME,
    "typescript-path",
    "typescript-name",
    "create-typescript-path",
    "typescript-write-existing",
    "recording-path",
    "recording-name",
    "recording-exclude-output",
    "recording-exclude-mouse",
    "recording-include-keys",
    "recording-include-clipboard",
    "create-recording-path",
    "recording-write-existing",
    "read-only",
    "backspace",
    "terminal-type",
    "scrollback",
    "disable-copy",
    "disable-paste",
    NULL
};

enum IPMI_ARGS_IDX {

    /**
     * The hostname or IP address of the BMC to connect to. Required.
     */
    IDX_HOSTNAME,

    /**
     * The RMCP port. Informational only; libipmiconsole uses 623/UDP.
     */
    IDX_PORT,

    /**
     * The IPMI session timeout, in seconds. Optional.
     */
    IDX_TIMEOUT,

    /**
     * The username to authenticate with. Optional.
     */
    IDX_USERNAME,

    /**
     * The password to authenticate with. Optional.
     */
    IDX_PASSWORD,

    /**
     * The BMC key (K_g) for two-key authentication. Optional.
     */
    IDX_K_G,

    /**
     * The privilege level to authenticate with ("user", "operator", or
     * "admin"). Optional; defaults to "admin".
     */
    IDX_PRIVILEGE_LEVEL,

    /**
     * The RMCP+ cipher suite ID. Optional; defaults to 3.
     */
    IDX_CIPHER_SUITE,

    /**
     * The SOL payload instance (1-15). Optional; defaults to 1.
     */
    IDX_SOL_PAYLOAD_INSTANCE,

    /**
     * The chassis power action to perform once connected, prior to attaching
     * the console ("none", "on", "reset", or "cycle"). Optional; defaults to
     * "none".
     */
    IDX_POWER_ON_CONNECT,

    /**
     * The boot device override to apply for the next boot ("none", "pxe",
     * "disk", "cdrom", or "bios"). Optional; defaults to "none".
     */
    IDX_BOOT_DEVICE,

    /**
     * The name of the font to use within the terminal.
     */
    IDX_FONT_NAME,

    /**
     * The size of the font to use within the terminal, in points.
     */
    IDX_FONT_SIZE,

    /**
     * The color scheme to use, as accepted by the terminal emulator.
     */
    IDX_COLOR_SCHEME,

    /**
     * The full absolute path to the directory in which typescripts should be
     * written.
     */
    IDX_TYPESCRIPT_PATH,

    /**
     * The name that should be given to typescripts which are written in the
     * given path.
     */
    IDX_TYPESCRIPT_NAME,

    /**
     * Whether the specified typescript path should automatically be created if
     * it does not yet exist.
     */
    IDX_CREATE_TYPESCRIPT_PATH,

    /**
     * Whether existing files should be appended to when creating a new
     * typescript.
     */
    IDX_TYPESCRIPT_WRITE_EXISTING,

    /**
     * The full absolute path to the directory in which screen recordings
     * should be written.
     */
    IDX_RECORDING_PATH,

    /**
     * The name that should be given to screen recordings which are written in
     * the given path.
     */
    IDX_RECORDING_NAME,

    /**
     * Whether broadcast output should NOT be included in the session
     * recording.
     */
    IDX_RECORDING_EXCLUDE_OUTPUT,

    /**
     * Whether mouse state should NOT be included in the session recording.
     */
    IDX_RECORDING_EXCLUDE_MOUSE,

    /**
     * Whether keys pressed and released should be included in the session
     * recording.
     */
    IDX_RECORDING_INCLUDE_KEYS,

    /**
     * Whether clipboard data should be included in the session recording.
     * Clipboard data is NOT included by default within the recording,
     * as doing so has privacy and security implications. Including clipboard data
     * may be necessary in certain auditing contexts, but should only be done
     * with caution. Clipboard data can easily contain sensitive information, such
     * as passwords, credit card numbers, etc.
     */
    IDX_RECORDING_INCLUDE_CLIPBOARD,

    /**
     * Whether the specified screen recording path should automatically be
     * created if it does not yet exist.
     */
    IDX_CREATE_RECORDING_PATH,

    /**
     * Whether existing files should be appended to when creating a new
     * recording.
     */
    IDX_RECORDING_WRITE_EXISTING,

    /**
     * "true" if this connection should be read-only, "false" or blank
     * otherwise.
     */
    IDX_READ_ONLY,

    /**
     * ASCII code, as an integer, to use for the backspace key.
     */
    IDX_BACKSPACE,

    /**
     * The terminal emulator type that is exposed to the remote system.
     */
    IDX_TERMINAL_TYPE,

    /**
     * The maximum size of the scrollback buffer in rows.
     */
    IDX_SCROLLBACK,

    /**
     * Whether outbound clipboard access should be blocked.
     */
    IDX_DISABLE_COPY,

    /**
     * Whether inbound clipboard access should be blocked.
     */
    IDX_DISABLE_PASTE,

    IPMI_ARGS_COUNT
};

/**
 * Parses the given privilege level string into the corresponding
 * guac_ipmi_privilege_level value, logging a warning and falling back to the
 * default (admin) for unrecognized values.
 *
 * @param user
 *     The user who provided the value, on whose behalf warnings are logged.
 *
 * @param value
 *     The privilege level string ("user", "operator", or "admin").
 *
 * @return
 *     The parsed privilege level, or GUAC_IPMI_PRIVILEGE_ADMIN if the value is
 *     blank or unrecognized.
 */
static guac_ipmi_privilege_level guac_ipmi_parse_privilege_level(
        guac_user* user, const char* value) {

    if (value == NULL || strcmp(value, "") == 0
            || strcasecmp(value, "admin") == 0
            || strcasecmp(value, "administrator") == 0)
        return GUAC_IPMI_PRIVILEGE_ADMIN;

    if (strcasecmp(value, "operator") == 0)
        return GUAC_IPMI_PRIVILEGE_OPERATOR;

    if (strcasecmp(value, "user") == 0)
        return GUAC_IPMI_PRIVILEGE_USER;

    guac_user_log(user, GUAC_LOG_WARNING, "Unrecognized privilege level "
            "\"%s\"; defaulting to \"admin\".", value);
    return GUAC_IPMI_PRIVILEGE_ADMIN;

}

/**
 * Parses the given power action string into the corresponding
 * guac_ipmi_power_action value. Only the actions appropriate for an automatic
 * "on connect" action are accepted ("none", "on", "reset", "cycle").
 *
 * @param user
 *     The user who provided the value, on whose behalf warnings are logged.
 *
 * @param value
 *     The power action string.
 *
 * @return
 *     The parsed power action, or GUAC_IPMI_POWER_NONE if the value is blank
 *     or unrecognized.
 */
static guac_ipmi_power_action guac_ipmi_parse_power_action(
        guac_user* user, const char* value) {

    if (value == NULL || strcmp(value, "") == 0
            || strcasecmp(value, "none") == 0)
        return GUAC_IPMI_POWER_NONE;

    if (strcasecmp(value, "on") == 0)
        return GUAC_IPMI_POWER_ON;

    if (strcasecmp(value, "reset") == 0)
        return GUAC_IPMI_POWER_RESET;

    if (strcasecmp(value, "cycle") == 0)
        return GUAC_IPMI_POWER_CYCLE;

    guac_user_log(user, GUAC_LOG_WARNING, "Unrecognized power-on-connect "
            "action \"%s\"; no power action will be taken.", value);
    return GUAC_IPMI_POWER_NONE;

}

/**
 * Parses the given boot device string into the corresponding
 * guac_ipmi_boot_device value.
 *
 * @param user
 *     The user who provided the value, on whose behalf warnings are logged.
 *
 * @param value
 *     The boot device string ("none", "pxe", "disk", "cdrom", or "bios").
 *
 * @return
 *     The parsed boot device, or GUAC_IPMI_BOOT_NONE if the value is blank or
 *     unrecognized.
 */
static guac_ipmi_boot_device guac_ipmi_parse_boot_device(
        guac_user* user, const char* value) {

    if (value == NULL || strcmp(value, "") == 0
            || strcasecmp(value, "none") == 0)
        return GUAC_IPMI_BOOT_NONE;

    if (strcasecmp(value, "pxe") == 0)
        return GUAC_IPMI_BOOT_PXE;

    if (strcasecmp(value, "disk") == 0 || strcasecmp(value, "hd") == 0)
        return GUAC_IPMI_BOOT_DISK;

    if (strcasecmp(value, "cdrom") == 0 || strcasecmp(value, "cd") == 0)
        return GUAC_IPMI_BOOT_CDROM;

    if (strcasecmp(value, "bios") == 0 || strcasecmp(value, "setup") == 0)
        return GUAC_IPMI_BOOT_BIOS;

    guac_user_log(user, GUAC_LOG_WARNING, "Unrecognized boot device "
            "\"%s\"; boot order will be left unchanged.", value);
    return GUAC_IPMI_BOOT_NONE;

}

guac_ipmi_settings* guac_ipmi_parse_args(guac_user* user,
        int argc, const char** argv) {

    /* Validate arg count */
    if (argc != IPMI_ARGS_COUNT) {
        guac_user_log(user, GUAC_LOG_WARNING, "Incorrect number of connection "
                "parameters provided: expected %i, got %i.",
                IPMI_ARGS_COUNT, argc);
        return NULL;
    }

    guac_ipmi_settings* settings = guac_mem_zalloc(sizeof(guac_ipmi_settings));

    /* Read hostname */
    settings->hostname =
        guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_HOSTNAME, "");

    /* Read port (informational only) */
    settings->port =
        guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_PORT, GUAC_IPMI_DEFAULT_PORT);

    /* Warn if a non-default port was requested, as it cannot be honored */
    if (strcmp(settings->port, GUAC_IPMI_DEFAULT_PORT) != 0)
        guac_user_log(user, GUAC_LOG_INFO, "A non-default IPMI port (\"%s\") "
                "was specified, but IPMI out-of-band access always uses "
                "623/UDP. The provided value will be ignored.", settings->port);

    /* Read IPMI session timeout */
    settings->timeout =
        guac_user_parse_args_int(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_TIMEOUT, GUAC_IPMI_DEFAULT_TIMEOUT);

    /* Read credentials */
    settings->username =
        guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_USERNAME, NULL);

    settings->password =
        guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_PASSWORD, NULL);

    settings->k_g =
        guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_K_G, NULL);

    /* Read privilege level */
    settings->privilege_level = guac_ipmi_parse_privilege_level(user,
            guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                    IDX_PRIVILEGE_LEVEL, "admin"));

    /* Read cipher suite */
    settings->cipher_suite =
        guac_user_parse_args_int(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_CIPHER_SUITE, GUAC_IPMI_DEFAULT_CIPHER_SUITE);

    /* Warn strongly against the use of cipher suite 0 (no security) */
    if (settings->cipher_suite == 0)
        guac_user_log(user, GUAC_LOG_WARNING, "Cipher suite 0 provides NO "
                "authentication, integrity, or confidentiality. Its use is "
                "strongly discouraged.");

    /* Read SOL payload instance */
    settings->sol_payload_instance =
        guac_user_parse_args_int(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_SOL_PAYLOAD_INSTANCE, GUAC_IPMI_DEFAULT_SOL_PAYLOAD_INSTANCE);

    /* Read pre-connect power and boot actions */
    settings->power_on_connect = guac_ipmi_parse_power_action(user,
            guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                    IDX_POWER_ON_CONNECT, "none"));

    settings->boot_device = guac_ipmi_parse_boot_device(user,
            guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                    IDX_BOOT_DEVICE, "none"));

    /* Read-only mode */
    settings->read_only =
        guac_user_parse_args_boolean(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_READ_ONLY, false);

    /* Read maximum scrollback size */
    settings->max_scrollback =
        guac_user_parse_args_int(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_SCROLLBACK, GUAC_TERMINAL_DEFAULT_MAX_SCROLLBACK);

    /* Read font name */
    settings->font_name =
        guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_FONT_NAME, GUAC_TERMINAL_DEFAULT_FONT_NAME);

    /* Read font size */
    settings->font_size =
        guac_user_parse_args_int(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_FONT_SIZE, GUAC_TERMINAL_DEFAULT_FONT_SIZE);

    /* Copy requested color scheme */
    settings->color_scheme =
        guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_COLOR_SCHEME, GUAC_TERMINAL_DEFAULT_COLOR_SCHEME);

    /* Pull width/height/resolution directly from user */
    settings->width      = user->info.optimal_width;
    settings->height     = user->info.optimal_height;
    settings->resolution = user->info.optimal_resolution;

    /* Read typescript path */
    settings->typescript_path =
        guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_TYPESCRIPT_PATH, NULL);

    /* Read typescript name */
    settings->typescript_name =
        guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_TYPESCRIPT_NAME, GUAC_IPMI_DEFAULT_TYPESCRIPT_NAME);

    /* Parse path creation flag */
    settings->create_typescript_path =
        guac_user_parse_args_boolean(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_CREATE_TYPESCRIPT_PATH, false);

    /* Parse allow write existing file flag */
    settings->typescript_write_existing =
        guac_user_parse_args_boolean(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_TYPESCRIPT_WRITE_EXISTING, false);

    /* Read recording path */
    settings->recording_path =
        guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_RECORDING_PATH, NULL);

    /* Read recording name */
    settings->recording_name =
        guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_RECORDING_NAME, GUAC_IPMI_DEFAULT_RECORDING_NAME);

    /* Parse output exclusion flag */
    settings->recording_exclude_output =
        guac_user_parse_args_boolean(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_RECORDING_EXCLUDE_OUTPUT, false);

    /* Parse mouse exclusion flag */
    settings->recording_exclude_mouse =
        guac_user_parse_args_boolean(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_RECORDING_EXCLUDE_MOUSE, false);

    /* Parse key event inclusion flag */
    settings->recording_include_keys =
        guac_user_parse_args_boolean(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_RECORDING_INCLUDE_KEYS, false);

    /* Parse clipboard inclusion flag */
    settings->recording_include_clipboard =
        guac_user_parse_args_boolean(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_RECORDING_INCLUDE_CLIPBOARD, false);

    /* Parse path creation flag */
    settings->create_recording_path =
        guac_user_parse_args_boolean(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_CREATE_RECORDING_PATH, false);

    /* Parse allow write existing file flag */
    settings->recording_write_existing =
        guac_user_parse_args_boolean(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_RECORDING_WRITE_EXISTING, false);

    /* Parse backspace key code */
    settings->backspace =
        guac_user_parse_args_int(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_BACKSPACE, GUAC_TERMINAL_DEFAULT_BACKSPACE);

    /* Read terminal emulator type */
    settings->terminal_type =
        guac_user_parse_args_string(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_TERMINAL_TYPE, "linux");

    /* Parse clipboard copy disable flag */
    settings->disable_copy =
        guac_user_parse_args_boolean(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_DISABLE_COPY, false);

    /* Parse clipboard paste disable flag */
    settings->disable_paste =
        guac_user_parse_args_boolean(user, GUAC_IPMI_CLIENT_ARGS, argv,
                IDX_DISABLE_PASTE, false);

    /* Parsing was successful */
    return settings;

}

void guac_ipmi_settings_free(guac_ipmi_settings* settings) {

    /* Free network connection information */
    guac_mem_free(settings->hostname);
    guac_mem_free(settings->port);

    /* Free credentials */
    guac_mem_free(settings->username);
    guac_mem_free(settings->password);
    guac_mem_free(settings->k_g);

    /* Free display preferences */
    guac_mem_free(settings->font_name);
    guac_mem_free(settings->color_scheme);

    /* Free typescript settings */
    guac_mem_free(settings->typescript_name);
    guac_mem_free(settings->typescript_path);

    /* Free screen recording settings */
    guac_mem_free(settings->recording_name);
    guac_mem_free(settings->recording_path);

    /* Free terminal emulator type */
    guac_mem_free(settings->terminal_type);

    /* Free overall structure */
    guac_mem_free(settings);

}
