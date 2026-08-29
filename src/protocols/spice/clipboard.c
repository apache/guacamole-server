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
#include <guacamole/protocol.h>
#include <guacamole/recording.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>
#include <guacamole/user.h>
#include <spice-client.h>
#include <spice/vd_agent.h>

#include <limits.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>

/**
 * The SPICE clipboard selection used for all clipboard exchange. The standard
 * "CLIPBOARD" selection (as opposed to the X11 "PRIMARY" selection) is used,
 * matching the behavior expected by typical desktop environments.
 */
#define GUAC_SPICE_CLIPBOARD_SELECTION VD_AGENT_CLIPBOARD_SELECTION_CLIPBOARD

/**
 * The order in which VD_AGENT clipboard data types are preferred when the guest
 * offers several at once. UTF-8 text is preferred over images (it is the most
 * common paste target and cheapest to transfer), and among image formats PNG is
 * preferred, as it is the format every image-capable SPICE peer is required to
 * support. Only types Guacamole can represent as a clipboard mimetype are
 * listed; any other offered type is ignored.
 */
static const guint32 guac_spice_clipboard_preference[] = {
    VD_AGENT_CLIPBOARD_UTF8_TEXT,
    VD_AGENT_CLIPBOARD_IMAGE_PNG,
    VD_AGENT_CLIPBOARD_IMAGE_BMP,
    VD_AGENT_CLIPBOARD_IMAGE_JPG,
    VD_AGENT_CLIPBOARD_IMAGE_TIFF,
};

/**
 * Returns the VD_AGENT clipboard data type corresponding to the given
 * Guacamole clipboard mimetype, or VD_AGENT_CLIPBOARD_NONE if the mimetype has
 * no SPICE equivalent. Any textual ("text/...") mimetype maps to UTF-8 text.
 *
 * @param mimetype
 *     The Guacamole clipboard mimetype to translate.
 *
 * @return
 *     The corresponding VD_AGENT_CLIPBOARD_* type, or VD_AGENT_CLIPBOARD_NONE
 *     if unsupported.
 */
static guint32 guac_spice_clipboard_type_for_mimetype(const char* mimetype) {

    if (mimetype == NULL)
        return VD_AGENT_CLIPBOARD_NONE;

    if (strncmp(mimetype, "text/", 5) == 0)
        return VD_AGENT_CLIPBOARD_UTF8_TEXT;

    if (strcmp(mimetype, "image/png") == 0)
        return VD_AGENT_CLIPBOARD_IMAGE_PNG;

    if (strcmp(mimetype, "image/bmp") == 0
            || strcmp(mimetype, "image/x-bmp") == 0
            || strcmp(mimetype, "image/x-ms-bmp") == 0)
        return VD_AGENT_CLIPBOARD_IMAGE_BMP;

    if (strcmp(mimetype, "image/jpeg") == 0
            || strcmp(mimetype, "image/jpg") == 0)
        return VD_AGENT_CLIPBOARD_IMAGE_JPG;

    if (strcmp(mimetype, "image/tiff") == 0)
        return VD_AGENT_CLIPBOARD_IMAGE_TIFF;

    return VD_AGENT_CLIPBOARD_NONE;

}

/**
 * Returns the Guacamole clipboard mimetype corresponding to the given VD_AGENT
 * clipboard data type, or NULL if the type has no Guacamole equivalent.
 *
 * @param type
 *     The VD_AGENT_CLIPBOARD_* type to translate.
 *
 * @return
 *     A statically-allocated Guacamole mimetype string, or NULL if unsupported.
 */
static const char* guac_spice_clipboard_mimetype_for_type(guint32 type) {

    switch (type) {

        case VD_AGENT_CLIPBOARD_UTF8_TEXT:
            return "text/plain";

        case VD_AGENT_CLIPBOARD_IMAGE_PNG:
            return "image/png";

        case VD_AGENT_CLIPBOARD_IMAGE_BMP:
            return "image/bmp";

        case VD_AGENT_CLIPBOARD_IMAGE_JPG:
            return "image/jpeg";

        case VD_AGENT_CLIPBOARD_IMAGE_TIFF:
            return "image/tiff";

        default:
            return NULL;

    }

}

/**
 * Records a clipboard transfer within the session recording, annotated with its
 * direction. The annotation is emitted as a log instruction directly to the
 * recording socket (not to any live client), so playback tooling ignores it
 * while an audit of the recording can distinguish data copied *out* of the
 * guest ("guest-to-client" — a potential data-exfiltration path) from data
 * pasted *into* the guest ("client-to-guest"). Does nothing unless clipboard
 * recording is active.
 *
 * @param recording
 *     The session recording, or NULL if no recording is in progress.
 *
 * @param outbound
 *     Non-zero if the transfer is guest-to-client (leaving the guest), zero if
 *     it is client-to-guest (entering the guest).
 *
 * @param stream_index
 *     The index of the recording stream carrying the clipboard data, allowing
 *     the annotation to be correlated with the following clipboard/blob/end
 *     instructions.
 *
 * @param mimetype
 *     The mimetype of the clipboard data.
 *
 * @param length
 *     The length of the clipboard data in bytes, or a negative value if not yet
 *     known (as when a client-to-guest transfer is only beginning).
 */
static void guac_spice_clipboard_record_direction(guac_recording* recording,
        int outbound, int stream_index, const char* mimetype, int length) {

    /* Match the gating of guac_recording_report_clipboard() so the annotation
     * is present exactly when the clipboard data it describes is */
    if (recording == NULL || !recording->include_clipboard
            || recording->socket == NULL)
        return;

    const char* direction = outbound ? "guest-to-client" : "client-to-guest";

    if (length >= 0)
        guac_protocol_send_log(recording->socket,
                "clipboard stream=%d direction=%s mimetype=%s bytes=%d",
                stream_index, direction, mimetype, length);
    else
        guac_protocol_send_log(recording->socket,
                "clipboard stream=%d direction=%s mimetype=%s",
                stream_index, direction, mimetype);

}

/**
 * Handler which performs a deferred SPICE clipboard-selection grab on the
 * event-loop thread. The offered types array is carried in call->data.
 */
static void guac_spice_do_clipboard_grab(guac_spice_deferred_call* call) {
    spice_main_channel_clipboard_selection_grab((SpiceMainChannel*) call->channel,
            call->args[0], (guint32*) call->data, (int) call->args[1]);
}

/**
 * Signal handler for the SPICE main channel "main-clipboard-selection-grab"
 * signal, invoked when the remote guest takes ownership of the clipboard. The
 * best Guacamole-representable type offered by the guest (see
 * guac_spice_clipboard_preference) is requested from the guest.
 */
static void guac_spice_clipboard_grab(SpiceMainChannel* channel,
        guint selection, gpointer types, guint ntypes, gpointer data) {

    guac_client* client = (guac_client*) data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /* Ignore inbound clipboard if outbound (remote-to-client) transfer is
     * disabled */
    if (spice_client->settings->disable_copy)
        return;

    /* Request the most-preferred type the guest is offering that Guacamole can
     * represent (text over image, PNG over other image formats) */
    guint32* offered = (guint32*) types;
    for (unsigned int p = 0;
            p < sizeof(guac_spice_clipboard_preference) / sizeof(guint32); p++) {
        for (guint i = 0; i < ntypes; i++) {
            if (offered[i] == guac_spice_clipboard_preference[p]) {
                spice_main_channel_clipboard_selection_request(channel,
                        selection, guac_spice_clipboard_preference[p]);
                return;
            }
        }
    }

}

/**
 * Signal handler for the SPICE main channel "main-clipboard-selection" signal,
 * invoked when the remote guest sends clipboard data (in response to an earlier
 * request). The received data is pushed to all connected Guacamole users. Both
 * UTF-8 text and image data (PNG, BMP, JPEG, TIFF) are supported.
 */
static void guac_spice_clipboard_selection(SpiceMainChannel* channel,
        guint selection, guint type, gpointer data, guint size,
        gpointer user_data) {

    guac_client* client = (guac_client*) user_data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /* Ignore inbound clipboard if outbound transfer is disabled or no clipboard
     * structure is available */
    if (spice_client->settings->disable_copy
            || spice_client->clipboard == NULL)
        return;

    /* Ignore data of a type Guacamole cannot represent */
    const char* mimetype = guac_spice_clipboard_mimetype_for_type(type);
    if (mimetype == NULL)
        return;

    /* Reject an implausibly large payload rather than passing a size that would
     * wrap negative when narrowed to the int length taken by the clipboard
     * (the clipboard itself caps the retained data at clipboard-buffer-size) */
    if (size > (guint) INT_MAX)
        return;

    /* Replace clipboard contents with received data and broadcast to users. The
     * data may be binary (for images); guac_common_clipboard stores and streams
     * it by length, so this is safe for non-text mimetypes. */
    guac_common_clipboard_reset(spice_client->clipboard, mimetype);
    guac_common_clipboard_append(spice_client->clipboard, (char*) data, (int) size);
    guac_common_clipboard_send(spice_client->clipboard, client);

    /* Record this guest-to-client transfer. Unlike client-to-guest pastes (which
     * are recorded from their inbound stream), the outbound clipboard is
     * broadcast per-user and so is not otherwise captured by the recording. This
     * is the data-exfiltration direction, so it is important to audit. */
    guac_recording* recording = spice_client->recording;
    if (recording != NULL && recording->clipboard_stream != NULL) {
        guac_spice_clipboard_record_direction(recording, 1,
                recording->clipboard_stream->index, mimetype, (int) size);
        guac_recording_report_clipboard(recording, mimetype,
                (const char*) data, (int) size);
    }

}

/**
 * Signal handler for the SPICE main channel "main-clipboard-selection-request"
 * signal, invoked when the remote guest requests the current clipboard
 * contents. The most recently received Guacamole clipboard data is sent to the
 * guest, provided the requested type matches the type Guacamole currently holds.
 *
 * @return
 *     TRUE if the request was handled, FALSE otherwise.
 */
static gboolean guac_spice_clipboard_request(SpiceMainChannel* channel,
        guint selection, guint type, gpointer data) {

    guac_client* client = (guac_client*) data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /* Ignore requests if inbound (client-to-remote) transfer is disabled or no
     * clipboard data is available */
    guac_common_clipboard* clipboard = spice_client->clipboard;
    if (spice_client->settings->disable_paste || clipboard == NULL)
        return FALSE;

    /* Hold the clipboard lock across the type check and notify so a concurrent
     * paste from a Guacamole user thread (which resets/appends the clipboard)
     * cannot swap the contents out from under us mid-request, which would send
     * the guest a mimetype/buffer/length mismatch (especially for images). */
    pthread_mutex_lock(&clipboard->lock);

    gboolean handled = FALSE;

    /* Only respond if the guest is requesting the type we actually hold; we do
     * not transcode between clipboard formats */
    if (guac_spice_clipboard_type_for_mimetype(clipboard->mimetype) == type) {
        spice_main_channel_clipboard_selection_notify(channel, selection, type,
                (const guchar*) clipboard->buffer, clipboard->length);
        handled = TRUE;
    }

    pthread_mutex_unlock(&clipboard->lock);

    return handled;

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

    /* Report clipboard within recording, annotated as a client-to-guest paste
     * (the length is not yet known as the data is still streaming) */
    if (spice_client->recording != NULL) {
        guac_spice_clipboard_record_direction(spice_client->recording, 0,
                stream->index, mimetype, -1);
        guac_recording_report_clipboard_begin(spice_client->recording, stream,
                mimetype);
    }

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
     * the newly-received data to the remote guest. The guest will subsequently
     * request the data via "main-clipboard-selection-request". The offered type
     * matches the mimetype the client sent (text or image); an unsupported
     * mimetype is not offered. */
    guint32 type = guac_spice_clipboard_type_for_mimetype(clipboard->mimetype);
    if (spice_client->main_channel != NULL
            && !spice_client->settings->disable_paste
            && type != VD_AGENT_CLIPBOARD_NONE) {

        /* Marshal the grab onto the SPICE event-loop thread; this handler runs
         * on a Guacamole user thread, and spice-gtk channel functions must not
         * be called off the loop thread (see guac_spice_defer_call) */
        guint32* types = g_new(guint32, 1);
        types[0] = type;

        guac_spice_deferred_call* call = g_new0(guac_spice_deferred_call, 1);
        call->handler = guac_spice_do_clipboard_grab;
        call->channel = spice_client->main_channel;
        call->args[0] = GUAC_SPICE_CLIPBOARD_SELECTION;
        call->args[1] = 1; /* number of offered types */
        call->data = types;
        call->data_len = sizeof(guint32);
        guac_spice_defer_call(call);

    }

    return 0;

}
