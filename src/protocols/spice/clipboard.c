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

#include "clipboard.h"
#include "common/clipboard.h"
#include "spice.h"

#include <guacamole/client.h>
#include <guacamole/recording.h>
#include <guacamole/stream.h>
#include <guacamole/user.h>
#include <spice-client.h>
#include <spice/vd_agent.h>

#include <stdint.h>
#include <string.h>

/**
 * The SPICE clipboard selection used for all clipboard exchange. The standard
 * "CLIPBOARD" selection (as opposed to the X11 "PRIMARY" selection) is used,
 * matching the behavior expected by typical desktop environments.
 */
#define GUAC_SPICE_CLIPBOARD_SELECTION VD_AGENT_CLIPBOARD_SELECTION_CLIPBOARD

/**
 * Signal handler for the SPICE main channel "main-clipboard-selection-grab"
 * signal, invoked when the remote guest takes ownership of the clipboard. If
 * the guest is offering UTF-8 text, the data is requested from the guest.
 */
static void guac_spice_clipboard_grab(SpiceMainChannel* channel,
        guint selection, gpointer types, guint ntypes, gpointer data) {

    guac_client* client = (guac_client*) data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /* Ignore inbound clipboard if outbound (remote-to-client) transfer is
     * disabled */
    if (spice_client->settings->disable_copy)
        return;

    /* Request UTF-8 text from the guest, if offered */
    guint32* offered = (guint32*) types;
    for (guint i = 0; i < ntypes; i++) {
        if (offered[i] == VD_AGENT_CLIPBOARD_UTF8_TEXT) {
            spice_main_channel_clipboard_selection_request(channel, selection,
                    VD_AGENT_CLIPBOARD_UTF8_TEXT);
            return;
        }
    }

}

/**
 * Signal handler for the SPICE main channel "main-clipboard-selection" signal,
 * invoked when the remote guest sends clipboard data (in response to an earlier
 * request). The received data is pushed to all connected Guacamole users.
 */
static void guac_spice_clipboard_selection(SpiceMainChannel* channel,
        guint selection, guint type, gpointer data, guint size,
        gpointer user_data) {

    guac_client* client = (guac_client*) user_data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /* Ignore inbound clipboard if outbound transfer is disabled or unsupported
     * data type is received */
    if (spice_client->settings->disable_copy
            || type != VD_AGENT_CLIPBOARD_UTF8_TEXT
            || spice_client->clipboard == NULL)
        return;

    /* Replace clipboard contents with received text and broadcast to users */
    guac_common_clipboard_reset(spice_client->clipboard, "text/plain");
    guac_common_clipboard_append(spice_client->clipboard, (char*) data, size);
    guac_common_clipboard_send(spice_client->clipboard, client);

}

/**
 * Signal handler for the SPICE main channel "main-clipboard-selection-request"
 * signal, invoked when the remote guest requests the current clipboard
 * contents. The most recently received Guacamole clipboard data is sent to the
 * guest.
 *
 * @return
 *     TRUE if the request was handled, FALSE otherwise.
 */
static gboolean guac_spice_clipboard_request(SpiceMainChannel* channel,
        guint selection, guint type, gpointer data) {

    guac_client* client = (guac_client*) data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /* Ignore requests if inbound (client-to-remote) transfer is disabled, the
     * requested type is unsupported, or no clipboard data is available */
    if (spice_client->settings->disable_paste
            || type != VD_AGENT_CLIPBOARD_UTF8_TEXT
            || spice_client->clipboard == NULL)
        return FALSE;

    /* Provide the current clipboard contents to the guest */
    spice_main_channel_clipboard_selection_notify(channel, selection,
            VD_AGENT_CLIPBOARD_UTF8_TEXT,
            (const guchar*) spice_client->clipboard->buffer,
            spice_client->clipboard->length);

    return TRUE;

}

void guac_spice_clipboard_connect(guac_client* client,
        SpiceMainChannel* channel) {

    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /* Clipboard integration is unavailable if disabled entirely */
    if (spice_client->settings->disable_clipboard)
        return;

    g_signal_connect(channel, "main-clipboard-selection-grab",
            G_CALLBACK(guac_spice_clipboard_grab), client);
    g_signal_connect(channel, "main-clipboard-selection",
            G_CALLBACK(guac_spice_clipboard_selection), client);
    g_signal_connect(channel, "main-clipboard-selection-request",
            G_CALLBACK(guac_spice_clipboard_request), client);

}

int guac_spice_clipboard_handler(guac_user* user, guac_stream* stream,
        char* mimetype) {

    guac_spice_client* spice_client = (guac_spice_client*) user->client->data;

    /* Ignore stream creation if no clipboard structure is available */
    guac_common_clipboard* clipboard = spice_client->clipboard;
    if (clipboard == NULL)
        return 0;

    /* Clear clipboard and prepare for new data */
    guac_common_clipboard_reset(clipboard, mimetype);

    /* Set handlers for clipboard stream */
    stream->blob_handler = guac_spice_clipboard_blob_handler;
    stream->end_handler = guac_spice_clipboard_end_handler;

    /* Report clipboard within recording */
    if (spice_client->recording != NULL)
        guac_recording_report_clipboard_begin(spice_client->recording, stream,
                mimetype);

    return 0;

}

int guac_spice_clipboard_blob_handler(guac_user* user, guac_stream* stream,
        void* data, int length) {

    guac_spice_client* spice_client = (guac_spice_client*) user->client->data;

    guac_common_clipboard* clipboard = spice_client->clipboard;
    if (clipboard == NULL)
        return 0;

    /* Report clipboard blob within recording */
    if (spice_client->recording != NULL)
        guac_recording_report_clipboard_blob(spice_client->recording, stream,
                data, length);

    /* Append new data */
    guac_common_clipboard_append(clipboard, (char*) data, length);

    return 0;

}

int guac_spice_clipboard_end_handler(guac_user* user, guac_stream* stream) {

    guac_client* client = user->client;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    guac_common_clipboard* clipboard = spice_client->clipboard;
    if (clipboard == NULL)
        return 0;

    /* Report clipboard stream end within recording */
    if (spice_client->recording != NULL)
        guac_recording_report_clipboard_end(spice_client->recording, stream);

    /* Take ownership of the SPICE clipboard on behalf of the client, offering
     * the newly-received text to the remote guest. The guest will subsequently
     * request the data via "main-clipboard-selection-request". */
    if (spice_client->main_channel != NULL
            && !spice_client->settings->disable_paste) {
        guint32 types[] = { VD_AGENT_CLIPBOARD_UTF8_TEXT };
        spice_main_channel_clipboard_selection_grab(spice_client->main_channel,
                GUAC_SPICE_CLIPBOARD_SELECTION, types, 1);
    }

    return 0;

}
