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

#include "argv.h"
#include "auth.h"
#include "channels.h"
#include "client.h"
#include "log.h"
#include "settings.h"
#include "spice.h"

#ifdef ENABLE_COMMON_SSH
#include "common-ssh/sftp.h"
#include "common-ssh/ssh.h"
#include "common-ssh/user.h"
#endif

#include <guacamole/argv.h>
#include <guacamole/client.h>
#include <guacamole/display.h>
#include <guacamole/proctitle.h>
#include <guacamole/recording.h>
#include <guacamole/string.h>
#include <guacamole/timestamp.h>
#include <guacamole/wol-constants.h>
#include <guacamole/wol.h>
#include <spice-client.h>

#include <stdlib.h>
#include <string.h>

/**
 * GSource callback which periodically checks whether the Guacamole connection
 * is still running, terminating the SPICE event loop if it is not.
 *
 * @param data
 *     The guac_client associated with the SPICE connection.
 *
 * @return
 *     G_SOURCE_CONTINUE if the connection is still running and the loop should
 *     continue, or G_SOURCE_REMOVE if the loop should terminate.
 */
static gboolean guac_spice_state_check(gpointer data) {

    guac_client* client = (guac_client*) data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /* Continue running while the client is running */
    if (client->state == GUAC_CLIENT_RUNNING)
        return G_SOURCE_CONTINUE;

    /* Otherwise, stop the SPICE event loop */
    g_main_loop_quit(spice_client->main_loop);
    return G_SOURCE_REMOVE;

}

#ifdef ENABLE_COMMON_SSH
/**
 * Establishes the SSH/SFTP connection requested by the connection settings,
 * exposing the resulting filesystem to the connection owner. Any failure
 * aborts the Guacamole connection.
 *
 * @param client
 *     The guac_client associated with the SPICE connection.
 *
 * @return
 *     Zero on success, non-zero if the SFTP connection could not be
 *     established.
 */
static int guac_spice_start_sftp(guac_client* client) {

    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    guac_spice_settings* settings = spice_client->settings;

    guac_common_ssh_init(client);

    /* Abort if username is missing */
    if (settings->sftp_username == NULL) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "SFTP username is required if SFTP is enabled.");
        return 1;
    }

    guac_client_log(client, GUAC_LOG_DEBUG,
            "Connecting via SSH for SFTP filesystem access.");

    spice_client->sftp_user =
        guac_common_ssh_create_user(settings->sftp_username);

    /* Authenticate with private key, if given */
    if (settings->sftp_private_key != NULL) {

        if (guac_common_ssh_user_import_key(spice_client->sftp_user,
                    settings->sftp_private_key, settings->sftp_passphrase)) {
            guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                    "Private key unreadable.");
            return 1;
        }

        /* Import the public key, if specified */
        if (settings->sftp_public_key != NULL
                && guac_common_ssh_user_import_public_key(
                    spice_client->sftp_user, settings->sftp_public_key)) {
            guac_client_abort(client, GUAC_PROTOCOL_STATUS_CLIENT_UNAUTHORIZED,
                    "Failed to import public key: %s",
                    guac_common_ssh_key_error());
            return 1;
        }

    }

    /* Otherwise, authenticate with password */
    else
        guac_common_ssh_user_set_password(spice_client->sftp_user,
                settings->sftp_password);

    /* Attempt SSH connection */
    spice_client->sftp_session =
        guac_common_ssh_create_session(client, settings->sftp_hostname,
                settings->sftp_port, spice_client->sftp_user,
                settings->sftp_timeout, settings->sftp_server_alive_interval,
                settings->sftp_host_key, NULL);

    if (spice_client->sftp_session == NULL)
        /* Already aborted within guac_common_ssh_create_session() */
        return 1;

    /* Load filesystem */
    spice_client->sftp_filesystem =
        guac_common_ssh_create_sftp_filesystem(spice_client->sftp_session,
                settings->sftp_root_directory, NULL,
                settings->sftp_disable_download,
                settings->sftp_disable_upload);

    /* Expose filesystem to connection owner */
    guac_client_for_owner(client,
            guac_common_ssh_expose_sftp_filesystem,
            spice_client->sftp_filesystem);

    /* Configure destination for basic uploads, if specified */
    if (settings->sftp_directory != NULL)
        guac_common_ssh_sftp_set_upload_path(spice_client->sftp_filesystem,
                settings->sftp_directory);

    guac_client_log(client, GUAC_LOG_DEBUG, "SFTP connection succeeded.");
    return 0;

}
#endif

/**
 * GSourceFunc which runs a deferred spice-gtk call on the SPICE event-loop
 * thread. Always returns G_SOURCE_REMOVE so the call is dispatched exactly once.
 *
 * @param user_data
 *     The guac_spice_deferred_call to dispatch.
 *
 * @return
 *     G_SOURCE_REMOVE, always.
 */
static gboolean guac_spice_deferred_run(gpointer user_data) {

    guac_spice_deferred_call* call = (guac_spice_deferred_call*) user_data;

    if (call->handler != NULL)
        call->handler(call);

    return G_SOURCE_REMOVE;

}

/**
 * GDestroyNotify which frees a deferred spice-gtk call and any payload it holds.
 * Invoked after the call has been dispatched, or when the loop terminates before
 * dispatch, ensuring no leak in either case.
 *
 * @param user_data
 *     The guac_spice_deferred_call to free.
 */
static void guac_spice_deferred_free(gpointer user_data) {

    guac_spice_deferred_call* call = (guac_spice_deferred_call*) user_data;

    g_free(call->data);
    g_free(call);

}

void guac_spice_defer_call(guac_spice_deferred_call* call) {

    /* Schedule the call on the default GMainContext, which is driven by the
     * SPICE client thread. If invoked from that thread (e.g. a spice-gtk signal
     * handler), it runs immediately; otherwise it is queued and executed by the
     * loop, guaranteeing all channel I/O occurs on a single thread. */
    g_main_context_invoke_full(NULL, G_PRIORITY_DEFAULT,
            guac_spice_deferred_run, call, guac_spice_deferred_free);

}

void* guac_spice_client_thread(void* data) {

    /* Thread name spice-worker: main SPICE client thread; runs the spice-gtk
     * session and GLib event loop. */
    guac_thread_name_set("spice-worker");

    guac_client* client = (guac_client*) data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    guac_spice_settings* settings = spice_client->settings;

    /* Set the process title to reflect the connection endpoint */
    const char* title_port = settings->port[0] != '\0'
            ? settings->port : settings->tls_port;
    guac_process_title_set_endpoint(GUAC_SPICE_PROCESS_TITLE_NAME,
            settings->username, settings->hostname, title_port);

    /* Route spice-gtk/GLib log output to the Guacamole client */
    guac_spice_client_log_init(client);

    /* Register password/username arguments for runtime updates */
    guac_argv_register(GUAC_SPICE_ARGV_PASSWORD,
            guac_spice_argv_callback, NULL, 0);
    guac_argv_register(GUAC_SPICE_ARGV_USERNAME,
            guac_spice_argv_callback, NULL, 0);

    /* If Wake-on-LAN is enabled, attempt to wake the remote host */
    if (settings->wol_send_packet) {
        guac_client_log(client, GUAC_LOG_DEBUG, "Sending Wake-on-LAN packet.");
        if (guac_wol_wake(settings->wol_mac_addr,
                    settings->wol_broadcast_addr, settings->wol_udp_port)) {
            guac_client_log(client, GUAC_LOG_ERROR,
                    "Failed to send Wake-on-LAN packet.");
        }
        else if (settings->wol_wait_time > 0) {
            guac_client_log(client, GUAC_LOG_DEBUG, "Waiting %d seconds for "
                    "remote host to wake.", settings->wol_wait_time);
            guac_timestamp_msleep(settings->wol_wait_time * 1000);
        }
    }

    /* Drive all SPICE channel I/O from this thread's default GLib main context.
     * spice-gtk schedules each channel's connection coroutine on the default
     * main context, so the event loop we run below must be that same context.
     * guacd forks a dedicated process per connection, so the default context is
     * private to this connection and safe to use. */
    spice_client->main_context = NULL;
    spice_client->main_loop = g_main_loop_new(NULL, FALSE);

    /* Disable the Opus audio codec for this connection if requested. spice-gtk
     * then stops advertising Opus and the SPICE server falls back to its next
     * available audio mode: raw/lossless on modern servers (which no longer
     * support the legacy CELT codec), or CELT on older servers. guacd runs a
     * dedicated process per connection, so this environment variable affects
     * only the current connection. */
    if (settings->disable_audio_opus)
        setenv("SPICE_DISABLE_OPUS", "1", 1);

    /* Allocate and configure the SPICE session */
    spice_client->spice_session = spice_session_new();
    guac_spice_session_configure(client, spice_client->spice_session);

    /* Set up the shared folder file browser, if file transfer is enabled. The
     * shared-dir property here takes precedence over the basic enable-drive
     * sharing configured within guac_spice_session_configure(). */
    if (settings->file_transfer && settings->file_directory != NULL) {

        guac_client_log(client, GUAC_LOG_INFO,
                "Enabling shared folder file transfer for \"%s\"%s.",
                settings->file_directory,
                settings->file_transfer_ro ? " (read-only)" : "");

        g_object_set(spice_client->spice_session,
                "shared-dir", settings->file_directory, NULL);
        g_object_set(spice_client->spice_session,
                "share-dir-ro", settings->file_transfer_ro, NULL);

        /* Allocate the shared folder, exposing it to the connection owner as a
         * Guacamole filesystem object */
        spice_client->shared_folder = guac_spice_folder_alloc(client,
                settings->file_directory,
                settings->file_transfer_create_folder,
                settings->disable_download,
                settings->disable_upload);

        guac_client_for_owner(client, guac_spice_folder_expose,
                spice_client->shared_folder);

    }

    /* Allocate keyboard, translating keysyms to scancodes according to the
     * keyboard layout requested by the connection (falling back to the default
     * layout if the requested layout is unknown) */
    const guac_spice_keymap* keymap =
            guac_spice_keymap_find(settings->server_layout);
    if (keymap == NULL) {
        guac_client_log(client, GUAC_LOG_WARNING, "Unknown keyboard layout "
                "\"%s\". Falling back to \"%s\".",
                settings->server_layout, GUAC_SPICE_DEFAULT_KEYMAP);
        keymap = guac_spice_keymap_find(GUAC_SPICE_DEFAULT_KEYMAP);
    }

    pthread_rwlock_wrlock(&spice_client->lock);
    spice_client->keyboard = guac_spice_keyboard_alloc(client, keymap);
    pthread_rwlock_unlock(&spice_client->lock);

    /* Dispatch newly-created and destroyed channels */
    g_signal_connect(spice_client->spice_session, "channel-new",
            G_CALLBACK(guac_spice_channel_new), client);
    g_signal_connect(spice_client->spice_session, "channel-destroy",
            G_CALLBACK(guac_spice_channel_destroy), client);

    /* Create the Guacamole display and render thread. The display is resized
     * once the SPICE display channel reports the dimensions of its primary
     * surface. */
    spice_client->display = guac_display_alloc(client);

    if (!settings->read_only)
        guac_display_set_cursor(spice_client->display,
                GUAC_DISPLAY_CURSOR_POINTER);

    guac_display_end_frame(spice_client->display);
    spice_client->render_thread =
            guac_display_render_thread_create(spice_client->display);

    /* Set up screen recording, if requested */
    if (settings->recording_path != NULL) {
        spice_client->recording = guac_recording_create(client,
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

#ifdef ENABLE_COMMON_SSH
    /* Connect via SSH for SFTP, if enabled */
    if (settings->enable_sftp && guac_spice_start_sftp(client))
        return NULL;
#endif

    /* Begin connecting to the SPICE server */
    guac_client_log(client, GUAC_LOG_INFO, "Connecting to SPICE server %s.",
            settings->hostname);

    if (!spice_session_connect(spice_client->spice_session)) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_NOT_FOUND,
                "Unable to initiate connection to SPICE server.");
    }

    else {

        /* Periodically check whether the connection should continue running */
        GSource* state_source =
                g_timeout_source_new(GUAC_SPICE_STATE_CHECK_INTERVAL);
        g_source_set_callback(state_source, guac_spice_state_check, client, NULL);
        g_source_attach(state_source, NULL);

        /* Run the SPICE event loop until the connection ends */
        g_main_loop_run(spice_client->main_loop);

        g_source_destroy(state_source);
        g_source_unref(state_source);

    }

    /* Cancel any pending debounced monitors-config flush so its timer cannot
     * fire after the client state is torn down */
    if (spice_client->resize_timer_source != 0) {
        g_source_remove(spice_client->resize_timer_source);
        spice_client->resize_timer_source = 0;
    }

    /* Disconnect the SPICE session */
    spice_session_disconnect(spice_client->spice_session);

    /* Stop the render loop */
    if (spice_client->render_thread != NULL) {
        guac_display_render_thread_destroy(spice_client->render_thread);
        spice_client->render_thread = NULL;
    }

    /* Kill client and finish connection */
    guac_client_stop(client);
    guac_client_log(client, GUAC_LOG_INFO, "Internal SPICE client disconnected");
    return NULL;

}
