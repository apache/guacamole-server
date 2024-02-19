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
#include "client.h"
#include "clipboard.h"
#include "common/clipboard.h"
#include "common/iconv.h"
#include "spice.h"
#include "spice-constants.h"
#include "user.h"

#include <guacamole/client.h>
#include <guacamole/stream.h>
#include <guacamole/user.h>

#include <spice-1/spice/vd_agent.h>

int guac_spice_clipboard_handler(guac_user* user, guac_stream* stream,
        char* mimetype) {

    guac_spice_client* spice_client = (guac_spice_client*) user->client->data;

    /* Some versions of VDAgent do not support sending clipboard data. */
    if (!spice_main_channel_agent_test_capability(spice_client->main_channel, VD_AGENT_CAP_CLIPBOARD_BY_DEMAND)) {
        guac_client_log(user->client, GUAC_LOG_WARNING, "Spice guest agent does"
            " not support sending clipboard data on demand.");
        return 0;
    }

    /* Clear the current clipboard and send the grab command to the agent. */
    guac_common_clipboard_reset(spice_client->clipboard, mimetype);
    guint32 clipboard_types[] = { VD_AGENT_CLIPBOARD_UTF8_TEXT };
    spice_main_channel_clipboard_selection_grab(spice_client->main_channel,
            VD_AGENT_CLIPBOARD_SELECTION_CLIPBOARD, clipboard_types, 1);

    /* Set handlers for clipboard stream */
    stream->blob_handler = guac_spice_clipboard_blob_handler;
    stream->end_handler = guac_spice_clipboard_end_handler;

    return 0;
}

int guac_spice_clipboard_blob_handler(guac_user* user, guac_stream* stream,
        void* data, int length) {

    /* Append new data */
    guac_spice_client* spice_client = (guac_spice_client*) user->client->data;
    guac_common_clipboard_append(spice_client->clipboard, (char*) data, length);

    return 0;
}

int guac_spice_clipboard_end_handler(guac_user* user, guac_stream* stream) {

    guac_spice_client* spice_client = (guac_spice_client*) user->client->data;

    /* Send via Spice only if finished connecting */
    if (spice_client->main_channel != NULL) {
        spice_main_channel_clipboard_selection_notify(spice_client->main_channel,
            VD_AGENT_CLIPBOARD_SELECTION_CLIPBOARD,
            VD_AGENT_CLIPBOARD_UTF8_TEXT,
            (const unsigned char*) spice_client->clipboard->buffer,
            spice_client->clipboard->length);
    }

    return 0;
}

void guac_spice_clipboard_selection_handler(SpiceMainChannel* channel,
        guint selection, guint type, gpointer data, guint size,
        guac_client* client) {

    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /* Loop through clipboard types - currently Guacamole only supports text. */
    switch (type) {
        case VD_AGENT_CLIPBOARD_UTF8_TEXT:
            guac_client_log(client, GUAC_LOG_DEBUG, "Notifying client of text "
                    " on clipboard from server: %s", (char *) data);
            guac_common_clipboard_append(spice_client->clipboard, (char *) data, size);
            break;

        default:
            guac_client_log(client, GUAC_LOG_WARNING, "Guacamole currently does"
                " not support clipboard data other than plain text.");
    }

    guac_common_clipboard_send(spice_client->clipboard, client);

}

void guac_spice_clipboard_selection_grab_handler(SpiceMainChannel* channel,
        guint selection, guint32* types, guint ntypes, guac_client* client) {

    guac_client_log(client, GUAC_LOG_DEBUG, "Notifying client of clipboard grab"
            " in the guest.");
    guac_client_log(client, GUAC_LOG_DEBUG, "Arg: channel: 0x%08x", channel);
    guac_client_log(client, GUAC_LOG_DEBUG, "Arg: selection: 0x%08x", selection);
    guac_client_log(client, GUAC_LOG_DEBUG, "Arg: types: 0x%08x", types);
    guac_client_log(client, GUAC_LOG_DEBUG, "Arg: ntypes: 0x%08x", ntypes);

    /* Ignore selection types other than clipboard. */
    if (selection != VD_AGENT_CLIPBOARD_SELECTION_CLIPBOARD) {
        guac_client_log(client, GUAC_LOG_WARNING, "Unsupported clipboard grab type: %d", selection);
        return;
    }

    /* Loop through the data types sent by the Spice server and process them. */
    for (int i = 0; i < ntypes; i++) {

        /* Currently Guacamole only supports text. */
        if (types[i] != VD_AGENT_CLIPBOARD_UTF8_TEXT) {
            guac_client_log(client, GUAC_LOG_WARNING, "Unsupported clipboard data type: %d", types[i]);
            continue;
        }

        /* Reset our clipboard and request the data from the Spice serer. */
        guac_spice_client* spice_client = (guac_spice_client*) client->data;
        guac_common_clipboard_reset(spice_client->clipboard, "text/plain");
        spice_main_channel_clipboard_selection_request(channel, selection, types[i]);
    }

    

}

void guac_spice_clipboard_selection_release_handler(SpiceMainChannel* channel,
        guint selection, guac_client* client) {

    guac_client_log(client, GUAC_LOG_DEBUG, "Notifying client of clipboard"
            " release in the guest.");

    /* Transfer data from guest to Guacamole clipboard. */
    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    guac_common_clipboard_send(spice_client->clipboard, client);

}

void guac_spice_clipboard_selection_request_handler(SpiceMainChannel* channel,
        guint selection, guint type, guac_client* client) {

    guac_client_log(client, GUAC_LOG_DEBUG, "Requesting clipboard data from"
            " the client.");

    /* Guacamole only supports one clipboard selection type. */
    if (selection != VD_AGENT_CLIPBOARD_SELECTION_CLIPBOARD) {
        guac_client_log(client, GUAC_LOG_WARNING, "Unsupported selection type: %d", selection);
        return;
    }

    /* Currently Guacamole only implements text support - other types are images. */
    if (type != VD_AGENT_CLIPBOARD_UTF8_TEXT) {
        guac_client_log(client, GUAC_LOG_WARNING, "Unsupported clipboard data type: %d", type);
        return;
    }

    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    guac_client_log(client, GUAC_LOG_DEBUG, "Sending clipboard data to server: %s", spice_client->clipboard->buffer);

    /* Send the clipboard data to the guest. */
    spice_main_channel_clipboard_selection_notify(channel,
            selection,
            type,
            (const unsigned char*) spice_client->clipboard->buffer,
            spice_client->clipboard->length);

}
