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

#ifndef GUAC_DBSHELL_DRIVER_H
#define GUAC_DBSHELL_DRIVER_H

/**
 * Declarations for the driver interface which database protocol plugins
 * implement to provide database-specific behavior to the shared dbshell
 * REPL.
 *
 * @file driver.h
 */

#include "splitter.h"

typedef struct guac_dbshell_session guac_dbshell_session;

/**
 * Return value of a driver's execute handler indicating that the statement
 * completed, successfully or not, and the session may continue.
 */
#define GUAC_DBSHELL_EXECUTE_OK 0

/**
 * Return value of a driver's execute handler indicating that the connection
 * to the database server has been lost and the session must end.
 */
#define GUAC_DBSHELL_EXECUTE_LOST 1

/**
 * The set of handlers and properties which define database-specific behavior
 * of an interactive database session. Each database protocol plugin defines
 * exactly one static instance of this structure.
 *
 * All strings received by and returned from these handlers are UTF-8.
 */
typedef struct guac_dbshell_driver {

    /**
     * The human-readable, lowercase name of the database protocol, as used
     * within the session prompt and log messages, for example "mysql".
     */
    const char* name;

    /**
     * The statement-splitting dialect of the database, one of the
     * GUAC_DBSHELL_DIALECT_* constants.
     */
    guac_dbshell_dialect dialect;

    /**
     * Establishes the connection to the database server described by the
     * settings of the given session, assigning the resulting
     * connection handle to the session's driver_data. On failure, this
     * handler is responsible for rendering a human-readable error to the
     * session's terminal (or aborting the client), and any partially-
     * allocated resources must be released.
     *
     * @param session
     *     The session on whose behalf the connection is being established.
     *
     * @return
     *     Zero on success, non-zero on failure.
     */
    int (*connect_handler)(guac_dbshell_session* session);

    /**
     * Closes the connection to the database server and frees the connection
     * handle stored within the session's driver_data. This handler is
     * invoked exactly once for each successful invocation of the connect
     * handler.
     *
     * @param session
     *     The session whose connection should be closed.
     */
    void (*disconnect_handler)(guac_dbshell_session* session);

    /**
     * Executes the given single statement against the database server,
     * rendering all result sets, row counts, and errors to the session's
     * terminal. Statement errors are NOT failures of this handler; the
     * handler fails only if the connection itself is lost.
     *
     * @param session
     *     The session on whose behalf the statement is being executed.
     *
     * @param statement
     *     The statement to execute, stripped of its terminator.
     *
     * @return
     *     GUAC_DBSHELL_EXECUTE_OK if the session may continue, or
     *     GUAC_DBSHELL_EXECUTE_LOST if the connection to the database server
     *     has been lost.
     */
    int (*execute_handler)(guac_dbshell_session* session,
            const char* statement);

    /**
     * Interrupts the statement currently being executed by the execute
     * handler, causing it to return as soon as possible. This handler is
     * invoked from a different thread than the thread running the execute
     * handler and must be thread-safe with respect to it. If the underlying
     * database client library provides no thread-safe cancellation
     * mechanism, this member may be NULL, in which case cancellation
     * requests are ignored.
     *
     * @param session
     *     The session whose current statement should be interrupted.
     */
    void (*cancel_handler)(guac_dbshell_session* session);

    /**
     * Handles the given driver-specific meta-command (a command beginning
     * with a backslash which is interpreted by the shell itself rather than
     * sent to the database server), if the driver recognizes it. The command
     * is provided without its leading backslash. If the driver does not
     * provide any additional meta-commands, this member may be NULL.
     *
     * @param session
     *     The session on whose behalf the meta-command is being handled.
     *
     * @param command
     *     The meta-command to handle, without its leading backslash, for
     *     example "use mydb".
     *
     * @return
     *     Non-zero if the meta-command was recognized and handled, zero if
     *     the command is not recognized by the driver.
     */
    int (*meta_handler)(guac_dbshell_session* session, const char* command);

    /**
     * Writes driver-specific lines to the session's terminal as part of the
     * help text produced by the \h meta-command, if any. If the driver adds
     * no meta-commands of its own, this member may be NULL.
     *
     * @param session
     *     The session whose terminal should receive the help text.
     */
    void (*help_handler)(guac_dbshell_session* session);

} guac_dbshell_driver;

#endif
