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

#ifndef GUAC_DBSHELL_CLIENT_H
#define GUAC_DBSHELL_CLIENT_H

/**
 * Declarations for the generic protocol plugin scaffolding shared by all
 * database protocol plugins. A plugin's guac_client_init() implementation
 * needs only to invoke guac_dbshell_client_init() with its plugin
 * definition; all user, input, clipboard, pipe, and lifecycle handling is
 * provided here.
 *
 * @file client.h
 */

#include "dbshell.h"
#include "driver.h"
#include "settings.h"

#include <guacamole/client.h>
#include <guacamole/recording.h>
#include <guacamole/user.h>
#include <terminal/terminal.h>

#include <pthread.h>

/**
 * The name of the inbound pipe stream which, when opened by a user,
 * redirects the terminal's STDIN to that stream.
 */
#define GUAC_DBSHELL_STDIN_PIPE_NAME "STDIN"

/**
 * The definition of a database protocol plugin: the driver implementing
 * database-specific behavior, together with the plugin's argument list and
 * defaults.
 */
typedef struct guac_dbshell_plugin {

    /**
     * The driver implementing database-specific behavior.
     */
    const guac_dbshell_driver* driver;

    /**
     * The NULL-terminated argument list of the plugin. The list must begin
     * with the arguments declared by GUAC_DBSHELL_COMMON_ARGS, optionally
     * followed by database-specific arguments.
     */
    const char** args;

    /**
     * The number of arguments within the argument list, excluding the
     * NULL terminator.
     */
    int argc;

    /**
     * The TCP port to use if no port argument is provided.
     */
    int default_port;

    /**
     * Parses the database-specific arguments of the plugin (the arguments
     * at indices GUAC_DBSHELL_COMMON_ARG_COUNT and beyond), returning a
     * newly-allocated, driver-defined settings structure. If the plugin
     * has no database-specific arguments, this member may be NULL.
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
     *     A newly-allocated, driver-defined settings structure, or NULL if
     *     the plugin allocates none.
     */
    void* (*parse_extra_args)(guac_user* user, int argc, const char** argv);

    /**
     * Frees the driver-defined settings structure returned by
     * parse_extra_args(). If parse_extra_args is NULL, this member may
     * also be NULL.
     *
     * @param extra
     *     The driver-defined settings structure to free.
     */
    void (*free_extra_args)(void* extra);

} guac_dbshell_plugin;

/**
 * The data associated with a database protocol guac_client, assigned to
 * that client's data member.
 */
typedef struct guac_dbshell_client {

    /**
     * The plugin definition of the protocol.
     */
    const guac_dbshell_plugin* plugin;

    /**
     * The common settings of the connection owner, or NULL if the owner
     * has not yet joined.
     */
    guac_dbshell_settings* settings;

    /**
     * The driver-defined settings of the connection owner, or NULL if the
     * plugin defines none.
     */
    void* extra_settings;

    /**
     * The terminal which serves as the display and input source of the
     * session, or NULL if the terminal has not yet been created.
     */
    guac_terminal* term;

    /**
     * The in-progress session recording, or NULL if no recording is in
     * progress.
     */
    guac_recording* recording;

    /**
     * The database session, or NULL if the session has not yet been
     * created.
     */
    guac_dbshell_session* session;

    /**
     * The thread which runs the database session.
     */
    pthread_t client_thread;

} guac_dbshell_client;

/**
 * The per-user data of a database protocol connection, assigned to each
 * guac_user's data member upon join.
 */
typedef struct guac_dbshell_user {

    /**
     * The common settings parsed from the arguments the user provided.
     */
    guac_dbshell_settings* settings;

    /**
     * The driver-defined settings parsed from the arguments the user
     * provided, or NULL if the plugin defines none.
     */
    void* extra_settings;

} guac_dbshell_user;

/**
 * Initializes the given guac_client as a database protocol client using
 * the given plugin definition. This function provides the complete
 * implementation of a database protocol plugin's guac_client_init().
 *
 * @param client
 *     The guac_client to initialize.
 *
 * @param plugin
 *     The plugin definition. This structure must have static storage
 *     duration.
 *
 * @return
 *     Zero on success, non-zero on failure.
 */
int guac_dbshell_client_init(guac_client* client,
        const guac_dbshell_plugin* plugin);

/**
 * Prompts the user for their username and/or password using the terminal
 * of the given session, storing the entered values within the session's
 * common settings. Values which are already present within the settings
 * are not prompted for. Password input is not echoed.
 *
 * @param session
 *     The session whose settings should be completed.
 *
 * @param need_username
 *     Whether a username is required for the database being connected to.
 *
 * @param need_password
 *     Whether a password is required for the database being connected to.
 */
void guac_dbshell_prompt_credentials(guac_dbshell_session* session,
        bool need_username, bool need_password);

#endif
