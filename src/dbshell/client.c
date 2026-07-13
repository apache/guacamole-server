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

#include "dbshell/client.h"
#include "dbshell/dbshell.h"
#include "dbshell/settings.h"
#include "terminal/terminal.h"

#include <guacamole/argv.h>
#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <guacamole/proctitle.h>
#include <guacamole/protocol.h>
#include <guacamole/recording.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>
#include <guacamole/string.h>
#include <guacamole/user.h>
#include <guacamole/wol.h>

#include <langinfo.h>
#include <locale.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Internal extension of guac_dbshell_client tracking whether the client
 * thread was successfully started. This is required so that the free
 * handler does not attempt to join a thread which was never created.
 */
typedef struct guac_dbshell_client_internal {

    /**
     * The public portion of the client data.
     */
    guac_dbshell_client client;

    /**
     * Whether the client thread has been successfully started.
     */
    bool thread_started;

} guac_dbshell_client_internal;

/**
 * Sends the current values of all arguments which may be changed
 * mid-session (color scheme, font name, and font size) over the given
 * socket.
 *
 * @param client
 *     The guac_client associated with the database session.
 *
 * @param socket
 *     The socket to send the argument values over.
 *
 * @return
 *     Always NULL.
 */
static void* guac_dbshell_send_current_argv_batch(guac_client* client,
        guac_socket* socket) {

    guac_dbshell_client* dbshell_client = (guac_dbshell_client*) client->data;
    guac_terminal* terminal = dbshell_client->term;

    if (terminal == NULL)
        return NULL;

    /* Send current color scheme */
    guac_client_stream_argv(client, socket, "text/plain",
            GUAC_DBSHELL_ARGV_COLOR_SCHEME,
            guac_terminal_get_color_scheme(terminal));

    /* Send current font name */
    guac_client_stream_argv(client, socket, "text/plain",
            GUAC_DBSHELL_ARGV_FONT_NAME,
            guac_terminal_get_font_name(terminal));

    /* Send current font size */
    char font_size[64];
    snprintf(font_size, sizeof(font_size), "%i",
            guac_terminal_get_font_size(terminal));
    guac_client_stream_argv(client, socket, "text/plain",
            GUAC_DBSHELL_ARGV_FONT_SIZE, font_size);

    return NULL;

}

/**
 * Sends the current values of all mid-session arguments to the given user.
 *
 * @param user
 *     The user to send the argument values to.
 *
 * @param data
 *     Unused.
 *
 * @return
 *     Always NULL.
 */
static void* guac_dbshell_send_current_argv(guac_user* user, void* data) {
    return guac_dbshell_send_current_argv_batch(user->client, user->socket);
}

/**
 * Handles mid-session updates of the color scheme, font name, and font
 * size arguments, applying the new values to the terminal.
 *
 * @param user
 *     The user who changed the argument.
 *
 * @param mimetype
 *     The mimetype of the data within the argument value stream (ignored).
 *
 * @param name
 *     The name of the argument being changed.
 *
 * @param value
 *     The new value of the argument.
 *
 * @param data
 *     Unused.
 *
 * @return
 *     Always zero.
 */
static int guac_dbshell_argv_callback(guac_user* user, const char* mimetype,
        const char* name, const char* value, void* data) {

    guac_client* client = user->client;
    guac_dbshell_client* dbshell_client = (guac_dbshell_client*) client->data;
    guac_terminal* terminal = dbshell_client->term;

    if (terminal == NULL)
        return 0;

    /* Update color scheme */
    if (strcmp(name, GUAC_DBSHELL_ARGV_COLOR_SCHEME) == 0)
        guac_terminal_apply_color_scheme(terminal, value);

    /* Update font name */
    else if (strcmp(name, GUAC_DBSHELL_ARGV_FONT_NAME) == 0)
        guac_terminal_apply_font(terminal, value, -1, 0);

    /* Update only if font size is sane */
    else if (strcmp(name, GUAC_DBSHELL_ARGV_FONT_SIZE) == 0) {
        int size = atoi(value);
        if (size > 0)
            guac_terminal_apply_font(terminal, NULL, size,
                    dbshell_client->settings->resolution);
    }

    return 0;

}

/**
 * Handles mouse events, forwarding them to the terminal.
 *
 * @param user
 *     The user that originated the mouse event.
 *
 * @param x
 *     The X coordinate of the mouse within the display, in pixels.
 *
 * @param y
 *     The Y coordinate of the mouse within the display, in pixels.
 *
 * @param mask
 *     An integer value representing the current state of each button.
 *
 * @return
 *     Always zero.
 */
static int guac_dbshell_user_mouse_handler(guac_user* user,
        int x, int y, int mask) {

    guac_client* client = user->client;
    guac_dbshell_client* dbshell_client = (guac_dbshell_client*) client->data;

    /* Skip if terminal not yet ready */
    guac_terminal* term = dbshell_client->term;
    if (term == NULL)
        return 0;

    /* Report mouse position within recording */
    if (dbshell_client->recording != NULL)
        guac_recording_report_mouse(dbshell_client->recording, x, y, mask);

    guac_terminal_send_mouse(term, user, x, y, mask);
    return 0;

}

/**
 * Handles key events, forwarding them to the terminal. Additionally, if
 * Ctrl+C is pressed while a statement is executing, cancellation of that
 * statement is requested.
 *
 * @param user
 *     The user that originated the key event.
 *
 * @param keysym
 *     The X11 keysym of the key that was pressed or released.
 *
 * @param pressed
 *     Non-zero if the key was pressed, zero if it was released.
 *
 * @return
 *     Always zero.
 */
static int guac_dbshell_user_key_handler(guac_user* user, int keysym,
        int pressed) {

    guac_client* client = user->client;
    guac_dbshell_client* dbshell_client = (guac_dbshell_client*) client->data;

    /* Report key state within recording */
    if (dbshell_client->recording != NULL)
        guac_recording_report_key(dbshell_client->recording, keysym,
                pressed);

    /* Skip if terminal not yet ready */
    guac_terminal* term = dbshell_client->term;
    if (term == NULL)
        return 0;

    /* Request cancellation of the running statement on Ctrl+C. The 0x03
     * byte resulting from the key event additionally clears the input
     * line once the REPL resumes reading. */
    if (pressed && (keysym == 'c' || keysym == 'C')
            && guac_terminal_get_mod_ctrl(term)
            && dbshell_client->session != NULL)
        guac_dbshell_session_cancel(dbshell_client->session);

    guac_terminal_send_key(term, keysym, pressed);
    return 0;

}

/**
 * Handles display size change events, resizing the terminal accordingly.
 *
 * @param user
 *     The user that originated the resize request.
 *
 * @param width
 *     The new display width, in pixels.
 *
 * @param height
 *     The new display height, in pixels.
 *
 * @return
 *     Always zero.
 */
static int guac_dbshell_user_size_handler(guac_user* user, int width,
        int height) {

    guac_client* client = user->client;
    guac_dbshell_client* dbshell_client = (guac_dbshell_client*) client->data;

    /* Skip if terminal not yet ready */
    guac_terminal* terminal = dbshell_client->term;
    if (terminal == NULL)
        return 0;

    /* Resize terminal (there is no remote terminal to notify) */
    guac_terminal_resize(terminal, width, height);
    return 0;

}

/**
 * Handles inbound clipboard streams, resetting the terminal clipboard to
 * receive the new data.
 *
 * @param user
 *     The user that opened the clipboard stream.
 *
 * @param stream
 *     The stream through which clipboard data will be received.
 *
 * @param mimetype
 *     The mimetype of the clipboard data.
 *
 * @return
 *     Always zero.
 */
static int guac_dbshell_clipboard_handler(guac_user* user,
        guac_stream* stream, char* mimetype);

/**
 * Handles blobs of data received along an inbound clipboard stream,
 * appending them to the terminal clipboard.
 *
 * @param user
 *     The user that sent the blob.
 *
 * @param stream
 *     The clipboard stream.
 *
 * @param data
 *     The blob data received.
 *
 * @param length
 *     The number of bytes received.
 *
 * @return
 *     Always zero.
 */
static int guac_dbshell_clipboard_blob_handler(guac_user* user,
        guac_stream* stream, void* data, int length);

/**
 * Handles the end of an inbound clipboard stream.
 *
 * @param user
 *     The user that closed the stream.
 *
 * @param stream
 *     The clipboard stream.
 *
 * @return
 *     Always zero.
 */
static int guac_dbshell_clipboard_end_handler(guac_user* user,
        guac_stream* stream);

static int guac_dbshell_clipboard_handler(guac_user* user,
        guac_stream* stream, char* mimetype) {

    guac_client* client = user->client;
    guac_dbshell_client* dbshell_client = (guac_dbshell_client*) client->data;

    /* Skip if terminal not yet ready */
    if (dbshell_client->term == NULL)
        return 0;

    /* Clear clipboard and prepare for new data */
    guac_terminal_clipboard_reset(dbshell_client->term, mimetype);

    /* Set handlers for clipboard stream */
    stream->blob_handler = guac_dbshell_clipboard_blob_handler;
    stream->end_handler = guac_dbshell_clipboard_end_handler;

    /* Report clipboard within recording */
    if (dbshell_client->recording != NULL)
        guac_recording_report_clipboard_begin(dbshell_client->recording,
                stream, mimetype);

    return 0;

}

static int guac_dbshell_clipboard_blob_handler(guac_user* user,
        guac_stream* stream, void* data, int length) {

    guac_client* client = user->client;
    guac_dbshell_client* dbshell_client = (guac_dbshell_client*) client->data;

    /* Report clipboard blob within recording */
    if (dbshell_client->recording != NULL)
        guac_recording_report_clipboard_blob(dbshell_client->recording,
                stream, data, length);

    /* Append new data */
    guac_terminal_clipboard_append(dbshell_client->term, data, length);

    return 0;

}

static int guac_dbshell_clipboard_end_handler(guac_user* user,
        guac_stream* stream) {

    guac_client* client = user->client;
    guac_dbshell_client* dbshell_client = (guac_dbshell_client*) client->data;

    /* Report end of clipboard within recording */
    if (dbshell_client->recording != NULL)
        guac_recording_report_clipboard_end(dbshell_client->recording,
                stream);

    /* Nothing further to do - the clipboard is implemented within the
     * terminal */

    return 0;

}

/**
 * Handles inbound pipe streams, redirecting the terminal's STDIN to the
 * stream if the pipe bears the expected name.
 *
 * @param user
 *     The user that opened the pipe stream.
 *
 * @param stream
 *     The pipe stream.
 *
 * @param mimetype
 *     The mimetype of the data within the stream.
 *
 * @param name
 *     The name of the pipe stream.
 *
 * @return
 *     Always zero.
 */
static int guac_dbshell_pipe_handler(guac_user* user, guac_stream* stream,
        char* mimetype, char* name) {

    guac_client* client = user->client;
    guac_dbshell_client* dbshell_client = (guac_dbshell_client*) client->data;

    /* Redirect STDIN if pipe has required name */
    if (dbshell_client->term != NULL
            && strcmp(name, GUAC_DBSHELL_STDIN_PIPE_NAME) == 0) {
        guac_terminal_send_stream(dbshell_client->term, user, stream);
        return 0;
    }

    /* No other inbound pipe streams are supported */
    guac_protocol_send_ack(user->socket, stream, "No such input stream.",
            GUAC_PROTOCOL_STATUS_RESOURCE_NOT_FOUND);
    guac_socket_flush(user->socket);
    return 0;

}

/**
 * The client thread of a database session: establishes the connection to
 * the database server, runs the interactive REPL until the session ends,
 * and disconnects.
 *
 * @param data
 *     The guac_client associated with the session.
 *
 * @return
 *     Always NULL.
 */
static void* guac_dbshell_client_thread(void* data) {

    guac_client* client = (guac_client*) data;
    guac_dbshell_client* dbshell_client = (guac_dbshell_client*) client->data;

    const guac_dbshell_plugin* plugin = dbshell_client->plugin;
    guac_dbshell_settings* settings = dbshell_client->settings;

    /* The port of the database server, as a string */
    char port_str[GUAC_USHORT_STRING_BUFSIZE];
    guac_itoa_safe(port_str, sizeof(port_str), settings->port);

    /* Identify the session within the process title, never including the
     * password */
    guac_process_title_set_endpoint(plugin->driver->name,
            settings->username, settings->hostname, port_str);

    /* Send Wake-on-LAN packet if requested */
    if (settings->wol_send_packet) {

        /* Send the packet and wait for the server to become reachable if
         * a wait time was given */
        if (settings->wol_wait_time > 0) {

            guac_client_log(client, GUAC_LOG_DEBUG, "Sending Wake-on-LAN "
                    "packet, and pausing for %i seconds.",
                    settings->wol_wait_time);

            if (guac_wol_wake_and_wait(settings->wol_mac_addr,
                        settings->wol_broadcast_addr,
                        settings->wol_udp_port,
                        settings->wol_wait_time,
                        GUAC_WOL_DEFAULT_CONNECT_RETRIES,
                        settings->hostname, port_str,
                        settings->timeout)) {
                guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                        "Failed to send WoL packet, or the database server "
                        "did not become reachable.");
                return NULL;
            }

        }

        else if (guac_wol_wake(settings->wol_mac_addr,
                    settings->wol_broadcast_addr,
                    settings->wol_udp_port)) {
            guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                    "Failed to send WoL packet.");
            return NULL;
        }

    }

    /* Set up screen recording, if requested */
    if (settings->recording_path != NULL) {
        dbshell_client->recording = guac_recording_create(client,
                settings->recording_path,
                settings->recording_name,
                settings->create_recording_path,
                !settings->recording_exclude_output,
                !settings->recording_exclude_mouse,
                0, /* Touch events not supported */
                settings->recording_include_keys,
                settings->recording_write_existing,
                settings->recording_include_clipboard);
    }

    /* Create terminal options with required parameters */
    guac_terminal_options* options = guac_terminal_options_create(
            settings->width, settings->height, settings->resolution);

    /* Set optional parameters */
    options->clipboard_buffer_size = settings->clipboard_buffer_size;
    options->disable_copy = settings->disable_copy;
    options->max_scrollback = settings->max_scrollback;
    options->font_name = settings->font_name;
    options->font_size = settings->font_size;
    options->color_scheme = settings->color_scheme;
    options->backspace = settings->backspace;
    options->func_keys_and_keypad = settings->func_keys_and_keypad;
    options->linux_console_keys =
        strcmp(settings->terminal_type, "linux") == 0;

    /* Create terminal */
    dbshell_client->term = guac_terminal_create(client, options);

    /* Free options struct now that it's been used */
    guac_mem_free(options);

    /* Fail if terminal init failed */
    if (dbshell_client->term == NULL) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "Terminal initialization failed");
        return NULL;
    }

    /* Send current values of exposed arguments to owner only */
    guac_client_for_owner(client, guac_dbshell_send_current_argv, NULL);

    /* Set up typescript, if requested */
    if (settings->typescript_path != NULL) {
        guac_terminal_create_typescript(dbshell_client->term,
                settings->typescript_path,
                settings->typescript_name,
                settings->create_typescript_path,
                settings->typescript_write_existing);
    }

    /* Allow terminal to render, and accept input for credential
     * prompting */
    guac_terminal_start(dbshell_client->term);

    /* Create the database session */
    guac_dbshell_session* session = guac_dbshell_session_alloc(client,
            dbshell_client->term, plugin->driver, settings);
    dbshell_client->session = session;

    /* Establish the connection to the database server */
    if (plugin->driver->connect_handler(session)) {
        guac_client_log(client, GUAC_LOG_INFO, "Database connection "
                "failed.");
        guac_client_stop(client);
        return NULL;
    }

    guac_client_log(client, GUAC_LOG_INFO, "Database connection "
            "successful.");

    /* Run the interactive shell until the session ends */
    guac_dbshell_repl_run(session);

    /* Close the connection to the database server */
    plugin->driver->disconnect_handler(session);

    guac_client_log(client, GUAC_LOG_INFO, "Database session ended.");
    guac_client_stop(client);

    return NULL;

}

/**
 * Handles users joining the connection, parsing their arguments and, for
 * the connection owner, starting the client thread.
 *
 * @param user
 *     The user joining the connection.
 *
 * @param argc
 *     The number of arguments provided by the user.
 *
 * @param argv
 *     The values of all arguments provided by the user.
 *
 * @return
 *     Zero if the user was successfully joined, non-zero otherwise.
 */
static int guac_dbshell_user_join_handler(guac_user* user, int argc,
        char** argv) {

    guac_client* client = user->client;
    guac_dbshell_client_internal* internal =
        (guac_dbshell_client_internal*) client->data;
    guac_dbshell_client* dbshell_client = &internal->client;
    const guac_dbshell_plugin* plugin = dbshell_client->plugin;

    /* Parse the common arguments */
    guac_dbshell_settings* settings = guac_dbshell_settings_parse(user,
            plugin->args, argc, (const char**) argv, plugin->argc,
            plugin->default_port);

    /* Fail if settings cannot be parsed */
    if (settings == NULL) {
        guac_user_log(user, GUAC_LOG_INFO,
                "Badly formatted client arguments.");
        return 1;
    }

    /* Parse any database-specific arguments */
    void* extra_settings = NULL;
    if (plugin->parse_extra_args != NULL)
        extra_settings = plugin->parse_extra_args(user, argc,
                (const char**) argv);

    /* Store settings at user level */
    guac_dbshell_user* user_data = guac_mem_zalloc(sizeof(guac_dbshell_user));
    user_data->settings = settings;
    user_data->extra_settings = extra_settings;
    user->data = user_data;

    /* Connect to the database if this user is the connection owner */
    if (user->owner) {

        /* Store owner's settings at client level */
        dbshell_client->settings = settings;
        dbshell_client->extra_settings = extra_settings;

        /* Start client thread */
        if (pthread_create(&dbshell_client->client_thread, NULL,
                    guac_dbshell_client_thread, (void*) client)) {
            guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                    "Unable to start database client thread");
            return 1;
        }

        internal->thread_started = true;

    }

    /* Only handle events if not read-only */
    if (!settings->read_only) {

        /* General mouse/keyboard events */
        user->key_handler = guac_dbshell_user_key_handler;
        user->mouse_handler = guac_dbshell_user_mouse_handler;

        /* Inbound (client to server) clipboard transfer */
        if (!settings->disable_paste)
            user->clipboard_handler = guac_dbshell_clipboard_handler;

        /* STDIN redirection */
        user->pipe_handler = guac_dbshell_pipe_handler;

        /* Updates to connection parameters */
        user->argv_handler = guac_argv_handler;

        /* Display size change events */
        user->size_handler = guac_dbshell_user_size_handler;

    }

    return 0;

}

/**
 * Handles users leaving the connection, removing them from the terminal
 * and freeing their per-user data.
 *
 * @param user
 *     The user leaving the connection.
 *
 * @return
 *     Always zero.
 */
static int guac_dbshell_user_leave_handler(guac_user* user) {

    guac_dbshell_client* dbshell_client =
        (guac_dbshell_client*) user->client->data;

    /* Remove the user from the terminal */
    if (dbshell_client->term != NULL)
        guac_terminal_remove_user(dbshell_client->term, user);

    /* Free settings if not owner (owner settings are freed with the
     * client) */
    guac_dbshell_user* user_data = (guac_dbshell_user*) user->data;
    if (user_data != NULL && !user->owner) {

        guac_dbshell_settings_free(user_data->settings);

        if (dbshell_client->plugin->free_extra_args != NULL)
            dbshell_client->plugin->free_extra_args(
                    user_data->extra_settings);

    }

    guac_mem_free(user_data);
    return 0;

}

/**
 * Synchronizes the terminal state and mid-session argument values to all
 * pending users prior to their promotion to full users.
 *
 * @param client
 *     The client whose pending users are about to be promoted.
 *
 * @return
 *     Always zero.
 */
static int guac_dbshell_join_pending_handler(guac_client* client) {

    guac_dbshell_client* dbshell_client = (guac_dbshell_client*) client->data;

    /* Synchronize the terminal state to all pending users */
    if (dbshell_client->term != NULL) {
        guac_socket* broadcast_socket = client->pending_socket;
        guac_terminal_sync_users(dbshell_client->term, client,
                broadcast_socket);
        guac_dbshell_send_current_argv_batch(client, broadcast_socket);
        guac_socket_flush(broadcast_socket);
    }

    return 0;

}

/**
 * Handles the disconnection and cleanup of the database client, stopping
 * the client thread and freeing all associated resources.
 *
 * @param client
 *     The client being freed.
 *
 * @return
 *     Always zero.
 */
static int guac_dbshell_client_free_handler(guac_client* client) {

    guac_dbshell_client_internal* internal =
        (guac_dbshell_client_internal*) client->data;
    guac_dbshell_client* dbshell_client = &internal->client;

    /* Interrupt any statement currently blocking the client thread */
    if (dbshell_client->session != NULL)
        guac_dbshell_session_cancel(dbshell_client->session);

    /* Stop the terminal to unblock any pending reads/writes */
    if (dbshell_client->term != NULL)
        guac_terminal_stop(dbshell_client->term);

    /* Wait for the client thread to terminate */
    if (internal->thread_started)
        pthread_join(dbshell_client->client_thread, NULL);

    /* Free the session, terminal, and recording now that the thread has
     * finished */
    guac_dbshell_session_free(dbshell_client->session);

    if (dbshell_client->term != NULL)
        guac_terminal_free(dbshell_client->term);

    if (dbshell_client->recording != NULL)
        guac_recording_free(dbshell_client->recording);

    /* Free the owner's settings */
    guac_dbshell_settings_free(dbshell_client->settings);

    if (dbshell_client->plugin->free_extra_args != NULL)
        dbshell_client->plugin->free_extra_args(
                dbshell_client->extra_settings);

    guac_mem_free(internal);
    return 0;

}

int guac_dbshell_client_init(guac_client* client,
        const guac_dbshell_plugin* plugin) {

    /* Set client args */
    client->args = plugin->args;

    /* Allocate client instance data */
    guac_dbshell_client_internal* internal =
        guac_mem_zalloc(sizeof(guac_dbshell_client_internal));
    internal->client.plugin = plugin;
    client->data = internal;

    /* Set handlers */
    client->join_handler = guac_dbshell_user_join_handler;
    client->join_pending_handler = guac_dbshell_join_pending_handler;
    client->free_handler = guac_dbshell_client_free_handler;
    client->leave_handler = guac_dbshell_user_leave_handler;

    /* Register handlers for argument values that may be sent after the
     * handshake */
    guac_argv_register(GUAC_DBSHELL_ARGV_COLOR_SCHEME,
            guac_dbshell_argv_callback, NULL, GUAC_ARGV_OPTION_ECHO);
    guac_argv_register(GUAC_DBSHELL_ARGV_FONT_NAME,
            guac_dbshell_argv_callback, NULL, GUAC_ARGV_OPTION_ECHO);
    guac_argv_register(GUAC_DBSHELL_ARGV_FONT_SIZE,
            guac_dbshell_argv_callback, NULL, GUAC_ARGV_OPTION_ECHO);

    /* Set locale and warn if not UTF-8 */
    setlocale(LC_CTYPE, "");
    if (strcmp(nl_langinfo(CODESET), "UTF-8") != 0) {
        guac_client_log(client, GUAC_LOG_INFO,
                "Current locale does not use UTF-8. Some characters may "
                "not render correctly.");
    }

    /* Success */
    return 0;

}

void guac_dbshell_prompt_credentials(guac_dbshell_session* session,
        bool need_username, bool need_password) {

    guac_dbshell_settings* settings =
        (guac_dbshell_settings*) session->settings;

    /* Prompt for username if required but not provided */
    if (need_username && settings->username == NULL)
        settings->username = guac_terminal_prompt(session->term,
                "Username: ", true);

    /* Prompt for password if required but not provided, without echo */
    if (need_password && settings->password == NULL)
        settings->password = guac_terminal_prompt(session->term,
                "Password: ", false);

}
