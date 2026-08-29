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

#include "audio.h"
#include "channels.h"
#include "clipboard.h"
#include "cursor.h"
#include "display.h"
#include "input.h"
#include "spice.h"

#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <spice-client.h>

#include <string.h>

/**
 * Signal handler for the "channel-event" signal of any SPICE channel. Handles
 * connection-level errors and closure by aborting or stopping the Guacamole
 * connection as appropriate.
 */
static void guac_spice_channel_event(SpiceChannel* channel,
        SpiceChannelEvent event, gpointer data) {

    guac_client* client = (guac_client*) data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    switch (event) {

        case SPICE_CHANNEL_OPENED:
            guac_client_log(client, GUAC_LOG_DEBUG, "SPICE channel opened.");

            /* Once the display channel is open (capabilities negotiated), and
             * only if the user configured a preferred video codec, ask the
             * server to prefer it for streamed video regions. The server's
             * default codec order lists MJPEG first, so without an explicit
             * preference the streamed codec is always MJPEG even when
             * H.264/VP9/VP8 encoding is available. This is opt-in (NULL by
             * default) because some spice-server builds crash when actually
             * encoding a GStreamer codec. Runs on the SPICE event-loop thread,
             * so no marshalling is required. */
            if (SPICE_IS_DISPLAY_CHANNEL(channel)
                    && spice_client->settings->preferred_video_codec != NULL) {

                const char* pref = spice_client->settings->preferred_video_codec;

                /* Build an ordered preference list: the requested codec first,
                 * with MJPEG appended as a universally-supported fallback. */
                gint codecs[4];
                int n = 0;

                if (strcmp(pref, "h264") == 0) {
                    codecs[n++] = SPICE_VIDEO_CODEC_TYPE_H264;
                    codecs[n++] = SPICE_VIDEO_CODEC_TYPE_VP9;
                    codecs[n++] = SPICE_VIDEO_CODEC_TYPE_VP8;
                }
                else if (strcmp(pref, "vp9") == 0) {
                    codecs[n++] = SPICE_VIDEO_CODEC_TYPE_VP9;
                    codecs[n++] = SPICE_VIDEO_CODEC_TYPE_VP8;
                    codecs[n++] = SPICE_VIDEO_CODEC_TYPE_H264;
                }
                else if (strcmp(pref, "vp8") == 0) {
                    codecs[n++] = SPICE_VIDEO_CODEC_TYPE_VP8;
                    codecs[n++] = SPICE_VIDEO_CODEC_TYPE_VP9;
                    codecs[n++] = SPICE_VIDEO_CODEC_TYPE_H264;
                }
                else if (strcmp(pref, "mjpeg") != 0) {
                    guac_client_log(client, GUAC_LOG_WARNING,
                            "Ignoring unknown preferred-video-codec \"%s\" "
                            "(expected h264, vp9, vp8, or mjpeg).", pref);
                    n = -1;
                }

                if (n >= 0) {

                    codecs[n++] = SPICE_VIDEO_CODEC_TYPE_MJPEG;

                    GError* codec_error = NULL;
                    if (spice_display_channel_change_preferred_video_codec_types(
                            channel, codecs, n, &codec_error))
                        guac_client_log(client, GUAC_LOG_INFO,
                                "Requested SPICE server prefer \"%s\" video "
                                "encoding for streamed regions.", pref);
                    else {
                        guac_client_log(client, GUAC_LOG_DEBUG,
                                "Preferred video codec selection unavailable: "
                                "%s", codec_error != NULL ? codec_error->message
                                                    : "unsupported by server");
                        g_clear_error(&codec_error);
                    }

                }

            }
            break;

        case SPICE_CHANNEL_ERROR_AUTH:
            guac_client_abort(client, GUAC_PROTOCOL_STATUS_CLIENT_UNAUTHORIZED,
                    "SPICE authentication failed. Verify the configured "
                    "password (ticket).");
            break;

        case SPICE_CHANNEL_ERROR_TLS:
            guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR,
                    "TLS error while connecting to SPICE server.");
            break;

        case SPICE_CHANNEL_ERROR_CONNECT:
            guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_NOT_FOUND,
                    "Unable to connect to SPICE server.");
            break;

        case SPICE_CHANNEL_ERROR_LINK:
        case SPICE_CHANNEL_ERROR_IO:
            guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_ERROR,
                    "Connection to SPICE server failed or was lost.");
            break;

        default:
            /* All other events (including normal closure) are not treated as
             * errors here; connection teardown is driven by the client state */
            break;

    }

}

/**
 * Signal handler for the SPICE main channel "main-mouse-update" signal.
 * Records the currently negotiated mouse mode so that pointer events can be
 * sent using the correct (absolute or relative) coordinate scheme.
 */
static void guac_spice_main_mouse_update(SpiceMainChannel* channel,
        gpointer data) {

    guac_client* client = (guac_client*) data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    gint mouse_mode = SPICE_MOUSE_MODE_CLIENT;
    g_object_get(channel, "mouse-mode", &mouse_mode, NULL);
    spice_client->mouse_mode = mouse_mode;

    guac_client_log(client, GUAC_LOG_DEBUG, "SPICE mouse mode is now %s.",
            mouse_mode == SPICE_MOUSE_MODE_SERVER ? "server" : "client");

}

void guac_spice_channel_new(SpiceSession* session, SpiceChannel* channel,
        gpointer data) {

    guac_client* client = (guac_client*) data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /* All channels report connection-level errors the same way */
    g_signal_connect(channel, "channel-event",
            G_CALLBACK(guac_spice_channel_event), client);

    /* Main channel: session-wide state, mouse mode, and clipboard */
    if (SPICE_IS_MAIN_CHANNEL(channel)) {
        guac_client_log(client, GUAC_LOG_DEBUG, "Connecting SPICE main channel.");
        spice_client->main_channel = SPICE_MAIN_CHANNEL(channel);
        g_signal_connect(channel, "main-mouse-update",
                G_CALLBACK(guac_spice_main_mouse_update), client);

        /* Track agent readiness (connection + monitors-config support) so a
         * queued dynamic display resize can be sent once the guest is ready.
         * The capability is announced after connection, so watch both the
         * connection state and subsequent agent updates. */
        g_signal_connect(channel, "notify::agent-connected",
                G_CALLBACK(guac_spice_resize_agent_update), client);
        g_signal_connect(channel, "main-agent-update",
                G_CALLBACK(guac_spice_resize_agent_updated), client);

        guac_spice_clipboard_connect(client, SPICE_MAIN_CHANNEL(channel));
    }

    /* Display channel: remote framebuffer */
    else if (SPICE_IS_DISPLAY_CHANNEL(channel)) {
        guac_client_log(client, GUAC_LOG_DEBUG, "Connecting SPICE display channel.");
        guac_spice_display_channel_connect(client, channel);
    }

    /* Inputs channel: keyboard and mouse */
    else if (SPICE_IS_INPUTS_CHANNEL(channel)) {
        guac_client_log(client, GUAC_LOG_DEBUG, "Connecting SPICE inputs channel.");
        spice_client->inputs_channel = SPICE_INPUTS_CHANNEL(channel);

        /* Track remote keyboard lock state (Caps Lock, Num Lock, etc.) so that
         * keysym translation can keep local and remote lock states in sync */
        g_signal_connect(channel, "inputs-modifiers",
                G_CALLBACK(guac_spice_keyboard_set_indicators), client);
    }

    /* Cursor channel: remote cursor shape */
    else if (SPICE_IS_CURSOR_CHANNEL(channel)) {
        guac_client_log(client, GUAC_LOG_DEBUG, "Connecting SPICE cursor channel.");
        guac_spice_cursor_channel_connect(client, channel);
    }

    /* Playback channel: audio output */
    else if (SPICE_IS_PLAYBACK_CHANNEL(channel)) {
        guac_client_log(client, GUAC_LOG_DEBUG, "Connecting SPICE playback channel.");
        guac_spice_playback_channel_connect(client, channel);
    }

    /* Record channel: audio input (e.g. microphone) */
    else if (SPICE_IS_RECORD_CHANNEL(channel)) {
        guac_client_log(client, GUAC_LOG_DEBUG, "Connecting SPICE record channel.");
        guac_spice_record_channel_connect(client, channel);
    }

#ifdef ENABLE_SPICE_WEBDAV
    /* WebDAV channel: folder sharing (driven internally by spice-gtk) */
    else if (SPICE_IS_WEBDAV_CHANNEL(channel)) {
        guac_client_log(client, GUAC_LOG_DEBUG, "Connecting SPICE WebDAV channel.");
    }
#endif

    /* USB redirection, smartcard, and microphone (record) channels are not
     * supported by the headless guacd proxy and are intentionally left
     * unconnected. */
    else {
        guac_client_log(client, GUAC_LOG_DEBUG,
                "Ignoring unsupported SPICE channel type.");
        return;
    }

    /* Open the channel now that handlers are registered */
    spice_channel_connect(channel);

}

void guac_spice_channel_destroy(SpiceSession* session, SpiceChannel* channel,
        gpointer data) {

    guac_client* client = (guac_client*) data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /* Release references to channels being destroyed */
    if (SPICE_IS_MAIN_CHANNEL(channel))
        spice_client->main_channel = NULL;
    else if (SPICE_IS_DISPLAY_CHANNEL(channel))
        spice_client->display_channel = NULL;
    else if (SPICE_IS_INPUTS_CHANNEL(channel))
        spice_client->inputs_channel = NULL;
    else if (SPICE_IS_CURSOR_CHANNEL(channel))
        spice_client->cursor_channel = NULL;
    else if (SPICE_IS_RECORD_CHANNEL(channel))
        spice_client->record_channel = NULL;

}
