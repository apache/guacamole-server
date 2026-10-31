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

#ifndef GUAC_DBSHELL_SETTINGS_H
#define GUAC_DBSHELL_SETTINGS_H

/**
 * Declarations for the connection settings shared by all database protocol
 * plugins. Each plugin's argument list begins with the common arguments
 * declared here (via GUAC_DBSHELL_COMMON_ARGS), optionally followed by
 * database-specific arguments parsed by the plugin itself.
 *
 * @file settings.h
 */

#include <guacamole/user.h>

#include <stdbool.h>

/**
 * The default number of seconds to wait for a connection to the database
 * server to be established before giving up.
 */
#define GUAC_DBSHELL_DEFAULT_TIMEOUT 10

/**
 * The filename to use for the typescript, if not specified.
 */
#define GUAC_DBSHELL_DEFAULT_TYPESCRIPT_NAME "typescript"

/**
 * The filename to use for the screen recording, if not specified.
 */
#define GUAC_DBSHELL_DEFAULT_RECORDING_NAME "recording"

/**
 * The name of the argument announcing the terminal color scheme, which may
 * also be updated mid-session.
 */
#define GUAC_DBSHELL_ARGV_COLOR_SCHEME "color-scheme"

/**
 * The name of the argument announcing the terminal font name, which may
 * also be updated mid-session.
 */
#define GUAC_DBSHELL_ARGV_FONT_NAME "font-name"

/**
 * The name of the argument announcing the terminal font size, which may
 * also be updated mid-session.
 */
#define GUAC_DBSHELL_ARGV_FONT_SIZE "font-size"

/**
 * The number of arguments within GUAC_DBSHELL_COMMON_ARGS. Any
 * database-specific arguments of a plugin begin at this index within the
 * plugin's argument list.
 */
#define GUAC_DBSHELL_COMMON_ARG_COUNT 34

/**
 * The connection arguments common to all database protocol plugins. Each
 * plugin embeds this list at the beginning of its NULL-terminated argument
 * array, appending any database-specific arguments afterwards. The
 * corresponding values are parsed into a guac_dbshell_settings structure by
 * guac_dbshell_settings_parse().
 */
#define GUAC_DBSHELL_COMMON_ARGS      \
    "hostname",                       \
    "port",                           \
    "username",                       \
    "password",                       \
    "database",                       \
    "timeout",                        \
    GUAC_DBSHELL_ARGV_FONT_NAME,      \
    GUAC_DBSHELL_ARGV_FONT_SIZE,      \
    GUAC_DBSHELL_ARGV_COLOR_SCHEME,   \
    "typescript-path",                \
    "typescript-name",                \
    "create-typescript-path",         \
    "typescript-write-existing",      \
    "recording-path",                 \
    "recording-name",                 \
    "recording-exclude-output",       \
    "recording-exclude-mouse",        \
    "recording-include-keys",         \
    "recording-include-clipboard",    \
    "create-recording-path",          \
    "recording-write-existing",       \
    "read-only",                      \
    "backspace",                      \
    "scrollback",                     \
    "func-keys-and-keypad",           \
    "clipboard-buffer-size",          \
    "disable-copy",                   \
    "disable-paste",                  \
    "terminal-type",                  \
    "wol-send-packet",                \
    "wol-mac-addr",                   \
    "wol-broadcast-addr",             \
    "wol-udp-port",                   \
    "wol-wait-time"

/**
 * The settings common to all database protocol plugins, parsed from the
 * leading GUAC_DBSHELL_COMMON_ARG_COUNT arguments given during the
 * Guacamole protocol handshake.
 */
typedef struct guac_dbshell_settings {

    /**
     * The hostname or IP address of the database server to connect to.
     */
    char* hostname;

    /**
     * The TCP port of the database server to connect to.
     */
    int port;

    /**
     * The username to authenticate as, or NULL if no username was
     * provided.
     */
    char* username;

    /**
     * The password to authenticate with, or NULL if no password was
     * provided.
     */
    char* password;

    /**
     * The name of the database to use initially, or NULL if no database
     * was provided.
     */
    char* database;

    /**
     * The maximum number of seconds to wait for the connection to the
     * database server to be established.
     */
    int timeout;

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
     * The path in which the typescript should be saved, if enabled. If no
     * typescript should be saved, this will be NULL.
     */
    char* typescript_path;

    /**
     * The filename to use for the typescript, if enabled.
     */
    char* typescript_name;

    /**
     * Whether the typescript path should be automatically created if it
     * does not already exist.
     */
    bool create_typescript_path;

    /**
     * Whether existing files should be appended to when creating a new
     * typescript. Disabled by default.
     */
    bool typescript_write_existing;

    /**
     * The path in which the screen recording should be saved, if enabled.
     * If no screen recording should be saved, this will be NULL.
     */
    char* recording_path;

    /**
     * The filename to use for the screen recording, if enabled.
     */
    char* recording_name;

    /**
     * Whether output which is broadcast to each connected client
     * (graphics, streams, etc.) should NOT be included in the session
     * recording. Output is included by default, as it is necessary for any
     * recording which must later be viewable as video.
     */
    bool recording_exclude_output;

    /**
     * Whether changes to mouse state, such as position and buttons pressed
     * or released, should NOT be included in the session recording. Mouse
     * state is included by default, as it is necessary for the mouse
     * cursor to be rendered in any resulting video.
     */
    bool recording_exclude_mouse;

    /**
     * Whether keys pressed and released should be included in the session
     * recording. Key events are NOT included by default within the
     * recording, as doing so has privacy and security implications. Key
     * events can easily contain sensitive information, such as passwords.
     */
    bool recording_include_keys;

    /**
     * Whether clipboard data should be included in the session recording.
     * Clipboard data is NOT included by default within the recording, as
     * doing so has privacy and security implications.
     */
    bool recording_include_clipboard;

    /**
     * Whether the screen recording path should be automatically created if
     * it does not already exist.
     */
    bool create_recording_path;

    /**
     * Whether existing files should be appended to when creating a new
     * recording. Disabled by default.
     */
    bool recording_write_existing;

    /**
     * Whether this connection is read-only, and user input should be
     * dropped.
     */
    bool read_only;

    /**
     * The ASCII code, as an integer, to send when the backspace key is
     * pressed.
     */
    int backspace;

    /**
     * The maximum size of the scrollback buffer in rows.
     */
    int max_scrollback;

    /**
     * The family of codes (e.g. vt100) which will be used when the
     * function and keypad keys are pressed.
     */
    char* func_keys_and_keypad;

    /**
     * The maximum number of bytes to allow within the clipboard.
     */
    int clipboard_buffer_size;

    /**
     * Whether outbound clipboard access should be blocked. If set, it will
     * not be possible to copy data from the terminal to the client using
     * the clipboard.
     */
    bool disable_copy;

    /**
     * Whether inbound clipboard access should be blocked. If set, it will
     * not be possible to paste data from the client to the terminal using
     * the clipboard.
     */
    bool disable_paste;

    /**
     * The terminal emulator type presented to the user (e.g. "xterm" or
     * "linux"), determining the key encoding used. "linux" is used if
     * unspecified.
     */
    char* terminal_type;

    /**
     * Whether a Wake-on-LAN packet should be sent to the database server
     * prior to connecting.
     */
    bool wol_send_packet;

    /**
     * The MAC address to place within the Wake-on-LAN packet.
     */
    char* wol_mac_addr;

    /**
     * The broadcast address to which the Wake-on-LAN packet should be
     * sent.
     */
    char* wol_broadcast_addr;

    /**
     * The UDP port to use when sending the Wake-on-LAN packet.
     */
    unsigned short wol_udp_port;

    /**
     * The number of seconds to wait after sending the Wake-on-LAN packet
     * before attempting to connect to the database server.
     */
    int wol_wait_time;

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

} guac_dbshell_settings;

/**
 * Parses the common (leading) connection arguments of a database protocol
 * plugin into a newly-allocated settings structure. Any database-specific
 * arguments which follow the common arguments must be parsed separately by
 * the plugin.
 *
 * @param user
 *     The user who submitted the given arguments while joining the
 *     connection.
 *
 * @param protocol_args
 *     The NULL-terminated argument list of the plugin, as assigned to
 *     guac_client's args member.
 *
 * @param argc
 *     The number of arguments within the argv array.
 *
 * @param argv
 *     The values of all arguments provided by the user.
 *
 * @param expected_argc
 *     The total number of arguments the plugin expects, including both the
 *     common arguments and any database-specific arguments.
 *
 * @param default_port
 *     The TCP port to use if no port argument is provided.
 *
 * @return
 *     A newly-allocated settings structure which must eventually be freed
 *     with guac_dbshell_settings_free(), or NULL if the arguments fail to
 *     parse.
 */
guac_dbshell_settings* guac_dbshell_settings_parse(guac_user* user,
        const char** protocol_args, int argc, const char** argv,
        int expected_argc, int default_port);

/**
 * Frees the given settings structure, having been previously allocated via
 * guac_dbshell_settings_parse().
 *
 * @param settings
 *     The settings structure to free.
 */
void guac_dbshell_settings_free(guac_dbshell_settings* settings);

#endif
