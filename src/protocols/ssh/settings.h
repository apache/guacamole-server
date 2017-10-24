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
 * The filename to use for the screen recording, if not specified.
 */
#define GUAC_SSH_DEFAULT_RECORDING_NAME "recording"

/**
 * The default polling timeout for SSH activity in milliseconds.
 */
#define GUAC_SSH_DEFAULT_POLL_TIMEOUT 1000

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
     * Whether this connection is read-only, and user input should be dropped.
     */
    bool read_only;

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

    /**
     * The path of the directory within the SSH server to expose as a
     * filesystem guac_object.
     */
    char* sftp_root_directory;

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

    /**
     * The path in which the screen recording should be saved, if enabled. If
     * no screen recording should be saved, this will be NULL.
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
     * The number of seconds between sending server alive messages.
     */
    int server_alive_interval;

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

