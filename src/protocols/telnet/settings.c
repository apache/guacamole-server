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

#include "settings.h"

#include <guacamole/user.h>

#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Client plugin arguments */
const char* GUAC_TELNET_CLIENT_ARGS[] = {
    "hostname",
    "port",
    "username",
    "username-regex",
    "password",
    "password-regex",
    "font-name",
    "font-size",
    "color-scheme",
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
    "backspace",
    "terminal-type",
    "scrollback",
    "login-success-regex",
    "login-failure-regex",
    "disable-copy",
    "disable-paste",
    NULL
};

enum TELNET_ARGS_IDX {
    
    /**
     * The hostname to connect to. Required.
     */
    IDX_HOSTNAME,

    /**
     * The port to connect to. Optional.
     */
    IDX_PORT,

    /**
     * The name of the user to login as. Optional.
     */
    IDX_USERNAME,

    /**
     * The regular expression to use when searching for the username/login
     * prompt. Optional.
     */
    IDX_USERNAME_REGEX,

    /**
     * The password to use when logging in. Optional.
     */
    IDX_PASSWORD,

    /**
     * The regular expression to use when searching for the password prompt.
     * Optional.
     */
    IDX_PASSWORD_REGEX,

    /**
     * The name of the font to use within the terminal.
     */
    IDX_FONT_NAME,

    /**
     * The size of the font to use within the terminal, in points.
     */
    IDX_FONT_SIZE,

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
     * ASCII code, as an integer to use for the backspace key, or 127
     * if not specified.
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
     * The regular expression to use when searching for whether login was
     * successful. This parameter is optional. If given, the
     * "login-failure-regex" parameter must also be specified, and the first
     * frame of the Guacamole connection will be withheld until login
     * success/failure has been determined.
     */
    IDX_LOGIN_SUCCESS_REGEX,

    /**
     * The regular expression to use when searching for whether login was
     * unsuccessful. This parameter is optional. If given, the
     * "login-success-regex" parameter must also be specified, and the first
     * frame of the Guacamole connection will be withheld until login
     * success/failure has been determined.
     */
    IDX_LOGIN_FAILURE_REGEX,

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

    TELNET_ARGS_COUNT
};

/**
 * Compiles the given regular expression, returning NULL if compilation fails
 * or of the given regular expression is NULL. The returned regex_t must be
 * freed with regfree() AND free(), or with guac_telnet_regex_free().
 *
 * @param user
 *     The user who provided the setting associated with the given regex
 *     pattern. Error messages will be logged on behalf of this user.
 *
 * @param pattern
 *     The regular expression pattern to compile.
 *
 * @return
 *     The compiled regular expression, or NULL if compilation fails or NULL
 *     was originally provided for the pattern.
 */
static regex_t* guac_telnet_compile_regex(guac_user* user, char* pattern) {

    /* Nothing to compile if no pattern provided */
    if (pattern == NULL)
        return NULL;

    int compile_result;
    regex_t* regex = malloc(sizeof(regex_t));

    /* Compile regular expression */
    compile_result = regcomp(regex, pattern,
            REG_EXTENDED | REG_NOSUB | REG_ICASE | REG_NEWLINE);

    /* Notify of failure to parse/compile */
    if (compile_result != 0) {
        guac_user_log(user, GUAC_LOG_ERROR, "Regular expression '%s' "
                "could not be compiled.", pattern);
        free(regex);
        return NULL;
    }

    return regex;
}

void guac_telnet_regex_free(regex_t** regex) {
    if (*regex != NULL) {
        regfree(*regex);
        free(*regex);
        *regex = NULL;
    }
}

guac_telnet_settings* guac_telnet_parse_args(guac_user* user,
        int argc, const char** argv) {

    /* Validate arg count */
    if (argc != TELNET_ARGS_COUNT) {
        guac_user_log(user, GUAC_LOG_WARNING, "Incorrect number of connection "
                "parameters provided: expected %i, got %i.",
                TELNET_ARGS_COUNT, argc);
        return NULL;
    }

    guac_telnet_settings* settings = calloc(1, sizeof(guac_telnet_settings));

    /* Read parameters */
    settings->hostname =
        guac_user_parse_args_string(user, GUAC_TELNET_CLIENT_ARGS, argv,
                IDX_HOSTNAME, "");

    /* Read username */
    settings->username =
        guac_user_parse_args_string(user, GUAC_TELNET_CLIENT_ARGS, argv,
                IDX_USERNAME, NULL);

    /* Read username regex only if username is specified */
    if (settings->username != NULL) {
        settings->username_regex = guac_telnet_compile_regex(user,
            guac_user_parse_args_string(user, GUAC_TELNET_CLIENT_ARGS, argv,
                    IDX_USERNAME_REGEX, GUAC_TELNET_DEFAULT_USERNAME_REGEX));
    }

    /* Read password */
    settings->password =
        guac_user_parse_args_string(user, GUAC_TELNET_CLIENT_ARGS, argv,
                IDX_PASSWORD, NULL);

    /* Read password regex only if password is specified */
    if (settings->password != NULL) {
        settings->password_regex = guac_telnet_compile_regex(user,
            guac_user_parse_args_string(user, GUAC_TELNET_CLIENT_ARGS, argv,
                    IDX_PASSWORD_REGEX, GUAC_TELNET_DEFAULT_PASSWORD_REGEX));
    }

    /* Read optional login success detection regex */
    settings->login_success_regex = guac_telnet_compile_regex(user,
            guac_user_parse_args_string(user, GUAC_TELNET_CLIENT_ARGS, argv,
                    IDX_LOGIN_SUCCESS_REGEX, NULL));

    /* Read optional login failure detection regex */
    settings->login_failure_regex = guac_telnet_compile_regex(user,
            guac_user_parse_args_string(user, GUAC_TELNET_CLIENT_ARGS, argv,
                    IDX_LOGIN_FAILURE_REGEX, NULL));

    /* Both login success and login failure regexes must be provided if either
     * is present at all */
    if (settings->login_success_regex != NULL
            && settings->login_failure_regex == NULL) {
        guac_telnet_regex_free(&settings->login_success_regex);
        guac_user_log(user, GUAC_LOG_WARNING, "Ignoring provided value for "
                "\"%s\" as \"%s\" must also be provided.",
                GUAC_TELNET_CLIENT_ARGS[IDX_LOGIN_SUCCESS_REGEX],
                GUAC_TELNET_CLIENT_ARGS[IDX_LOGIN_FAILURE_REGEX]);
    }
    else if (settings->login_failure_regex != NULL
            && settings->login_success_regex == NULL) {
        guac_telnet_regex_free(&settings->login_failure_regex);
        guac_user_log(user, GUAC_LOG_WARNING, "Ignoring provided value for "
                "\"%s\" as \"%s\" must also be provided.",
                GUAC_TELNET_CLIENT_ARGS[IDX_LOGIN_FAILURE_REGEX],
                GUAC_TELNET_CLIENT_ARGS[IDX_LOGIN_SUCCESS_REGEX]);
    }

    /* Read-only mode */
    settings->read_only =
        guac_user_parse_args_boolean(user, GUAC_TELNET_CLIENT_ARGS, argv,
                IDX_READ_ONLY, false);

    /* Read maximum scrollback size */
    settings->max_scrollback =
        guac_user_parse_args_int(user, GUAC_TELNET_CLIENT_ARGS, argv,
                IDX_SCROLLBACK, GUAC_TELNET_DEFAULT_MAX_SCROLLBACK);

    /* Read font name */
    settings->font_name =
        guac_user_parse_args_string(user, GUAC_TELNET_CLIENT_ARGS, argv,
                IDX_FONT_NAME, GUAC_TELNET_DEFAULT_FONT_NAME);

    /* Read font size */
    settings->font_size =
        guac_user_parse_args_int(user, GUAC_TELNET_CLIENT_ARGS, argv,
                IDX_FONT_SIZE, GUAC_TELNET_DEFAULT_FONT_SIZE);

    /* Copy requested color scheme */
    settings->color_scheme =
        guac_user_parse_args_string(user, GUAC_TELNET_CLIENT_ARGS, argv,
                IDX_COLOR_SCHEME, "");

    /* Pull width/height/resolution directly from user */
    settings->width      = user->info.optimal_width;
    settings->height     = user->info.optimal_height;
    settings->resolution = user->info.optimal_resolution;

    /* Read port */
    settings->port =
        guac_user_parse_args_string(user, GUAC_TELNET_CLIENT_ARGS, argv,
                IDX_PORT, GUAC_TELNET_DEFAULT_PORT);

    /* Read typescript path */
    settings->typescript_path =
        guac_user_parse_args_string(user, GUAC_TELNET_CLIENT_ARGS, argv,
                IDX_TYPESCRIPT_PATH, NULL);

    /* Read typescript name */
    settings->typescript_name =
        guac_user_parse_args_string(user, GUAC_TELNET_CLIENT_ARGS, argv,
                IDX_TYPESCRIPT_NAME, GUAC_TELNET_DEFAULT_TYPESCRIPT_NAME);

    /* Parse path creation flag */
    settings->create_typescript_path =
        guac_user_parse_args_boolean(user, GUAC_TELNET_CLIENT_ARGS, argv,
                IDX_CREATE_TYPESCRIPT_PATH, false);

    /* Read recording path */
    settings->recording_path =
        guac_user_parse_args_string(user, GUAC_TELNET_CLIENT_ARGS, argv,
                IDX_RECORDING_PATH, NULL);

    /* Read recording name */
    settings->recording_name =
        guac_user_parse_args_string(user, GUAC_TELNET_CLIENT_ARGS, argv,
                IDX_RECORDING_NAME, GUAC_TELNET_DEFAULT_RECORDING_NAME);

    /* Parse output exclusion flag */
    settings->recording_exclude_output =
        guac_user_parse_args_boolean(user, GUAC_TELNET_CLIENT_ARGS, argv,
                IDX_RECORDING_EXCLUDE_OUTPUT, false);

    /* Parse mouse exclusion flag */
    settings->recording_exclude_mouse =
        guac_user_parse_args_boolean(user, GUAC_TELNET_CLIENT_ARGS, argv,
                IDX_RECORDING_EXCLUDE_MOUSE, false);

    /* Parse key event inclusion flag */
    settings->recording_include_keys =
        guac_user_parse_args_boolean(user, GUAC_TELNET_CLIENT_ARGS, argv,
                IDX_RECORDING_INCLUDE_KEYS, false);

    /* Parse path creation flag */
    settings->create_recording_path =
        guac_user_parse_args_boolean(user, GUAC_TELNET_CLIENT_ARGS, argv,
                IDX_CREATE_RECORDING_PATH, false);

    /* Parse backspace key code */
    settings->backspace =
        guac_user_parse_args_int(user, GUAC_TELNET_CLIENT_ARGS, argv,
                IDX_BACKSPACE, 127);

    /* Read terminal emulator type. */
    settings->terminal_type =
        guac_user_parse_args_string(user, GUAC_TELNET_CLIENT_ARGS, argv,
                IDX_TERMINAL_TYPE, "linux");

    /* Parse clipboard copy disable flag */
    settings->disable_copy =
        guac_user_parse_args_boolean(user, GUAC_TELNET_CLIENT_ARGS, argv,
                IDX_DISABLE_COPY, false);

    /* Parse clipboard paste disable flag */
    settings->disable_paste =
        guac_user_parse_args_boolean(user, GUAC_TELNET_CLIENT_ARGS, argv,
                IDX_DISABLE_PASTE, false);

    /* Parsing was successful */
    return settings;

}

void guac_telnet_settings_free(guac_telnet_settings* settings) {

    /* Free network connection information */
    free(settings->hostname);
    free(settings->port);

    /* Free credentials */
    free(settings->username);
    free(settings->password);

    /* Free various regexes */
    guac_telnet_regex_free(&settings->username_regex);
    guac_telnet_regex_free(&settings->password_regex);
    guac_telnet_regex_free(&settings->login_success_regex);
    guac_telnet_regex_free(&settings->login_failure_regex);

    /* Free display preferences */
    free(settings->font_name);
    free(settings->color_scheme);

    /* Free typescript settings */
    free(settings->typescript_name);
    free(settings->typescript_path);

    /* Free screen recording settings */
    free(settings->recording_name);
    free(settings->recording_path);

    /* Free terminal emulator type. */
    free(settings->terminal_type);

    /* Free overall structure */
    free(settings);

}

