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
 *
 * @param channel
 *     The SPICE channel that emitted the signal. Used to identify the channel
 *     type in log messages and, upon SPICE_CHANNEL_OPENED, to identify the
 *     display channel when applying the preferred video codec.
 *
 * @param event
 *     The event that occurred on the channel (opened, closed, or one of the
 *     error conditions).
 *
 * @param data
 *     The guac_client associated with the connection, as passed when the
 *     handler was connected.
 */
static void guac_spice_channel_event(SpiceChannel* channel,
        SpiceChannelEvent event, gpointer data) {

    guac_client* client = (guac_client*) data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    switch (event) {

        case SPICE_CHANNEL_OPENED: {

            gint type = 0;
            g_object_get(channel, "channel-type", &type, NULL);
            guac_client_log(client, GUAC_LOG_DEBUG, "SPICE %s channel opened.",
                    spice_channel_type_to_string(type));

            /*
             * Only the display channel requires action here: the video codec
             * preference can be applied only once the display channel's
             * capabilities have been negotiated, which is complete when the
             * channel is opened. All other channels are driven entirely by
             * their own signals, connected in guac_spice_channel_new().
             *
             * Once the display channel is open (capabilities negotiated), and
             * only if the user configured a preferred video codec, ask the
             * server to prefer it for streamed video regions. The server's
             * default codec order lists MJPEG first, so without an explicit
             * preference the streamed codec is always MJPEG even when
             * H.264/VP9/VP8 encoding is available. This is opt-in (NULL by
             * default) because some spice-server builds crash when actually
             * encoding a GStreamer codec. Runs on the SPICE event-loop thread,
             * so no marshalling is required.
             */
            if (SPICE_IS_DISPLAY_CHANNEL(channel)
                    && spice_client->settings->preferred_video_codec != NULL) {

                const char* pref = spice_client->settings->preferred_video_codec;

                /*
                 * Build an ordered preference list: the requested codec first,
                 * followed by the other GStreamer codecs, with MJPEG appended
                 * last. MJPEG is the baseline codec that both spice-gtk and
                 * spice-server always support, so it is always the final
                 * fallback; a preference of "mjpeg" yields a list containing
                 * only MJPEG. An unrecognised value sets n to -1, in which
                 * case the preference API is not called at all and the
                 * server's default codec order (MJPEG first) is left
                 * unchanged.
                 */
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

        }

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
            /*
             * All other events (including normal closure) are not treated as
             * errors here; connection teardown is driven by the client state
             */
            break;

    }

}

/**
 * Signal handler for the SPICE main channel "main-mouse-update" signal.
 * Records the currently negotiated mouse mode so that pointer events can be
 * sent using the correct (absolute or relative) coordinate scheme.
 *
 * @param channel
 *     The SPICE main channel that emitted the signal. Its "mouse-mode"
 *     property is read to determine the negotiated mouse mode.
 *
 * @param data
 *     The guac_client associated with the connection, as passed when the
 *     handler was connected.
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

        /*
         * Track agent readiness (connection + monitors-config support) so a
         * queued dynamic display resize can be sent once the guest is ready.
         * The capability is announced after connection, so watch both the
         * connection state and subsequent agent updates.
         */
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

        /*
         * Track remote keyboard lock state (Caps Lock, Num Lock, etc.) so that
         * keysym translation can keep local and remote lock states in sync
         */
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
    /*
     * WebDAV channel: folder sharing. No handlers are needed, as spice-gtk
     * drives this channel internally; it is connected below like every other
     * supported channel.
     */
    else if (SPICE_IS_WEBDAV_CHANNEL(channel)) {
        guac_client_log(client, GUAC_LOG_DEBUG, "Connecting SPICE WebDAV channel.");
    }
#endif

    /*
     * USB redirection and smartcard channels are not supported by guacd, which
     * has no local devices to redirect, and are intentionally left
     * unconnected.
     */
    else {
        gint type = 0;
        g_object_get(channel, "channel-type", &type, NULL);
        guac_client_log(client, GUAC_LOG_DEBUG, "Ignoring unsupported SPICE "
                "channel type \"%s\" (%i).",
                spice_channel_type_to_string(type), type);
        return;
    }

    /*
     * Open the channel now that handlers are registered. All supported
     * channels, including those above that register no handlers, reach this
     * point.
     */
    spice_channel_connect(channel);

}

void guac_spice_channel_destroy(SpiceSession* session, SpiceChannel* channel,
        gpointer data) {

    guac_client* client = (guac_client*) data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /*
     * Forget the pointers to channels being destroyed. spice-gtk owns and
     * frees all channels; no references are taken on them here, so nothing
     * needs to be unreferenced. Signal handlers connected to a channel are
     * dropped automatically when the channel is disposed. While the event
     * loop is active, this handler runs on the SPICE event-loop thread, so
     * deferred calls (which also run on that thread) observe the NULL pointer
     * and do nothing. It is also emitted while the client thread drains the
     * main context after spice_session_disconnect(), at which point deferred
     * calls are no longer dispatched to their handlers.
     *
     * Each pointer is cleared only if it refers to the channel being
     * destroyed, so destroying a secondary channel of the same type (such as
     * an additional display channel of a multi-head guest) does not clear the
     * state of the tracked one.
     */
    if ((gpointer) channel == (gpointer) spice_client->main_channel)
        spice_client->main_channel = NULL;
    else if ((gpointer) channel == (gpointer) spice_client->display_channel) {

        spice_client->display_channel = NULL;

        /*
         * The primary surface buffer belongs to the channel and is freed
         * with it, so it must no longer be read
         */
        pthread_mutex_lock(&spice_client->surface_lock);
        spice_client->surface_data = NULL;
        pthread_mutex_unlock(&spice_client->surface_lock);

        /* A guest resize cannot be sent until a new primary surface exists */
        spice_client->resize_display_ready = 0;

    }
    else if ((gpointer) channel == (gpointer) spice_client->inputs_channel)
        spice_client->inputs_channel = NULL;
    else if ((gpointer) channel == (gpointer) spice_client->cursor_channel)
        spice_client->cursor_channel = NULL;
    else if ((gpointer) channel == (gpointer) spice_client->record_channel)
        spice_client->record_channel = NULL;

}
