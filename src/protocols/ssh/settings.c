/*
 * Copyright (C) 2016 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
    "port",
    "username",
    "password",
    "font-name",
    "font-size",
    "enable-sftp",
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
    NULL
};

enum SSH_ARGS_IDX {

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
     * The name of the color scheme to use. Currently valid color schemes are:
     * "black-white", "white-black", "gray-black", and "green-black", each
     * following the "foreground-background" pattern. By default, this will be
     * "gray-black".
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

#ifdef ENABLE_SSH_AGENT
    settings->enable_agent =
        guac_user_parse_args_boolean(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_ENABLE_AGENT, false);
#endif

    /* Read port */
    settings->port =
        guac_user_parse_args_string(user, GUAC_SSH_CLIENT_ARGS, argv,
                IDX_PORT, GUAC_SSH_DEFAULT_PORT);

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

    /* Parsing was successful */
    return settings;

}

void guac_ssh_settings_free(guac_ssh_settings* settings) {

    /* Free network connection information */
    free(settings->hostname);
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

    /* Free typescript settings */
    free(settings->typescript_name);
    free(settings->typescript_path);

    /* Free overall structure */
    free(settings);

}

