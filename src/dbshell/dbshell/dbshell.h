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

#ifndef GUAC_DBSHELL_H
#define GUAC_DBSHELL_H

/**
 * Core declarations for the database shell (dbshell) library, a shared
 * implementation of an interactive, terminal-based REPL (read-eval-print
 * loop) for database protocol plugins. The dbshell library provides all
 * database-agnostic functionality (line editing, command history, statement
 * splitting, result table rendering, and the REPL itself), while each
 * database protocol plugin provides a driver implementation which performs
 * the actual communication with the database server.
 *
 * @file dbshell.h
 */

#include "driver.h"
#include "history.h"

#include <guacamole/client.h>
#include <terminal/terminal.h>

#include <pthread.h>
#include <stdbool.h>

/**
 * The maximum number of statements retained within the in-memory command
 * history of a database session. Older statements are discarded as new
 * statements are submitted. History is never persisted.
 */
#define GUAC_DBSHELL_HISTORY_SIZE 100

/**
 * The maximum length of the prompt string displayed at the beginning of each
 * input line, in bytes, including the null terminator.
 */
#define GUAC_DBSHELL_MAX_PROMPT_LENGTH 64

/**
 * The state of an interactive database session, shared between the REPL, the
 * driver implementation of the database protocol plugin using that REPL, and
 * the input handlers of that plugin.
 */
typedef struct guac_dbshell_session {

    /**
     * The guac_client associated with the database session, for logging and
     * lifecycle purposes.
     */
    guac_client* client;

    /**
     * The terminal emulator which serves as the display and input source of
     * the database session.
     */
    guac_terminal* term;

    /**
     * The driver implementing database-specific behavior for this session.
     */
    const guac_dbshell_driver* driver;

    /**
     * Arbitrary data assigned by the driver, typically the connection handle
     * of the underlying database client library. This value is assigned by
     * the driver's connect handler and must be freed/released by the
     * driver's disconnect handler.
     */
    void* driver_data;

    /**
     * The settings structure of the protocol plugin which created this
     * session. The dbshell library does not interpret this value; it exists
     * solely for the benefit of the driver.
     */
    void* settings;

    /**
     * The in-memory history of previously-submitted statements.
     */
    guac_dbshell_history* history;

    /**
     * Lock which protects the executing flag, guaranteeing that
     * cancellation requests observe a consistent view of whether a
     * statement is currently being executed.
     */
    pthread_mutex_t execute_lock;

    /**
     * Whether the driver's execute handler is currently running. This flag
     * is set by the REPL around each execute call and tested by
     * guac_dbshell_session_cancel() to determine whether a cancellation
     * request should be forwarded to the driver.
     */
    bool executing;

} guac_dbshell_session;

/**
 * Allocates a new database session which will run within the given terminal
 * on behalf of the given client, using the given driver. The returned
 * session must eventually be freed with guac_dbshell_session_free().
 *
 * @param client
 *     The guac_client associated with the database session.
 *
 * @param term
 *     The terminal emulator which should serve as the display and input
 *     source of the database session.
 *
 * @param driver
 *     The driver implementing database-specific behavior for the session.
 *
 * @param settings
 *     The settings structure of the protocol plugin creating the session,
 *     to be interpreted only by the driver.
 *
 * @return
 *     A newly-allocated database session, or NULL if the session could not
 *     be allocated.
 */
guac_dbshell_session* guac_dbshell_session_alloc(guac_client* client,
        guac_terminal* term, const guac_dbshell_driver* driver,
        void* settings);

/**
 * Frees all resources associated with the given database session. The
 * driver's disconnect handler is NOT invoked by this function; callers must
 * disconnect first if a connection was established.
 *
 * @param session
 *     The session to free.
 */
void guac_dbshell_session_free(guac_dbshell_session* session);

/**
 * Requests cancellation of the statement currently being executed by the
 * given session, if any. This function is safe to call from threads other
 * than the thread running the REPL, and does nothing if no statement is
 * currently being executed or if the driver does not support cancellation.
 *
 * @param session
 *     The session whose current statement should be cancelled.
 */
void guac_dbshell_session_cancel(guac_dbshell_session* session);

/**
 * Runs the interactive REPL of the given session until the user exits, the
 * terminal is stopped, or the connection to the database server is lost.
 * The driver's connect handler must have completed successfully before this
 * function is invoked. This function must be called from the protocol
 * plugin's client thread.
 *
 * @param session
 *     The session whose REPL should run.
 *
 * @return
 *     Zero if the REPL terminated normally (user exit or terminal stop), or
 *     non-zero if the REPL terminated because the connection to the database
 *     server was lost.
 */
int guac_dbshell_repl_run(guac_dbshell_session* session);

/**
 * Writes the given printf-style message to the session's terminal, followed
 * by a CRLF line ending. This is a convenience wrapper for drivers rendering
 * errors and informational messages.
 *
 * @param session
 *     The session whose terminal should receive the message.
 *
 * @param format
 *     A printf-style format string.
 *
 * @param ...
 *     Any arguments to use when filling the format string.
 */
void guac_dbshell_println(guac_dbshell_session* session,
        const char* format, ...);

/**
 * Writes a standard summary line to the session's terminal describing the
 * outcome of a statement, in the style of common database CLIs, for example
 * "3 rows in set (0.02 sec)" or "1 row affected (0.01 sec)".
 *
 * @param session
 *     The session whose terminal should receive the summary.
 *
 * @param rows
 *     The number of rows returned or affected by the statement.
 *
 * @param affected
 *     Whether the given row count refers to rows affected by the statement
 *     (non-zero) rather than rows returned in a result set (zero).
 *
 * @param millis
 *     The wall-clock duration of the statement, in milliseconds.
 */
void guac_dbshell_print_summary(guac_dbshell_session* session,
        unsigned long rows, int affected, long millis);

#endif
