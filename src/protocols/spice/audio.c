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
#include <guacamole/mem.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>
#include <guacamole/user.h>
#include <spice-client.h>

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/**
 * The number of bits per audio sample. The SPICE playback channel always
 * delivers signed 16-bit PCM (SPICE_AUDIO_FMT_S16).
 */
#define GUAC_SPICE_AUDIO_BPS 16

/**
 * Bounds on the client-advertised audio capture format and on the resampler's
 * output size. These values are attacker-controlled (parsed from the audio
 * stream mimetype) and feed allocation-size arithmetic, so they are bounded to
 * prevent integer overflow and oversized allocations (CWE-190/CWE-400).
 */
#define GUAC_SPICE_AUDIO_MIN_RATE     8000
#define GUAC_SPICE_AUDIO_MAX_RATE     192000
#define GUAC_SPICE_AUDIO_MAX_CHANNELS 8

/**
 * The maximum number of output frames a single resample operation may produce.
 * A normal input blob is a small PCM chunk, so this bound is far above any
 * legitimate value while capping worst-case allocation to a few tens of MB.
 */
#define GUAC_SPICE_AUDIO_MAX_FRAMES   (GUAC_SPICE_AUDIO_MAX_RATE * 30)

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

    /* Reject implausible sample rates / channel counts. These values are
     * attacker-controlled (from the client's audio stream mimetype) and feed
     * directly into resampler allocation size arithmetic; bounding them here
     * prevents integer overflow / oversized allocations downstream (CWE-190). */
    if (parsed_rate < GUAC_SPICE_AUDIO_MIN_RATE
            || parsed_rate > GUAC_SPICE_AUDIO_MAX_RATE)
        return 1;
    if (parsed_channels < 1 || parsed_channels > GUAC_SPICE_AUDIO_MAX_CHANNELS)
        return 1;

    *rate = parsed_rate;
    *channels = parsed_channels;
    *bps = parsed_bps;
    return 0;

}

/**
 * Converts signed 16-bit interleaved PCM from one sample rate and channel count
 * to another using linear interpolation (for rate) and simple up/down mixing
 * (mono<->stereo, or passthrough for matching counts). Returns a newly
 * allocated buffer, which the caller must free with guac_mem_free(), storing
 * its length in bytes in *out_length, or NULL on failure.
 */
static int16_t* guac_spice_resample_s16(const int16_t* in, int in_bytes,
        int in_rate, int in_channels, int out_rate, int out_channels,
        int* out_length) {

    *out_length = 0;

    if (in_rate <= 0 || out_rate <= 0 || in_channels <= 0 || out_channels <= 0)
        return NULL;

    int in_frames = in_bytes / (2 * in_channels);
    if (in_frames <= 0)
        return NULL;

    long out_frames = (long) in_frames * out_rate / in_rate;
    if (out_frames <= 0 || out_frames > GUAC_SPICE_AUDIO_MAX_FRAMES)
        return NULL;

    /* Use the checked-multiply allocation form so the output size cannot
     * overflow (out_frames and out_channels are both bounded above) */
    int16_t* out = guac_mem_alloc(out_frames, out_channels, sizeof(int16_t));
    if (out == NULL)
        return NULL;

    for (long o = 0; o < out_frames; o++) {

        /* Fractional source frame position for linear interpolation */
        double src = (double) o * in_rate / out_rate;
        long i0 = (long) src;
        long i1 = (i0 + 1 < in_frames) ? i0 + 1 : i0;
        double frac = src - i0;

        /* Interpolate the source channels into a working left/right pair */
        int16_t l0 = in[i0 * in_channels];
        int16_t l1 = in[i1 * in_channels];
        double left = l0 + (l1 - l0) * frac;

        double right;
        if (in_channels >= 2) {
            int16_t r0 = in[i0 * in_channels + 1];
            int16_t r1 = in[i1 * in_channels + 1];
            right = r0 + (r1 - r0) * frac;
        }
        else
            right = left;

        /* Emit the requested number of output channels */
        int16_t* frame = &out[o * out_channels];
        if (out_channels == 1)
            frame[0] = (int16_t) ((left + right) / 2.0);
        else {
            frame[0] = (int16_t) left;
            frame[1] = (int16_t) right;
            for (int c = 2; c < out_channels; c++)
                frame[c] = (int16_t) left;
        }

    }

    *out_length = (int) (out_frames * out_channels * sizeof(int16_t));
    return out;

}

/**
 * Handler for "blob" instructions received on the audio input stream. Forwards
 * the received PCM audio data to the SPICE record channel, converting it to the
 * rate/channels the server expects if the connected user is capturing in a
 * different format.
 */
static int guac_spice_audio_blob_handler(guac_user* user, guac_stream* stream,
        void* data, int length) {

    guac_spice_client* spice_client = (guac_spice_client*) user->client->data;

    if (spice_client->record_channel == NULL)
        return 0;

    int rr = spice_client->record_rate;
    int rc = spice_client->record_channels;
    int ir = spice_client->input_rate;
    int ic = spice_client->input_channels;

    /* Forward directly if the server's format is not yet known or already
     * matches the inbound stream */
    if (rr <= 0 || rc <= 0 || (rr == ir && rc == ic)) {
        spice_record_channel_send_data(spice_client->record_channel,
                data, length, (unsigned long) time(NULL));
        return 0;
    }

    /* Otherwise convert to the format the SPICE record channel expects */
    int out_length = 0;
    int16_t* resampled = guac_spice_resample_s16((const int16_t*) data, length,
            ir, ic, rr, rc, &out_length);

    if (resampled != NULL) {
        spice_record_channel_send_data(spice_client->record_channel,
                resampled, out_length, (unsigned long) time(NULL));
        guac_mem_free(resampled);
    }

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

    /* Remember the format the user is sending so it can be converted to the
     * rate/channels the SPICE record channel expects, if they differ */
    spice_client->input_rate = rate;
    spice_client->input_channels = channels;

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
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /* Record the format the SPICE server expects so inbound audio can be
     * converted to match if the connected user is capturing at a different
     * rate or channel count */
    spice_client->record_rate = rate;
    spice_client->record_channels = channels;

    guac_client_log(client, GUAC_LOG_DEBUG, "SPICE audio recording started "
            "(%d Hz, %d channel(s)).", rate, channels);
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
