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

#ifndef GUAC_SSH_SETTINGS_H
#define GUAC_SSH_SETTINGS_H

#include "config.h"

#include <guacamole/user.h>

#include <stdbool.h>

/**
 * The name of the font to use for the terminal if no name is specified.
 */
#define GUAC_SSH_DEFAULT_FONT_NAME "monospace" 

/**
 * The size of the font to use for the terminal if no font size is specified,
 * in points.
 */
#define GUAC_SSH_DEFAULT_FONT_SIZE 12

/**
 * The port to connect to when initiating any SSH connection, if no other port
 * is specified.
 */
#define GUAC_SSH_DEFAULT_PORT "22"

/**
 * The filename to use for the typescript, if not specified.
 */
#define GUAC_SSH_DEFAULT_TYPESCRIPT_NAME "typescript" 

/**
 * Settings for the SSH connection. The values for this structure are parsed
 * from the arguments given during the Guacamole protocol handshake using the
 * guac_ssh_parse_args() function.
 */
typedef struct guac_ssh_settings {

    /**
     * The hostname of the SSH server to connect to.
     */
    char* hostname;

    /**
     * The port of the SSH server to connect to.
     */
    char* port;

    /**
     * The name of the user to login as, if any. If no username is specified,
     * this will be NULL.
     */
    char* username;

    /**
     * The password to give when authenticating, if any. If no password is
     * specified, this will be NULL.
     */
    char* password;

    /**
     * The private key, encoded as base64, if any. If no private key is
     * specified, this will be NULL.
     */
    char* key_base64;

    /**
     * The passphrase to use to decrypt the given private key, if any. If no
     * passphrase is specified, this will be NULL.
     */
    char* key_passphrase;

    /**
     * The command to run instead of the default shell. If a normal shell
     * session is desired, this will be NULL.
     */
    char* command;

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
     * Whether SFTP is enabled.
     */
    bool enable_sftp;

#ifdef ENABLE_SSH_AGENT
    /**
     * Whether the SSH agent is enabled.
     */
    bool enable_agent;
#endif

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
     * Whether the typescript path should be automatically created if it does
     * not already exist.
     */
    bool create_typescript_path;

} guac_ssh_settings;

/**
 * Parses all given args, storing them in a newly-allocated settings object. If
 * the args fail to parse, NULL is returned.
 *
 * @param user
 *     The user who submitted the given arguments while joining the
 *     connection.
 *
 * @param argc
 *     The number of arguments within the argv array.
 *
 * @param argv
 *     The values of all arguments provided by the user.
 *
 * @return
 *     A newly-allocated settings object which must be freed with
 *     guac_ssh_settings_free() when no longer needed. If the arguments fail
 *     to parse, NULL is returned.
 */
guac_ssh_settings* guac_ssh_parse_args(guac_user* user,
        int argc, const char** argv);

/**
 * Frees the given guac_ssh_settings object, having been previously allocated
 * via guac_ssh_parse_args().
 *
 * @param settings
 *     The settings object to free.
 */
void guac_ssh_settings_free(guac_ssh_settings* settings);

/**
 * NULL-terminated array of accepted client args.
 */
extern const char* GUAC_SSH_CLIENT_ARGS[];

#endif

