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
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>
#include <guacamole/user.h>
#include <spice-client.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

    /* Drop audio entirely while playback is muted */
    if (spice_client->audio == NULL || spice_client->audio_muted)
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

/**
 * Handler for changes to the SPICE playback channel's "mute" property. Records
 * the current mute state so that playback data can be dropped while muted.
 *
 * Note that the channel's "volume" property is intentionally not tracked: the
 * guest's PCM stream is already scaled to the reported volume, so re-applying
 * it here would double-attenuate the audio.
 */
static void guac_spice_playback_mute_changed(SpicePlaybackChannel* channel,
        GParamSpec* pspec, gpointer data) {

    guac_client* client = (guac_client*) data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    gboolean muted = FALSE;
    g_object_get(channel, "mute", &muted, NULL);
    spice_client->audio_muted = muted;

    guac_client_log(client, GUAC_LOG_DEBUG, "SPICE audio playback is now %s.",
            muted ? "muted" : "unmuted");

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

    /* Track mute state reported by the SPICE server */
    g_signal_connect(channel, "notify::mute",
            G_CALLBACK(guac_spice_playback_mute_changed), client);

}

/**
 * Parses the given raw audio mimetype, producing the corresponding rate,
 * number of channels, and bytes per sample. Only "audio/L16" (signed 16-bit
 * PCM) is supported, matching the format expected by the SPICE record channel.
 *
 * @return
 *     Zero if the mimetype was successfully parsed, non-zero otherwise.
 */
static int guac_spice_audio_parse_mimetype(const char* mimetype, int* rate,
        int* channels, int* bps) {

    int parsed_rate = -1;
    int parsed_channels = 1;
    int parsed_bps;

    /* PCM audio with two bytes per sample */
    if (strncmp(mimetype, "audio/L16;", 10) == 0) {
        mimetype += 9; /* Advance to semicolon ONLY */
        parsed_bps = 2;
    }

    /* Unsupported mimetype */
    else
        return 1;

    /* Parse each parameter name/value pair within the mimetype */
    do {

        /* Advance to first character of parameter (current is either a
         * semicolon or a comma) */
        mimetype++;

        /* Parse number of channels */
        if (strncmp(mimetype, "channels=", 9) == 0) {
            mimetype += 9;
            parsed_channels = strtol(mimetype, (char**) &mimetype, 10);
            if (errno == EINVAL || errno == ERANGE)
                return 1;
        }

        /* Parse sample rate */
        else if (strncmp(mimetype, "rate=", 5) == 0) {
            mimetype += 5;
            parsed_rate = strtol(mimetype, (char**) &mimetype, 10);
            if (errno == EINVAL || errno == ERANGE)
                return 1;
        }

        /* Advance to next parameter */
        mimetype = strchr(mimetype, ',');

    } while (mimetype != NULL);

    /* Mimetype is invalid if rate was not specified */
    if (parsed_rate == -1)
        return 1;

    *rate = parsed_rate;
    *channels = parsed_channels;
    *bps = parsed_bps;
    return 0;

}

/**
 * Handler for "blob" instructions received on the audio input stream. Forwards
 * the received PCM audio data to the SPICE record channel.
 */
static int guac_spice_audio_blob_handler(guac_user* user, guac_stream* stream,
        void* data, int length) {

    guac_spice_client* spice_client = (guac_spice_client*) user->client->data;

    if (spice_client->record_channel != NULL)
        spice_record_channel_send_data(spice_client->record_channel,
                data, length, (unsigned long) time(NULL));

    return 0;

}

/**
 * Handler for the "end" instruction on the audio input stream.
 */
static int guac_spice_audio_end_handler(guac_user* user, guac_stream* stream) {
    /* Nothing to do - the record channel simply stops receiving data */
    return 0;
}

int guac_spice_client_audio_record_handler(guac_user* user, guac_stream* stream,
        char* mimetype) {

    guac_spice_client* spice_client = (guac_spice_client*) user->client->data;
    spice_client->audio_input = stream;

    int rate, channels, bps;

    /* Deny the stream if the offered audio format cannot be handled */
    if (guac_spice_audio_parse_mimetype(mimetype, &rate, &channels, &bps)) {
        guac_user_log(user, GUAC_LOG_WARNING, "Denying user audio stream with "
                "unsupported mimetype: \"%s\"", mimetype);
        guac_protocol_send_ack(user->socket, stream,
                "Unsupported audio mimetype",
                GUAC_PROTOCOL_STATUS_CLIENT_BAD_TYPE);
        return 0;
    }

    /* Set up handlers for the audio input stream */
    stream->blob_handler = guac_spice_audio_blob_handler;
    stream->end_handler = guac_spice_audio_end_handler;

    return 0;

}

/**
 * Sends an "ack" for the audio input stream to the connection owner, if the
 * owner and stream are both present.
 */
static void guac_spice_audio_stream_ack(guac_user* user, guac_stream* stream,
        const char* message, guac_protocol_status status) {

    if (user == NULL || stream == NULL)
        return;

    guac_protocol_send_ack(user->socket, stream, message, status);
    guac_socket_flush(user->socket);

}

/**
 * Owner callback invoked when the SPICE server begins recording, acknowledging
 * the owner's audio input stream so the client starts sending audio.
 */
static void* spice_client_record_start_callback(guac_user* owner, void* data) {
    guac_spice_client* spice_client = (guac_spice_client*) data;
    guac_spice_audio_stream_ack(owner, spice_client->audio_input, "OK",
            GUAC_PROTOCOL_STATUS_SUCCESS);
    return NULL;
}

/**
 * Owner callback invoked when the SPICE server stops recording, closing the
 * owner's audio input stream so the client stops sending audio.
 */
static void* spice_client_record_stop_callback(guac_user* owner, void* data) {
    guac_spice_client* spice_client = (guac_spice_client*) data;
    guac_spice_audio_stream_ack(owner, spice_client->audio_input, "CLOSED",
            GUAC_PROTOCOL_STATUS_RESOURCE_CLOSED);
    return NULL;
}

void guac_spice_client_audio_record_start_handler(SpiceRecordChannel* channel,
        gint format, gint channels, gint rate, guac_client* client) {
    guac_client_log(client, GUAC_LOG_DEBUG, "SPICE audio recording started.");
    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    guac_client_for_owner(client, spice_client_record_start_callback, spice_client);
}

void guac_spice_client_audio_record_stop_handler(SpiceRecordChannel* channel,
        guac_client* client) {
    guac_client_log(client, GUAC_LOG_DEBUG, "SPICE audio recording stopped.");
    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    guac_client_for_owner(client, spice_client_record_stop_callback, spice_client);
}

void guac_spice_record_channel_connect(guac_client* client,
        SpiceChannel* channel) {

    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /* Do nothing if audio input has not been enabled */
    if (!spice_client->settings->audio_input_enabled)
        return;

    spice_client->record_channel = SPICE_RECORD_CHANNEL(channel);

    g_signal_connect(channel, "record-start",
            G_CALLBACK(guac_spice_client_audio_record_start_handler), client);
    g_signal_connect(channel, "record-stop",
            G_CALLBACK(guac_spice_client_audio_record_stop_handler), client);

}
