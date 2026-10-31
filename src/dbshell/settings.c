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

#include "common/defaults.h"
#include "dbshell/settings.h"
#include "terminal/terminal.h"

#include <guacamole/mem.h>
#include <guacamole/string.h>
#include <guacamole/user.h>
#include <guacamole/wol.h>

#include <stdlib.h>
#include <string.h>

/**
 * The indices of the common arguments within the argument list of any
 * database protocol plugin. These indices correspond exactly, in order, to
 * the arguments declared by GUAC_DBSHELL_COMMON_ARGS.
 */
enum GUAC_DBSHELL_ARGS_IDX {

    /**
     * The hostname or IP address of the database server to connect to.
     */
    IDX_HOSTNAME,

    /**
     * The TCP port of the database server to connect to.
     */
    IDX_PORT,

    /**
     * The username to authenticate as.
     */
    IDX_USERNAME,

    /**
     * The password to authenticate with.
     */
    IDX_PASSWORD,

    /**
     * The name of the database to use initially.
     */
    IDX_DATABASE,

    /**
     * The maximum number of seconds to wait for the connection to the
     * database server to be established.
     */
    IDX_TIMEOUT,

    /**
     * The name of the font to use within the terminal.
     */
    IDX_FONT_NAME,

    /**
     * The size of the font to use within the terminal, in points.
     */
    IDX_FONT_SIZE,

    /**
     * The color scheme to use, as a series of semicolon-separated
     * color-value pairs, or one of the special values "black-white",
     * "white-black", "gray-black", or "green-black".
     */
    IDX_COLOR_SCHEME,

    /**
     * The full absolute path to the directory in which typescripts should
     * be written.
     */
    IDX_TYPESCRIPT_PATH,

    /**
     * The name that should be given to typescripts which are written in
     * the given path.
     */
    IDX_TYPESCRIPT_NAME,

    /**
     * Whether the specified typescript path should automatically be
     * created if it does not yet exist.
     */
    IDX_CREATE_TYPESCRIPT_PATH,

    /**
     * Whether existing files should be appended to when creating a new
     * typescript. Disabled by default.
     */
    IDX_TYPESCRIPT_WRITE_EXISTING,

    /**
     * The full absolute path to the directory in which screen recordings
     * should be written.
     */
    IDX_RECORDING_PATH,

    /**
     * The name that should be given to screen recordings which are written
     * in the given path.
     */
    IDX_RECORDING_NAME,

    /**
     * Whether output which is broadcast to each connected client should
     * NOT be included in the session recording.
     */
    IDX_RECORDING_EXCLUDE_OUTPUT,

    /**
     * Whether changes to mouse state should NOT be included in the session
     * recording.
     */
    IDX_RECORDING_EXCLUDE_MOUSE,

    /**
     * Whether keys pressed and released should be included in the session
     * recording.
     */
    IDX_RECORDING_INCLUDE_KEYS,

    /**
     * Whether clipboard data should be included in the session recording.
     */
    IDX_RECORDING_INCLUDE_CLIPBOARD,

    /**
     * Whether the specified screen recording path should automatically be
     * created if it does not yet exist.
     */
    IDX_CREATE_RECORDING_PATH,

    /**
     * Whether existing files should be appended to when creating a new
     * recording. Disabled by default.
     */
    IDX_RECORDING_WRITE_EXISTING,

    /**
     * "true" if this connection should be read-only (user input should be
     * dropped), "false" or blank otherwise.
     */
    IDX_READ_ONLY,

    /**
     * ASCII code, as an integer, to use for the backspace key, or
     * GUAC_TERMINAL_DEFAULT_BACKSPACE if not specified.
     */
    IDX_BACKSPACE,

    /**
     * The maximum size of the scrollback buffer in rows.
     */
    IDX_SCROLLBACK,

    /**
     * The family of codes (e.g. vt100) which will be used when the
     * function and keypad keys are pressed.
     */
    IDX_FUNC_KEYS_AND_KEYPAD,

    /**
     * The maximum number of bytes to allow within the clipboard.
     */
    IDX_CLIPBOARD_BUFFER_SIZE,

    /**
     * Whether outbound clipboard access should be blocked. If set to
     * "true", it will not be possible to copy data from the terminal to
     * the client using the clipboard.
     */
    IDX_DISABLE_COPY,

    /**
     * Whether inbound clipboard access should be blocked. If set to
     * "true", it will not be possible to paste data from the client to the
     * terminal using the clipboard.
     */
    IDX_DISABLE_PASTE,

    /**
     * The terminal emulator type presented to the user (e.g. "xterm").
     * "linux" is used if unspecified.
     */
    IDX_TERMINAL_TYPE,

    /**
     * Whether a Wake-on-LAN packet should be sent to the database server
     * prior to connecting.
     */
    IDX_WOL_SEND_PACKET,

    /**
     * The MAC address to place within the Wake-on-LAN packet.
     */
    IDX_WOL_MAC_ADDR,

    /**
     * The broadcast address to which the Wake-on-LAN packet should be
     * sent.
     */
    IDX_WOL_BROADCAST_ADDR,

    /**
     * The UDP port to use when sending the Wake-on-LAN packet.
     */
    IDX_WOL_UDP_PORT,

    /**
     * The number of seconds to wait after sending the Wake-on-LAN packet
     * before attempting to connect to the database server.
     */
    IDX_WOL_WAIT_TIME,

    GUAC_DBSHELL_ARGS_IDX_COUNT

} ;

/* The argument indices must match the argument list */
_Static_assert(GUAC_DBSHELL_ARGS_IDX_COUNT == GUAC_DBSHELL_COMMON_ARG_COUNT,
        "GUAC_DBSHELL_COMMON_ARGS and GUAC_DBSHELL_ARGS_IDX must declare "
        "the same number of arguments");

guac_dbshell_settings* guac_dbshell_settings_parse(guac_user* user,
        const char** protocol_args, int argc, const char** argv,
        int expected_argc, int default_port) {

    /* Validate arg count */
    if (argc != expected_argc) {
        guac_user_log(user, GUAC_LOG_WARNING, "Incorrect number of "
                "connection parameters provided: expected %i, got %i.",
                expected_argc, argc);
        return NULL;
    }

    guac_dbshell_settings* settings =
        guac_mem_zalloc(sizeof(guac_dbshell_settings));

    /* Read hostname */
    settings->hostname =
        guac_user_parse_args_string(user, protocol_args, argv,
                IDX_HOSTNAME, "");

    /* Read port */
    settings->port =
        guac_user_parse_args_int_bounded(user, protocol_args, argv,
                IDX_PORT, default_port, GUAC_ITOA_USHORT_MIN + 1,
                GUAC_ITOA_USHORT_MAX);

    /* Read credentials and database (NULL if not provided) */
    settings->username =
        guac_user_parse_args_string(user, protocol_args, argv,
                IDX_USERNAME, NULL);

    settings->password =
        guac_user_parse_args_string(user, protocol_args, argv,
                IDX_PASSWORD, NULL);

    settings->database =
        guac_user_parse_args_string(user, protocol_args, argv,
                IDX_DATABASE, NULL);

    /* Read connect timeout */
    settings->timeout =
        guac_user_parse_args_int(user, protocol_args, argv,
                IDX_TIMEOUT, GUAC_DBSHELL_DEFAULT_TIMEOUT);

    /* Read display preferences */
    settings->font_name =
        guac_user_parse_args_string(user, protocol_args, argv,
                IDX_FONT_NAME, GUAC_TERMINAL_DEFAULT_FONT_NAME);

    settings->font_size =
        guac_user_parse_args_int(user, protocol_args, argv,
                IDX_FONT_SIZE, GUAC_TERMINAL_DEFAULT_FONT_SIZE);

    settings->color_scheme =
        guac_user_parse_args_string(user, protocol_args, argv,
                IDX_COLOR_SCHEME, GUAC_TERMINAL_DEFAULT_COLOR_SCHEME);

    /* Read typescript preferences */
    settings->typescript_path =
        guac_user_parse_args_string(user, protocol_args, argv,
                IDX_TYPESCRIPT_PATH, NULL);

    settings->typescript_name =
        guac_user_parse_args_string(user, protocol_args, argv,
                IDX_TYPESCRIPT_NAME, GUAC_DBSHELL_DEFAULT_TYPESCRIPT_NAME);

    settings->create_typescript_path =
        guac_user_parse_args_boolean(user, protocol_args, argv,
                IDX_CREATE_TYPESCRIPT_PATH, false);

    settings->typescript_write_existing =
        guac_user_parse_args_boolean(user, protocol_args, argv,
                IDX_TYPESCRIPT_WRITE_EXISTING, false);

    /* Read screen recording preferences */
    settings->recording_path =
        guac_user_parse_args_string(user, protocol_args, argv,
                IDX_RECORDING_PATH, NULL);

    settings->recording_name =
        guac_user_parse_args_string(user, protocol_args, argv,
                IDX_RECORDING_NAME, GUAC_DBSHELL_DEFAULT_RECORDING_NAME);

    settings->recording_exclude_output =
        guac_user_parse_args_boolean(user, protocol_args, argv,
                IDX_RECORDING_EXCLUDE_OUTPUT, false);

    settings->recording_exclude_mouse =
        guac_user_parse_args_boolean(user, protocol_args, argv,
                IDX_RECORDING_EXCLUDE_MOUSE, false);

    settings->recording_include_keys =
        guac_user_parse_args_boolean(user, protocol_args, argv,
                IDX_RECORDING_INCLUDE_KEYS, false);

    settings->recording_include_clipboard =
        guac_user_parse_args_boolean(user, protocol_args, argv,
                IDX_RECORDING_INCLUDE_CLIPBOARD, false);

    settings->create_recording_path =
        guac_user_parse_args_boolean(user, protocol_args, argv,
                IDX_CREATE_RECORDING_PATH, false);

    settings->recording_write_existing =
        guac_user_parse_args_boolean(user, protocol_args, argv,
                IDX_RECORDING_WRITE_EXISTING, false);

    /* Read terminal behavior preferences */
    settings->read_only =
        guac_user_parse_args_boolean(user, protocol_args, argv,
                IDX_READ_ONLY, false);

    settings->backspace =
        guac_user_parse_args_int(user, protocol_args, argv,
                IDX_BACKSPACE, GUAC_TERMINAL_DEFAULT_BACKSPACE);

    settings->max_scrollback =
        guac_user_parse_args_int(user, protocol_args, argv,
                IDX_SCROLLBACK, GUAC_TERMINAL_DEFAULT_MAX_SCROLLBACK);

    settings->func_keys_and_keypad =
        guac_user_parse_args_string(user, protocol_args, argv,
                IDX_FUNC_KEYS_AND_KEYPAD, "");

    settings->clipboard_buffer_size =
        guac_user_parse_args_int(user, protocol_args, argv,
                IDX_CLIPBOARD_BUFFER_SIZE, 0);

    settings->disable_copy =
        guac_user_parse_args_boolean(user, protocol_args, argv,
                IDX_DISABLE_COPY, false);

    settings->disable_paste =
        guac_user_parse_args_boolean(user, protocol_args, argv,
                IDX_DISABLE_PASTE, false);

    settings->terminal_type =
        guac_user_parse_args_string(user, protocol_args, argv,
                IDX_TERMINAL_TYPE, "linux");

    /* Read Wake-on-LAN preferences */
    settings->wol_send_packet =
        guac_user_parse_args_boolean(user, protocol_args, argv,
                IDX_WOL_SEND_PACKET, false);

    if (settings->wol_send_packet) {

        /* Warn and disable if WoL was requested without a MAC address */
        if (strcmp(argv[IDX_WOL_MAC_ADDR], "") == 0) {
            guac_user_log(user, GUAC_LOG_WARNING, "WoL was enabled, but no "
                    "MAC address was provided. WoL will not be sent.");
            settings->wol_send_packet = false;
        }

        settings->wol_mac_addr =
            guac_user_parse_args_string(user, protocol_args, argv,
                    IDX_WOL_MAC_ADDR, NULL);

        settings->wol_broadcast_addr =
            guac_user_parse_args_string(user, protocol_args, argv,
                    IDX_WOL_BROADCAST_ADDR, GUAC_WOL_LOCAL_IPV4_BROADCAST);

        settings->wol_udp_port = (unsigned short)
            guac_user_parse_args_int_bounded(user, protocol_args, argv,
                    IDX_WOL_UDP_PORT, GUAC_WOL_PORT, GUAC_ITOA_USHORT_MIN,
                    GUAC_ITOA_USHORT_MAX);

        settings->wol_wait_time =
            guac_user_parse_args_int(user, protocol_args, argv,
                    IDX_WOL_WAIT_TIME, GUAC_WOL_DEFAULT_BOOT_WAIT_TIME);

    }

    /* Use the dimensions and resolution of the connecting user's optimal
     * display */
    settings->width      = user->info.optimal_width;
    settings->height     = user->info.optimal_height;
    settings->resolution = user->info.optimal_resolution;

    return settings;

}

void guac_dbshell_settings_free(guac_dbshell_settings* settings) {

    if (settings == NULL)
        return;

    /* Free network and authentication details */
    guac_mem_free(settings->hostname);
    guac_mem_free(settings->username);
    guac_mem_free(settings->password);
    guac_mem_free(settings->database);

    /* Free display preferences */
    guac_mem_free(settings->font_name);
    guac_mem_free(settings->color_scheme);

    /* Free typescript and recording settings */
    guac_mem_free(settings->typescript_path);
    guac_mem_free(settings->typescript_name);
    guac_mem_free(settings->recording_path);
    guac_mem_free(settings->recording_name);

    /* Free terminal behavior settings */
    guac_mem_free(settings->func_keys_and_keypad);
    guac_mem_free(settings->terminal_type);

    /* Free Wake-on-LAN settings */
    guac_mem_free(settings->wol_mac_addr);
    guac_mem_free(settings->wol_broadcast_addr);

    guac_mem_free(settings);

}
