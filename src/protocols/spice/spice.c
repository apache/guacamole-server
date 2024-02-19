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

#include "auth.h"
#include "client.h"
#include "common/cursor.h"
#include "common/display.h"
#include "channels/audio.h"
#include "channels/cursor.h"
#include "channels/display.h"
#include "channels/file.h"
#include "log.h"
#include "settings.h"
#include "spice.h"
#include "spice-constants.h"

#ifdef ENABLE_COMMON_SSH
#include "common-ssh/sftp.h"
#include "common-ssh/ssh.h"
#include "sftp.h"
#endif

#include <glib/gmain.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/timestamp.h>
#include <spice-client-glib-2.0/spice-client.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>

SpiceSession* guac_spice_get_session(guac_client* client) {

    guac_client_log(client, GUAC_LOG_DEBUG, "Initializing new SPICE session.");
    
    /* Set up the Spice session and Guacamole client. */
    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    guac_spice_settings* spice_settings = spice_client->settings;
    
    /* Create a new Spice client. */
    SpiceSession* spice_session = spice_session_new();
    
    /* Register a callback for handling new channel events. */
    g_signal_connect(spice_session, SPICE_SIGNAL_CHANNEL_NEW,
            G_CALLBACK(guac_spice_client_channel_handler), client);
    
    /* Set hostname and port */
    g_object_set(spice_session, SPICE_PROPERTY_HOST, spice_settings->hostname, NULL);
    guac_client_log(client, GUAC_LOG_DEBUG, "Connecting to host %s",
        spice_settings->hostname);
    if (spice_settings->tls) {
        guac_client_log(client, GUAC_LOG_DEBUG, "Using TLS mode on port %s",
            spice_settings->port);
        g_object_set(spice_session,
                SPICE_PROPERTY_TLS_PORT, spice_settings->port,
                SPICE_PROPERTY_VERIFY, spice_settings->tls_verify,
                NULL);
        if (spice_settings->ca != NULL)
            g_object_set(spice_session, SPICE_PROPERTY_CA, spice_settings->ca, NULL);
        if (spice_settings->ca_file != NULL)
            g_object_set(spice_session, SPICE_PROPERTY_CA_FILE, spice_settings->ca_file, NULL);
    }
    else {
        guac_client_log(client, GUAC_LOG_DEBUG, "Using plaintext mode on port %s",
            spice_settings->port);
        g_object_set(spice_session,
                SPICE_PROPERTY_PORT, spice_settings->port, NULL);
    }

    guac_client_log(client, GUAC_LOG_DEBUG, "Setting up keyboard layout: %s",
        spice_settings->server_layout);

    /* Load keymap into client */
    spice_client->keyboard = guac_spice_keyboard_alloc(client,
            spice_settings->server_layout);

    /* If file transfer is enabled, set up the required properties. */
    if (spice_settings->file_transfer) {
        guac_client_log(client, GUAC_LOG_DEBUG, "File transfer enabled, configuring Spice client.");
        g_object_set(spice_session, SPICE_PROPERTY_SHARED_DIR, spice_settings->file_directory, NULL);
        g_object_set(spice_session, SPICE_PROPERTY_SHARED_DIR_RO, spice_settings->file_transfer_ro, NULL);
        spice_client->shared_folder = guac_spice_folder_alloc(client,
                spice_settings->file_directory,
                spice_settings->file_transfer_create_folder,
                spice_settings->disable_download,
                spice_settings->disable_upload
        );
        guac_client_for_owner(client, guac_spice_folder_expose,
                spice_client->shared_folder);
    }

    else
        g_object_set(spice_session, SPICE_PROPERTY_SHARED_DIR, NULL, NULL);
    
    /* Return the configured session. */
    return spice_session;

}

void* guac_spice_client_thread(void* data) {
    
    guac_client* client = (guac_client*) data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    guac_spice_settings* settings = spice_client->settings;
    spice_client->spice_mainloop = g_main_loop_new(NULL, false);

    /* Attempt connection */
    guac_client_log(client, GUAC_LOG_DEBUG, "Attempting initial connection to SPICE server.");
    spice_client->spice_session = guac_spice_get_session(client);
    int retries_remaining = settings->retries;

    /* If unsuccessful, retry as many times as specified */
    while (spice_client->spice_session == NULL && retries_remaining > 0) {

        guac_client_log(client, GUAC_LOG_INFO,
                "Connect failed. Waiting %ims before retrying...",
                GUAC_SPICE_CONNECT_INTERVAL);

        /* Wait for given interval then retry */
        guac_timestamp_msleep(GUAC_SPICE_CONNECT_INTERVAL);
        spice_client->spice_session = guac_spice_get_session(client);
        retries_remaining--;

    }

    /* If the final connect attempt fails, return error */
    if (spice_client->spice_session == NULL) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_NOT_FOUND,
                "Unable to connect to SPICE server.");
        return NULL;
    }

    guac_socket_flush(client->socket);
    
    guac_client_log(client, GUAC_LOG_DEBUG, "Connection configuration finished, calling spice_session_connect.");
    
    if(!spice_session_connect(spice_client->spice_session))
        return NULL;
    
    guac_client_log(client, GUAC_LOG_DEBUG, "Session connected, entering main loop.");

    /* Handle messages from SPICE server while client is running */
    while (client->state == GUAC_CLIENT_RUNNING) {

        /* Run the main loop. */
        g_main_loop_run(spice_client->spice_mainloop);
        guac_client_log(client, GUAC_LOG_DEBUG, "Finished main loop.");

        /* Wait for an error on the main channel. */
        if (spice_client->main_channel != NULL
                && spice_channel_get_error(SPICE_CHANNEL(spice_client->main_channel)) != NULL)
            break;
    }

    guac_client_log(client, GUAC_LOG_DEBUG, "Exited main loop, cleaning up.");
    
    /* Kill client and finish connection */
    if (spice_client->spice_session != NULL) {
        guac_client_log(client, GUAC_LOG_DEBUG, "Cleaning up SPICE session.");
        spice_session_disconnect(spice_client->spice_session);
        g_object_unref(spice_client->spice_session);
        spice_client->spice_session = NULL;
    }

    guac_client_stop(client);
    guac_client_log(client, GUAC_LOG_INFO, "Internal SPICE client disconnected");
    return NULL;

}
