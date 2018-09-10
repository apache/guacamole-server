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
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Client plugin arguments */
const char* GUAC_KUBERNETES_CLIENT_ARGS[] = {
    "hostname",
    "port",
    "use-ssl",
    "client-cert-file",
    "client-key-file",
    "ca-cert-file",
    "ignore-cert",
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
    "scrollback",
    NULL
};

enum KUBERNETES_ARGS_IDX {

    /**
     * The hostname to connect to. Required.
     */
    IDX_HOSTNAME,

    /**
     * The port to connect to. Optional.
     */
    IDX_PORT,

    /**
     * Whether SSL/TLS should be used. If omitted, SSL/TLS will not be used.
     */
    IDX_USE_SSL,

    /**
     * The filename of the certificate to use if performing SSL/TLS client
     * authentication to authenticate with the Kubernetes server. If omitted,
     * SSL client authentication will not be performed. 
     */
    IDX_CLIENT_CERT_FILE,

    /**
     * The filename of the key to use if performing SSL/TLS client
     * authentication to authenticate with the Kubernetes server. If omitted,
     * SSL client authentication will not be performed. 
     */
    IDX_CLIENT_KEY_FILE,

    /**
     * The filename of the certificate of the certificate authority that signed
     * the certificate of the Kubernetes server.
     */
    IDX_CA_CERT_FILE,

    /**
     * Whether the certificate used by the Kubernetes server for SSL/TLS should
     * be ignored if it cannot be validated.
     */
    IDX_IGNORE_CERT,

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
     * The maximum size of the scrollback buffer in rows.
     */
    IDX_SCROLLBACK,

    KUBERNETES_ARGS_COUNT
};

guac_kubernetes_settings* guac_kubernetes_parse_args(guac_user* user,
        int argc, const char** argv) {

    /* Validate arg count */
    if (argc != KUBERNETES_ARGS_COUNT) {
        guac_user_log(user, GUAC_LOG_WARNING, "Incorrect number of connection "
                "parameters provided: expected %i, got %i.",
                KUBERNETES_ARGS_COUNT, argc);
        return NULL;
    }

    guac_kubernetes_settings* settings =
        calloc(1, sizeof(guac_kubernetes_settings));

    /* Read parameters */
    settings->hostname =
        guac_user_parse_args_string(user, GUAC_KUBERNETES_CLIENT_ARGS, argv,
                IDX_HOSTNAME, "");

    /* Parse whether SSL should be used */
    settings->use_ssl =
        guac_user_parse_args_boolean(user, GUAC_KUBERNETES_CLIENT_ARGS, argv,
                IDX_USE_SSL, false);

    /* Read SSL/TLS connection details only if enabled */
    if (settings->use_ssl) {

        settings->client_cert_file =
            guac_user_parse_args_string(user, GUAC_KUBERNETES_CLIENT_ARGS,
                    argv, IDX_CLIENT_CERT_FILE, NULL);

        settings->client_key_file =
            guac_user_parse_args_string(user, GUAC_KUBERNETES_CLIENT_ARGS,
                    argv, IDX_CLIENT_KEY_FILE, NULL);

        settings->ca_cert_file =
            guac_user_parse_args_string(user, GUAC_KUBERNETES_CLIENT_ARGS,
                    argv, IDX_CA_CERT_FILE, NULL);

        settings->ignore_cert =
            guac_user_parse_args_boolean(user, GUAC_KUBERNETES_CLIENT_ARGS,
                    argv, IDX_IGNORE_CERT, false);

    }

    /* Read-only mode */
    settings->read_only =
        guac_user_parse_args_boolean(user, GUAC_KUBERNETES_CLIENT_ARGS, argv,
                IDX_READ_ONLY, false);

    /* Read maximum scrollback size */
    settings->max_scrollback =
        guac_user_parse_args_int(user, GUAC_KUBERNETES_CLIENT_ARGS, argv,
                IDX_SCROLLBACK, GUAC_KUBERNETES_DEFAULT_MAX_SCROLLBACK);

    /* Read font name */
    settings->font_name =
        guac_user_parse_args_string(user, GUAC_KUBERNETES_CLIENT_ARGS, argv,
                IDX_FONT_NAME, GUAC_KUBERNETES_DEFAULT_FONT_NAME);

    /* Read font size */
    settings->font_size =
        guac_user_parse_args_int(user, GUAC_KUBERNETES_CLIENT_ARGS, argv,
                IDX_FONT_SIZE, GUAC_KUBERNETES_DEFAULT_FONT_SIZE);

    /* Copy requested color scheme */
    settings->color_scheme =
        guac_user_parse_args_string(user, GUAC_KUBERNETES_CLIENT_ARGS, argv,
                IDX_COLOR_SCHEME, "");

    /* Pull width/height/resolution directly from user */
    settings->width      = user->info.optimal_width;
    settings->height     = user->info.optimal_height;
    settings->resolution = user->info.optimal_resolution;

    /* Read port */
    settings->port =
        guac_user_parse_args_int(user, GUAC_KUBERNETES_CLIENT_ARGS, argv,
                IDX_PORT, GUAC_KUBERNETES_DEFAULT_PORT);

    /* Read typescript path */
    settings->typescript_path =
        guac_user_parse_args_string(user, GUAC_KUBERNETES_CLIENT_ARGS, argv,
                IDX_TYPESCRIPT_PATH, NULL);

    /* Read typescript name */
    settings->typescript_name =
        guac_user_parse_args_string(user, GUAC_KUBERNETES_CLIENT_ARGS, argv,
                IDX_TYPESCRIPT_NAME, GUAC_KUBERNETES_DEFAULT_TYPESCRIPT_NAME);

    /* Parse path creation flag */
    settings->create_typescript_path =
        guac_user_parse_args_boolean(user, GUAC_KUBERNETES_CLIENT_ARGS, argv,
                IDX_CREATE_TYPESCRIPT_PATH, false);

    /* Read recording path */
    settings->recording_path =
        guac_user_parse_args_string(user, GUAC_KUBERNETES_CLIENT_ARGS, argv,
                IDX_RECORDING_PATH, NULL);

    /* Read recording name */
    settings->recording_name =
        guac_user_parse_args_string(user, GUAC_KUBERNETES_CLIENT_ARGS, argv,
                IDX_RECORDING_NAME, GUAC_KUBERNETES_DEFAULT_RECORDING_NAME);

    /* Parse output exclusion flag */
    settings->recording_exclude_output =
        guac_user_parse_args_boolean(user, GUAC_KUBERNETES_CLIENT_ARGS, argv,
                IDX_RECORDING_EXCLUDE_OUTPUT, false);

    /* Parse mouse exclusion flag */
    settings->recording_exclude_mouse =
        guac_user_parse_args_boolean(user, GUAC_KUBERNETES_CLIENT_ARGS, argv,
                IDX_RECORDING_EXCLUDE_MOUSE, false);

    /* Parse key event inclusion flag */
    settings->recording_include_keys =
        guac_user_parse_args_boolean(user, GUAC_KUBERNETES_CLIENT_ARGS, argv,
                IDX_RECORDING_INCLUDE_KEYS, false);

    /* Parse path creation flag */
    settings->create_recording_path =
        guac_user_parse_args_boolean(user, GUAC_KUBERNETES_CLIENT_ARGS, argv,
                IDX_CREATE_RECORDING_PATH, false);

    /* Parse backspace key code */
    settings->backspace =
        guac_user_parse_args_int(user, GUAC_KUBERNETES_CLIENT_ARGS, argv,
                IDX_BACKSPACE, 127);

    /* Parsing was successful */
    return settings;

}

void guac_kubernetes_settings_free(guac_kubernetes_settings* settings) {

    /* Free network connection information */
    free(settings->hostname);

    /* Free SSL/TLS details */
    free(settings->client_cert_file);
    free(settings->client_key_file);
    free(settings->ca_cert_file);

    /* Free display preferences */
    free(settings->font_name);
    free(settings->color_scheme);

    /* Free typescript settings */
    free(settings->typescript_name);
    free(settings->typescript_path);

    /* Free screen recording settings */
    free(settings->recording_name);
    free(settings->recording_path);

    /* Free overall structure */
    free(settings);

}

