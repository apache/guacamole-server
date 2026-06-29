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
#include "spice.h"

#include <guacamole/audio.h>
#include <guacamole/client.h>
#include <spice-client.h>

/**
 * The number of bits per audio sample. The SPICE playback channel always
 * delivers signed 16-bit PCM (SPICE_AUDIO_FMT_S16).
 */
#define GUAC_SPICE_AUDIO_BPS 16

/**
 * Signal handler for the SPICE playback channel "playback-start" signal.
 * Allocates (or reconfigures) the Guacamole audio stream to match the format
 * reported by the SPICE server.
 */
static void guac_spice_playback_start(SpicePlaybackChannel* channel,
        gint format, gint channels, gint frequency, gpointer data) {

    guac_client* client = (guac_client*) data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /* SPICE only ever delivers signed 16-bit PCM */
    if (format != SPICE_AUDIO_FMT_S16) {
        guac_client_log(client, GUAC_LOG_WARNING,
                "Unsupported SPICE audio format (%d). Audio will be dropped.",
                format);
        return;
    }

    spice_client->audio_rate = frequency;
    spice_client->audio_channels = channels;

    /* Allocate the audio stream on first use, or reconfigure it if the format
     * has changed since it was allocated */
    if (spice_client->audio == NULL)
        spice_client->audio = guac_audio_stream_alloc(client, NULL,
                frequency, channels, GUAC_SPICE_AUDIO_BPS);
    else
        guac_audio_stream_reset(spice_client->audio, NULL,
                frequency, channels, GUAC_SPICE_AUDIO_BPS);

    if (spice_client->audio == NULL)
        guac_client_log(client, GUAC_LOG_INFO,
                "No audio support detected for connected client(s). Audio "
                "will be disabled.");

}

/**
 * Signal handler for the SPICE playback channel "playback-data" signal. Writes
 * the received PCM data to the Guacamole audio stream.
 */
static void guac_spice_playback_data(SpicePlaybackChannel* channel,
        gpointer data, gint size, gpointer user_data) {

    guac_client* client = (guac_client*) user_data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    if (spice_client->audio == NULL)
        return;

    guac_audio_stream_write_pcm(spice_client->audio,
            (const unsigned char*) data, size);
    guac_audio_stream_flush(spice_client->audio);

}

/**
 * Signal handler for the SPICE playback channel "playback-stop" signal.
 * Flushes any buffered audio.
 */
static void guac_spice_playback_stop(SpicePlaybackChannel* channel,
        gpointer data) {

    guac_client* client = (guac_client*) data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    if (spice_client->audio != NULL)
        guac_audio_stream_flush(spice_client->audio);

}

void guac_spice_playback_channel_connect(guac_client* client,
        SpiceChannel* channel) {

    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /* Do nothing if audio has not been enabled */
    if (!spice_client->settings->audio_enabled)
        return;

    g_signal_connect(channel, "playback-start",
            G_CALLBACK(guac_spice_playback_start), client);
    g_signal_connect(channel, "playback-data",
            G_CALLBACK(guac_spice_playback_data), client);
    g_signal_connect(channel, "playback-stop",
            G_CALLBACK(guac_spice_playback_stop), client);

}
