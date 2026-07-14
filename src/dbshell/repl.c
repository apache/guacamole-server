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

#include "dbshell/dbshell.h"
#include "dbshell/driver.h"
#include "dbshell/history.h"
#include "dbshell/line-editor.h"
#include "dbshell/splitter.h"

#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <guacamole/string.h>
#include <terminal/terminal.h>

#include <ctype.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

guac_dbshell_session* guac_dbshell_session_alloc(guac_client* client,
        guac_terminal* term, const guac_dbshell_driver* driver,
        void* settings) {

    guac_dbshell_session* session =
        guac_mem_zalloc(sizeof(guac_dbshell_session));

    session->client = client;
    session->term = term;
    session->driver = driver;
    session->settings = settings;
    session->history =
        guac_dbshell_history_alloc(GUAC_DBSHELL_HISTORY_SIZE);

    pthread_mutex_init(&session->execute_lock, NULL);

    return session;

}

void guac_dbshell_session_free(guac_dbshell_session* session) {

    if (session == NULL)
        return;

    guac_dbshell_history_free(session->history);
    pthread_mutex_destroy(&session->execute_lock);
    guac_mem_free(session);

}

void guac_dbshell_session_cancel(guac_dbshell_session* session) {

    pthread_mutex_lock(&session->execute_lock);

    /* Forward the request only if a statement is genuinely executing and
     * the driver supports cancellation */
    if (session->executing && session->driver->cancel_handler != NULL)
        session->driver->cancel_handler(session);

    pthread_mutex_unlock(&session->execute_lock);

}

void guac_dbshell_println(guac_dbshell_session* session,
        const char* format, ...) {

    va_list args;

    /* Measure the message */
    va_start(args, format);
    int length = vsnprintf(NULL, 0, format, args);
    va_end(args);

    if (length < 0)
        return;

    /* Format the message */
    char* message = guac_mem_alloc(guac_mem_ckd_add_or_die(length, 1));
    va_start(args, format);
    vsnprintf(message, length + 1, format, args);
    va_end(args);

    guac_terminal_write(session->term, message, length);
    guac_terminal_write(session->term, "\r\n", 2);

    guac_mem_free(message);

}

void guac_dbshell_print_summary(guac_dbshell_session* session,
        unsigned long rows, int affected, long millis) {

    guac_dbshell_println(session, "%lu %s %s (%li.%02li sec)",
            rows,
            rows == 1 ? "row" : "rows",
            affected ? "affected" : "in set",
            millis / 1000, (millis % 1000) / 10);

}

/**
 * Returns whether the given input line consists solely of the given word,
 * compared case-insensitively, optionally followed by a single semicolon.
 * Leading and trailing whitespace is ignored.
 *
 * @param line
 *     The null-terminated input line to test.
 *
 * @param word
 *     The null-terminated word to compare the line against.
 *
 * @return
 *     Non-zero if the line consists solely of the given word, zero
 *     otherwise.
 */
static int guac_dbshell_line_is_command(const char* line,
        const char* word) {

    /* Ignore leading whitespace */
    while (isspace((unsigned char) *line))
        line++;

    /* Compare against the word itself */
    int word_length = strlen(word);
    for (int i = 0; i < word_length; i++) {
        if (tolower((unsigned char) line[i])
                != tolower((unsigned char) word[i]))
            return 0;
    }

    line += word_length;

    /* Allow a single trailing semicolon */
    if (*line == ';')
        line++;

    /* Ignore trailing whitespace */
    while (isspace((unsigned char) *line))
        line++;

    return *line == '\0';

}

/**
 * Writes the built-in help text to the terminal of the given session,
 * followed by any driver-specific help lines.
 *
 * @param session
 *     The session requesting help.
 */
static void guac_dbshell_print_help(guac_dbshell_session* session) {

    guac_dbshell_println(session, "Shell commands:");
    guac_dbshell_println(session, "  \\h        Display this help text.");
    guac_dbshell_println(session, "  \\q        Disconnect and end the "
            "session (also: quit, exit, Ctrl+D).");
    guac_dbshell_println(session, "  Ctrl+C    Cancel the current input "
            "line or running statement.");

    if (session->driver->help_handler != NULL)
        session->driver->help_handler(session);

}

/**
 * Handles the given complete input line as a meta-command if it is one,
 * dispatching driver-specific commands to the driver's meta handler.
 *
 * @param session
 *     The session which received the line.
 *
 * @param line
 *     The null-terminated input line.
 *
 * @param quit
 *     Set to true if the meta-command requests that the session end, and
 *     left unmodified otherwise.
 *
 * @return
 *     Non-zero if the line was handled as a meta-command and must not be
 *     forwarded to the statement splitter, zero otherwise.
 */
static int guac_dbshell_handle_meta(guac_dbshell_session* session,
        const char* line, bool* quit) {

    /* Bare "quit" / "exit" end the session */
    if (guac_dbshell_line_is_command(line, "quit")
            || guac_dbshell_line_is_command(line, "exit")) {
        *quit = true;
        return 1;
    }

    /* All other meta-commands begin with a backslash */
    const char* command = line;
    while (isspace((unsigned char) *command))
        command++;

    if (*command != '\\')
        return 0;

    command++;

    if (guac_dbshell_line_is_command(command, "q")) {
        *quit = true;
        return 1;
    }

    if (guac_dbshell_line_is_command(command, "h")
            || guac_dbshell_line_is_command(command, "help")) {
        guac_dbshell_print_help(session);
        return 1;
    }

    /* Offer the command to the driver */
    if (session->driver->meta_handler != NULL
            && session->driver->meta_handler(session, command))
        return 1;

    guac_dbshell_println(session, "Unknown command \"\\%s\". Type \\h for "
            "help.", command);
    return 1;

}

int guac_dbshell_repl_run(guac_dbshell_session* session) {

    const guac_dbshell_driver* driver = session->driver;

    /* Primary prompt: "name> "; continuation prompt of equal width
     * ending in "-> " */
    char prompt[GUAC_DBSHELL_MAX_PROMPT_LENGTH];
    char continuation[GUAC_DBSHELL_MAX_PROMPT_LENGTH];

    int prompt_length = snprintf(prompt, sizeof(prompt), "%s> ",
            driver->name);
    if (prompt_length > (int) sizeof(prompt) - 1)
        prompt_length = sizeof(prompt) - 1;
    if (prompt_length < 3)
        prompt_length = 3;

    memset(continuation, ' ', prompt_length);
    memcpy(continuation + prompt_length - 3, "-> ", 4);

    guac_dbshell_splitter* splitter =
        guac_dbshell_splitter_alloc(driver->dialect);

    /* The input parser persists across reads so that multi-byte sequences
     * spanning reads are handled correctly */
    guac_dbshell_parser parser;
    guac_dbshell_parser_init(&parser);

    int lost = 0;
    bool quit = false;

    while (!quit && session->client->state == GUAC_CLIENT_RUNNING) {

        /* Read the next input line */
        guac_dbshell_read_status status;
        char* line = guac_dbshell_line_editor_read(session->term, &parser,
                session->history,
                guac_dbshell_splitter_pending(splitter)
                    ? continuation : prompt,
                &status);

        /* End of input */
        if (status == GUAC_DBSHELL_READ_CLOSED)
            break;

        /* Ctrl+C discards the statement being accumulated */
        if (status == GUAC_DBSHELL_READ_CANCELLED) {
            guac_dbshell_splitter_reset(splitter);
            continue;
        }

        guac_dbshell_history_add(session->history, line);

        /* Handle meta-commands only outside multi-line statements */
        if (!guac_dbshell_splitter_pending(splitter)
                && guac_dbshell_handle_meta(session, line, &quit)) {
            guac_mem_free(line);
            continue;
        }

        guac_dbshell_splitter_feed(splitter, line);
        guac_mem_free(line);

        if (guac_dbshell_splitter_overflowed(splitter)) {
            guac_dbshell_println(session, "ERROR: Statement too long; "
                    "input discarded.");
            continue;
        }

        /* Execute each completed statement */
        char* statement;
        while (!lost && (statement =
                    guac_dbshell_splitter_next_statement(splitter))
                != NULL) {

            pthread_mutex_lock(&session->execute_lock);
            session->executing = true;
            pthread_mutex_unlock(&session->execute_lock);

            int result = driver->execute_handler(session, statement);

            pthread_mutex_lock(&session->execute_lock);
            session->executing = false;
            pthread_mutex_unlock(&session->execute_lock);

            guac_mem_free(statement);

            if (result == GUAC_DBSHELL_EXECUTE_LOST) {
                guac_dbshell_println(session, "The connection to the "
                        "database server has been lost.");
                lost = 1;
            }

        }

        if (lost)
            break;

    }

    guac_dbshell_splitter_free(splitter);
    return lost;

}
