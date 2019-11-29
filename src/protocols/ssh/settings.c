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

#include "client.h"
#include "settings.h"

#include <guacamole/user.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Client plugin arguments */
const char* GUAC_SSH_CLIENT_ARGS[] = {
    "hostname",
    "host-key",
    "port",
    "username",
    "password",
    "font-name",
    "font-size",
    "enable-sftp",
    "sftp-root-directory",
    "private-key",
    "passphrase",
#ifdef ENABLE_SSH_AGENT
    "enable-agent",
#endif
    "color-scheme",
    "command",
    "typescript-path",
    "typescript-name",
    "create-typescript-path",
    "recording-path",
    "recording-name",
    "recording-exclude-output",
    "recording-exclude-mouse",
    "recording-include-keys",
    "create-recording-path",
    "read-only",
    "server-alive-interval",
    "backspace",
    "terminal-type",
    "scrollback",
    "locale",
    "timezone",
    "disable-copy",
    "disable-paste",
    NULL
};

enum SSH_ARGS_IDX {

    /**
     * The hostname to connect to. Required.
     */
    IDX_HOSTNAME,

    /**
     * The Base64-encoded public SSH host key.  Optional.
     */
    IDX_HOST_KEY,

    /**
     * The port to connect to. Optional.
     */
    IDX_PORT,

    /**
     * The name of the user to login as. Optional.
     */
    IDX_USERNAME,

    /**
     * The password to use when logging in. Optional.
     */
    IDX_PASSWORD,

    /**
     * The name of the font to use within the terminal.
     */
    IDX_FONT_NAME,

    /**
     * The size of the font to use within the terminal, in points.
     */
    IDX_FONT_SIZE,

    /**
     * Whether SFTP should be enabled.
     */
    IDX_ENABLE_SFTP,

    /**
     * The path of the directory within the SSH server to expose as a
     * filesystem guac_object. If omitted, "/" will be used by default.
     */
    IDX_SFTP_ROOT_DIRECTORY,

    /**
     * The private key to use for authentication, if any.
     */
    IDX_PRIVATE_KEY,

    /**
     * The passphrase required to decrypt the private key, if any.
     */
    IDX_PASSPHRASE,

#ifdef ENABLE_SSH_AGENT
    /**
     * Whether SSH agent forwarding support should be enabled.
     */
    IDX_ENABLE_AGENT,
#endif

    /**
     * The color scheme to use, as a series of semicolon-separated color-value
     * pairs: "background: <color>", "foreground: <color>", or
     * "color<n>: <color>", where <n> is a number from 0 to 255, and <color> is
     * "color<n>" or an X11 color code (e.g. "aqua" or "rgb:12/34/56").
     * The color scheme can also be one of the special values: "black-white",
     * "white-black", "gray-black", or "green-black".
     */
    IDX_COLOR_SCHEME,

    /**
     * The command to run instead if the default shell. If omitted, a normal
     * shell session will be created.
     */
    IDX_COMMAND,

    /**
     * The full absolute path to the directory in which typescripts should be
     * written.
     */
    IDX_TYPESCRIPT_PATH,

    /**
     * The name that should be given to typescripts which are written in the
     * given path. Each typescript will consist of two files: "NAME" and
     * "NAME.timing".
     */
    IDX_TYPESCRIPT_NAME,

    /**
     * Whether the specified typescript path should automatically be created
     * if it does not yet exist.
     */
    IDX_CREATE_TYPESCRIPT_PATH,

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
     * Whether output which is broadcast to each connected client (graphics,
     * streams, etc.) should NOT be included in the session recording. Output
     * is included by default, as it is necessary for any recording which must
     * later be viewable as video.
     */
    IDX_RECORDING_EXCLUDE_OUTPUT,

    /**
     * Whether changes to mouse state, such as position and buttons pressed or
     * released, should NOT be included in the session recording. Mouse state
     * is included by default, as it is necessary for the mouse cursor to be
     * rendered in any resulting video.
     */
    IDX_RECORDING_EXCLUDE_MOUSE,

    /**
     * Whether keys pressed and released should be included in the session
     * recording. Key events are NOT included by default within the recording,
     * as doing so has privacy and security implications.  Including key events
     * may be necessary in certain auditing contexts, but should only be done
     * with caution. Key events can easily contain sensitive information, such
     * as passwords, credit card numbers, etc.
     */
    IDX_RECORDING_INCLUDE_KEYS,

    /**
     * Whether the specified screen recording path should automatically be
     * created if it does not yet exist.
     */
    IDX_CREATE_RECORDING_PATH,

    /**
     * "true" if this connection should be read-only (user input should be
     * dropped), "false" or blank otherwise.
     */
    IDX_READ_ONLY,

    /**
     * Number of seconds between sending alive packets.  A default of 0
     * tells SSH not to send these packets.  A value of 1 is automatically
     * changed by libssh2 to 2 to avoid busy-loop corner cases.
     */
    IDX_SERVER_ALIVE_INTERVAL,

    /**
     * The ASCII code, as an integer, to send for the backspace key, as configured
     * by the SSH connection from the client.  By default this will be 127,
     * the ASCII DELETE code.
     */
    IDX_BACKSPACE,

    /**
     * The terminal emulator type that is passed to the remote system (e.g.
     * "xterm" or "xterm-256color"). "linux" is used if unspecified.
     */
    IDX_TERMINAL_TYPE,

    /**
     * The maximum size of the scrollback buffer in rows.
     */
    IDX_SCROLLBACK,

    /**
     * The locale that should be forwarded to the remote system via the LANG
     * environment variable. By default, no locale is forwarded. This setting
     * will only have an effect if the SSH server allows the LANG environment
     * variable to be set.
     */
    IDX_LOCALE,
     
    /**
     * The timezone that is to be passed to the remote system, via the
     * TZ environment variable.  By default, no timezone is forwarded
     * and the timezone of the remote system will be used.  This
     * setting will only work if the SSH server allows the TZ variable
     * to be set.  Timezones should be in standard IANA format, see:
     * https://en.wikipedia.org/wiki/List_of_tz_database_time_zones
     */
    IDX_TIMEZONE,

    /**
     * Whether outbound clipboard access should be blocked. If set to "true",
     * it will not be possible to copy data from the terminal to the client
     * using the clipboard. By default, clipboard access is not blocked.
     */
    IDX_DISABLE_COPY,

    /**
     * Whether inbound clipboard access should be blocked. If set to "true", it
     * will not be possible to paste data from the client to the terminal using
     * the clipboard. By default, clipboard access is not blocked.
     */
    IDX_DISABLE_PASTE,

    SSH_ARGS_COUNT
};

guac_ssh_settings* guac_ssh_parse_args(guac_user* user,
        int argc, const char** argv) {

    /* Validate arg count */
    if (argc != SSH_ARGS_COUNT) {
        guac_user_log(user, GUAC_LOG_WARNING, "Incorrect number of connection "
                "parameters provided: expected %i, got %i.",
                SSH_ARGS_COUNT, argc);
        return NULL;
    }

    guac_ssh_settings* settings = calloc(1, sizeof(guac_ssh_settings));

    /* Read parameters */
    settings->hostname =
        guac_user_parse_args_string(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_HOSTNAME, "");

    settings->host_key =
        guac_user_parse_args_string(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_HOST_KEY, NULL);

    settings->username =
        guac_user_parse_args_string(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_USERNAME, NULL);

    settings->password =
        guac_user_parse_args_string(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_PASSWORD, NULL);

    /* Init public key auth information */
    settings->key_base64 =
        guac_user_parse_args_string(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_PRIVATE_KEY, NULL);

    settings->key_passphrase =
        guac_user_parse_args_string(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_PASSPHRASE, NULL);

    /* Read maximum scrollback size */
    settings->max_scrollback =
        guac_user_parse_args_int(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_SCROLLBACK, GUAC_SSH_DEFAULT_MAX_SCROLLBACK);

    /* Read font name */
    settings->font_name =
        guac_user_parse_args_string(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_FONT_NAME, GUAC_SSH_DEFAULT_FONT_NAME);

    /* Read font size */
    settings->font_size =
        guac_user_parse_args_int(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_FONT_SIZE, GUAC_SSH_DEFAULT_FONT_SIZE);

    /* Copy requested color scheme */
    settings->color_scheme =
        guac_user_parse_args_string(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_COLOR_SCHEME, "");

    /* Pull width/height/resolution directly from user */
    settings->width      = user->info.optimal_width;
    settings->height     = user->info.optimal_height;
    settings->resolution = user->info.optimal_resolution;

    /* Parse SFTP enable */
    settings->enable_sftp =
        guac_user_parse_args_boolean(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_ENABLE_SFTP, false);

    /* SFTP root directory */
    settings->sftp_root_directory =
        guac_user_parse_args_string(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_SFTP_ROOT_DIRECTORY, "/");

#ifdef ENABLE_SSH_AGENT
    settings->enable_agent =
        guac_user_parse_args_boolean(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_ENABLE_AGENT, false);
#endif

    /* Read port */
    settings->port =
        guac_user_parse_args_string(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_PORT, GUAC_SSH_DEFAULT_PORT);

    /* Read-only mode */
    settings->read_only =
        guac_user_parse_args_boolean(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_READ_ONLY, false);

    /* Read command, if any */
    settings->command =
        guac_user_parse_args_string(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_COMMAND, NULL);

    /* Read typescript path */
    settings->typescript_path =
        guac_user_parse_args_string(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_TYPESCRIPT_PATH, NULL);

    /* Read typescript name */
    settings->typescript_name =
        guac_user_parse_args_string(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_TYPESCRIPT_NAME, GUAC_SSH_DEFAULT_TYPESCRIPT_NAME);

    /* Parse path creation flag */
    settings->create_typescript_path =
        guac_user_parse_args_boolean(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_CREATE_TYPESCRIPT_PATH, false);

    /* Read recording path */
    settings->recording_path =
        guac_user_parse_args_string(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_RECORDING_PATH, NULL);

    /* Read recording name */
    settings->recording_name =
        guac_user_parse_args_string(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_RECORDING_NAME, GUAC_SSH_DEFAULT_RECORDING_NAME);

    /* Parse output exclusion flag */
    settings->recording_exclude_output =
        guac_user_parse_args_boolean(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_RECORDING_EXCLUDE_OUTPUT, false);

    /* Parse mouse exclusion flag */
    settings->recording_exclude_mouse =
        guac_user_parse_args_boolean(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_RECORDING_EXCLUDE_MOUSE, false);

    /* Parse key event inclusion flag */
    settings->recording_include_keys =
        guac_user_parse_args_boolean(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_RECORDING_INCLUDE_KEYS, false);

    /* Parse path creation flag */
    settings->create_recording_path =
        guac_user_parse_args_boolean(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_CREATE_RECORDING_PATH, false);

    /* Parse server alive interval */
    settings->server_alive_interval =
        guac_user_parse_args_int(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_SERVER_ALIVE_INTERVAL, 0);

    /* Parse backspace key setting */
    settings->backspace =
        guac_user_parse_args_int(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_BACKSPACE, 127);

    /* Read terminal emulator type. */
    settings->terminal_type =
        guac_user_parse_args_string(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_TERMINAL_TYPE, "linux");

    /* Read locale */
    settings->locale =
        guac_user_parse_args_string(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_LOCALE, NULL);

    /* Read the timezone parameter, or use client handshake. */
    settings->timezone =
        guac_user_parse_args_string(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_TIMEZONE, user->info.timezone);

    /* Parse clipboard copy disable flag */
    settings->disable_copy =
        guac_user_parse_args_boolean(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_DISABLE_COPY, false);

    /* Parse clipboard paste disable flag */
    settings->disable_paste =
        guac_user_parse_args_boolean(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_DISABLE_PASTE, false);

    /* Parsing was successful */
    return settings;

}

void guac_ssh_settings_free(guac_ssh_settings* settings) {

    /* Free network connection information */
    free(settings->hostname);
    free(settings->host_key);
    free(settings->port);

    /* Free credentials */
    free(settings->username);
    free(settings->password);
    free(settings->key_base64);
    free(settings->key_passphrase);

    /* Free display preferences */
    free(settings->font_name);
    free(settings->color_scheme);

    /* Free requested command */
    free(settings->command);

    /* Free SFTP settings */
    free(settings->sftp_root_directory);

    /* Free typescript settings */
    free(settings->typescript_name);
    free(settings->typescript_path);

    /* Free screen recording settings */
    free(settings->recording_name);
    free(settings->recording_path);

    /* Free terminal emulator type. */
    free(settings->terminal_type);

    /* Free locale */
    free(settings->locale);

    /* Free the client timezone. */
    free(settings->timezone);

    /* Free overall structure */
    free(settings);

}
