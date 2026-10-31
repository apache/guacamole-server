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
 * The minimum audio sample rate, in Hz, accepted for audio input, whether
 * advertised by the client (in its audio stream mimetype) or by the SPICE
 * server (in the record channel's "record-start" signal). Sample rates feed
 * resampler allocation-size arithmetic, so they are bounded to prevent integer
 * overflow and oversized allocations.
 */
#define GUAC_SPICE_AUDIO_MIN_RATE 8000

/**
 * The maximum audio sample rate, in Hz, accepted for audio input, whether
 * advertised by the client or by the SPICE server. See
 * GUAC_SPICE_AUDIO_MIN_RATE.
 */
#define GUAC_SPICE_AUDIO_MAX_RATE 192000

/**
 * The maximum number of audio channels accepted for audio input, whether
 * advertised by the client or by the SPICE server. Like the sample rate, the
 * channel count feeds resampler allocation-size arithmetic and is bounded for
 * the same reason.
 */
#define GUAC_SPICE_AUDIO_MAX_CHANNELS 8

/**
 * The maximum number of output frames a single resample operation may produce,
 * equal to 30 seconds of audio at GUAC_SPICE_AUDIO_MAX_RATE. The value 30 is
 * not derived from any protocol constant; it is a deliberately generous
 * ceiling. Each audio input blob is carried by a single Guacamole instruction
 * (at most GUAC_INSTRUCTION_MAX_LENGTH bytes, so roughly 6 KB of decoded PCM),
 * which at the most extreme legitimate ratio (8 kHz mono upsampled to 192 kHz)
 * yields under half a second of output. The bound is therefore never reached
 * by valid input and exists purely as a defensive cap on the allocation in
 * guac_spice_resample_s16(), limiting it to at most about 92 MB (30 seconds at
 * 192 kHz, 8 channels, 16-bit samples).
 */
#define GUAC_SPICE_AUDIO_MAX_FRAMES (GUAC_SPICE_AUDIO_MAX_RATE * 30)

/**
 * Signal handler for the SPICE playback channel "playback-start" signal.
 * Allocates (or reconfigures) the Guacamole audio stream to match the format
 * reported by the SPICE server. Audio in any format other than signed 16-bit
 * PCM is dropped.
 *
 * @param channel
 *     The SPICE playback channel that emitted the signal. Unused.
 *
 * @param format
 *     The SPICE audio format of the playback stream (SPICE_AUDIO_FMT_*). Only
 *     SPICE_AUDIO_FMT_S16 is supported.
 *
 * @param channels
 *     The number of audio channels in the playback stream.
 *
 * @param frequency
 *     The sample rate of the playback stream, in Hz.
 *
 * @param data
 *     The guac_client associated with the SPICE connection, supplied as the
 *     user data given to g_signal_connect().
 */
static void guac_spice_playback_start(SpicePlaybackChannel* channel,
        gint format, gint channels, gint frequency, gpointer data) {

    (void) channel;

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

    /*
     * Allocate the audio stream on first use, or reconfigure it if the format
     * has changed since it was allocated
     */
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
 * the received PCM data to the Guacamole audio stream. The data is dropped if
 * no audio stream has been allocated or if playback is currently muted.
 *
 * @param channel
 *     The SPICE playback channel that emitted the signal. Unused.
 *
 * @param data
 *     A buffer containing the received signed 16-bit PCM audio. The buffer is
 *     owned by spice-gtk and is valid only for the duration of the call.
 *
 * @param size
 *     The size of the audio data buffer, in bytes.
 *
 * @param user_data
 *     The guac_client associated with the SPICE connection, supplied as the
 *     user data given to g_signal_connect().
 */
static void guac_spice_playback_data(SpicePlaybackChannel* channel,
        gpointer data, gint size, gpointer user_data) {

    (void) channel;

    guac_client* client = (guac_client*) user_data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /*
     * Drop audio if no audio stream has been allocated (playback has not
     * started), or while playback is muted
     */
    if (spice_client->audio == NULL || spice_client->audio_muted)
        return;

    guac_audio_stream_write_pcm(spice_client->audio,
            (const unsigned char*) data, size);
    guac_audio_stream_flush(spice_client->audio);

}

/**
 * Signal handler for the SPICE playback channel "playback-stop" signal.
 * Flushes any buffered audio.
 *
 * @param channel
 *     The SPICE playback channel that emitted the signal. Unused.
 *
 * @param data
 *     The guac_client associated with the SPICE connection, supplied as the
 *     user data given to g_signal_connect().
 */
static void guac_spice_playback_stop(SpicePlaybackChannel* channel,
        gpointer data) {

    (void) channel;

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
 *
 * @param channel
 *     The SPICE playback channel whose "mute" property changed.
 *
 * @param pspec
 *     The GParamSpec describing the property that changed. Unused.
 *
 * @param data
 *     The guac_client associated with the SPICE connection, supplied as the
 *     user data given to g_signal_connect().
 */
static void guac_spice_playback_mute_changed(SpicePlaybackChannel* channel,
        GParamSpec* pspec, gpointer data) {

    (void) pspec;

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
 * @param mimetype
 *     The mimetype of the audio stream, as received from the client (for
 *     example, "audio/L16;rate=44100,channels=2"). Must not be NULL.
 *
 * @param rate
 *     Pointer to an int in which the sample rate, in Hz, will be stored. Only
 *     modified if the mimetype is successfully parsed.
 *
 * @param channels
 *     Pointer to an int in which the number of channels will be stored. Only
 *     modified if the mimetype is successfully parsed.
 *
 * @param bps
 *     Pointer to an int in which the number of bytes per sample will be
 *     stored. Only modified if the mimetype is successfully parsed.
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

        /*
         * Advance to first character of parameter (current is either a
         * semicolon or a comma)
         */
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

    /*
     * Reject implausible sample rates / channel counts. These values are
     * supplied by the client (from its audio stream mimetype) and feed
     * directly into resampler allocation size arithmetic; bounding them here
     * prevents integer overflow / oversized allocations downstream.
     */
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
 * (mono<->stereo, or passthrough for matching counts).
 *
 * @param in
 *     The interleaved signed 16-bit PCM samples to convert.
 *
 * @param in_bytes
 *     The size of the input buffer, in bytes.
 *
 * @param in_rate
 *     The sample rate of the input, in Hz.
 *
 * @param in_channels
 *     The number of interleaved channels in the input.
 *
 * @param out_rate
 *     The desired sample rate of the output, in Hz.
 *
 * @param out_channels
 *     The desired number of interleaved channels in the output.
 *
 * @param out_length
 *     Pointer to a size_t in which the length of the returned buffer, in
 *     bytes, will be stored. This is set to zero if conversion fails.
 *
 * @return
 *     A newly allocated buffer containing the converted samples, which must be
 *     freed by the caller with guac_mem_free(), or NULL if the parameters are
 *     invalid, the input is too short, the output would be implausibly large,
 *     or allocation fails.
 */
static int16_t* guac_spice_resample_s16(const int16_t* in, int in_bytes,
        int in_rate, int in_channels, int out_rate, int out_channels,
        size_t* out_length) {

    *out_length = 0;

    if (in_rate <= 0 || out_rate <= 0 || in_channels <= 0 || out_channels <= 0)
        return NULL;

    int in_frames = in_bytes / (2 * in_channels);
    if (in_frames <= 0)
        return NULL;

    long out_frames = (long) in_frames * out_rate / in_rate;
    if (out_frames <= 0 || out_frames > GUAC_SPICE_AUDIO_MAX_FRAMES)
        return NULL;

    /*
     * Use the checked-multiply allocation form so the output size cannot
     * overflow (out_frames and out_channels are both bounded above)
     */
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

    *out_length = (size_t) out_frames * (size_t) out_channels * sizeof(int16_t);
    return out;

}

/**
 * GDestroyNotify which frees an audio buffer allocated with guac_mem_alloc(),
 * used to release the payload of a deferred record-channel send.
 *
 * @param data
 *     The audio buffer to free.
 */
static void guac_spice_audio_free_buffer(gpointer data) {
    guac_mem_free(data);
}

/**
 * Deferred-call handler which sends audio input data to the SPICE record
 * channel on the event-loop thread. The live record channel is read on the
 * event-loop thread, and the data is dropped if the channel no longer exists.
 *
 * @param call
 *     The deferred call whose client member is the guac_client associated
 *     with the SPICE connection, whose data and data_len members are the PCM
 *     audio to send (freed with the call), and whose args[0] is the timestamp
 *     to associate with the audio.
 */
static void guac_spice_do_record_send(guac_spice_deferred_call* call) {

    guac_spice_client* spice_client = (guac_spice_client*) call->client->data;
    SpiceRecordChannel* record_channel = spice_client->record_channel;

    if (record_channel != NULL)
        spice_record_channel_send_data(record_channel, call->data,
                call->data_len, call->args[0]);

}

/**
 * Handler for "blob" instructions received on the audio input stream. Forwards
 * the received PCM audio data to the SPICE record channel, converting it to the
 * rate/channels the server expects if the connected user is capturing in a
 * different format. As spice-gtk channel functions must not be called from
 * guacd user threads, the data is handed to the SPICE event-loop thread via
 * guac_spice_defer_call().
 *
 * @param user
 *     The user that sent the audio data.
 *
 * @param stream
 *     The audio input stream along which the data was received. Unused.
 *
 * @param data
 *     A buffer containing the received signed 16-bit PCM audio.
 *
 * @param length
 *     The size of the received audio data, in bytes.
 *
 * @return
 *     Zero, always. Audio that cannot be forwarded or converted is dropped.
 */
static int guac_spice_audio_blob_handler(guac_user* user, guac_stream* stream,
        void* data, int length) {

    (void) stream;

    guac_spice_client* spice_client = (guac_spice_client*) user->client->data;

    /*
     * Cheap early-out; the deferred handler re-checks on the event-loop
     * thread
     */
    if (spice_client->record_channel == NULL || length <= 0)
        return 0;

    /*
     * The record_rate/record_channels/input_rate/input_channels fields are
     * written from the SPICE event-loop thread (record-start/record-stop
     * handlers) while this blob handler runs on a guacd user thread; copy
     * them out under message_lock to avoid torn reads and a mismatched
     * rate/channel pairing, then do the resample work outside the lock
     */
    pthread_mutex_lock(&(spice_client->message_lock));
    int rr = spice_client->record_rate;
    int rc = spice_client->record_channels;
    int ir = spice_client->input_rate;
    int ic = spice_client->input_channels;
    pthread_mutex_unlock(&(spice_client->message_lock));

    void* buffer;
    size_t buffer_length;

    /*
     * Forward directly if the server's format is not yet known or already
     * matches the inbound stream. The received data is owned by libguac and
     * valid only for the duration of this call, so it must be copied.
     */
    if (rr <= 0 || rc <= 0 || (rr == ir && rc == ic)) {
        buffer = guac_mem_alloc(length);
        if (buffer == NULL)
            return 0;
        memcpy(buffer, data, length);
        buffer_length = length;
    }

    /* Otherwise convert to the format the SPICE record channel expects */
    else {
        buffer = guac_spice_resample_s16((const int16_t*) data, length,
                ir, ic, rr, rc, &buffer_length);
        if (buffer == NULL)
            return 0;
    }

    /*
     * Hand the audio to the SPICE event-loop thread. The deferred call takes
     * ownership of the buffer, which is freed once sent, or without being
     * sent if the session shuts down first (see guac_spice_defer_call())
     */
    guac_spice_deferred_call* call = g_new0(guac_spice_deferred_call, 1);
    call->handler = guac_spice_do_record_send;
    call->client = user->client;
    call->args[0] = (unsigned int) time(NULL);
    call->data = buffer;
    call->data_destroy = guac_spice_audio_free_buffer;
    call->data_len = buffer_length;
    guac_spice_defer_call(call);

    return 0;

}

/**
 * Handler for the "end" instruction on the audio input stream.
 *
 * @param user
 *     The user that sent the "end" instruction. Unused.
 *
 * @param stream
 *     The audio input stream that was ended. Unused.
 *
 * @return
 *     Zero, always.
 */
static int guac_spice_audio_end_handler(guac_user* user, guac_stream* stream) {

    (void) user;
    (void) stream;

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

    /*
     * Remember the format the user is sending so it can be converted to the
     * rate/channels the SPICE record channel expects, if they differ
     */
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
 *
 * @param user
 *     The connection owner, or NULL if there is no owner, in which case
 *     nothing is sent.
 *
 * @param stream
 *     The audio input stream to acknowledge, or NULL if no such stream exists,
 *     in which case nothing is sent.
 *
 * @param message
 *     A human-readable message describing the status.
 *
 * @param status
 *     The status code to send within the "ack" instruction.
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
 *
 * @param owner
 *     The connection owner, or NULL if there is no owner.
 *
 * @param data
 *     The guac_spice_client associated with the SPICE connection.
 *
 * @return
 *     NULL, always.
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
 *
 * @param owner
 *     The connection owner, or NULL if there is no owner.
 *
 * @param data
 *     The guac_spice_client associated with the SPICE connection.
 *
 * @return
 *     NULL, always.
 */
static void* spice_client_record_stop_callback(guac_user* owner, void* data) {
    guac_spice_client* spice_client = (guac_spice_client*) data;
    guac_spice_audio_stream_ack(owner, spice_client->audio_input, "CLOSED",
            GUAC_PROTOCOL_STATUS_RESOURCE_CLOSED);
    return NULL;
}

void guac_spice_client_audio_record_start_handler(SpiceRecordChannel* channel,
        gint format, gint channels, gint rate, guac_client* client) {

    /*
     * The record channel is tracked in guac_spice_client, and spice-protocol
     * defines only SPICE_AUDIO_FMT_S16, which this module assumes
     */
    (void) channel;
    (void) format;

    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /*
     * Reject implausible sample rates / channel counts advertised by the
     * SPICE server. The server is not a trusted party, and these values
     * feed directly into resampler allocation size arithmetic in
     * guac_spice_resample_s16(); bounding them here prevents integer
     * overflow / oversized allocations downstream.
     * Reset record_rate/record_channels (disabling resampling) rather than
     * storing a hostile value or leaving an earlier valid format active.
     */
    if (rate < GUAC_SPICE_AUDIO_MIN_RATE || rate > GUAC_SPICE_AUDIO_MAX_RATE
            || channels < 1 || channels > GUAC_SPICE_AUDIO_MAX_CHANNELS) {
        guac_client_log(client, GUAC_LOG_WARNING, "Ignoring SPICE audio "
                "record format advertised by the server, as it is outside "
                "the supported range (%d Hz, %d channel(s)).", rate, channels);
        pthread_mutex_lock(&(spice_client->message_lock));
        spice_client->record_rate = 0;
        spice_client->record_channels = 0;
        pthread_mutex_unlock(&(spice_client->message_lock));
        return;
    }

    /*
     * Record the format the SPICE server expects so inbound audio can be
     * converted to match if the connected user is capturing at a different
     * rate or channel count. This is written under message_lock as the blob
     * handler reads these fields from a different (guacd user) thread.
     */
    pthread_mutex_lock(&(spice_client->message_lock));
    spice_client->record_rate = rate;
    spice_client->record_channels = channels;
    pthread_mutex_unlock(&(spice_client->message_lock));

    guac_client_log(client, GUAC_LOG_DEBUG, "SPICE audio recording started "
            "(%d Hz, %d channel(s)).", rate, channels);
    guac_client_for_owner(client, spice_client_record_start_callback, spice_client);
}

void guac_spice_client_audio_record_stop_handler(SpiceRecordChannel* channel,
        guac_client* client) {

    /* The record channel is tracked in guac_spice_client */
    (void) channel;

    guac_client_log(client, GUAC_LOG_DEBUG, "SPICE audio recording stopped.");
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /*
     * Reset the recorded format so a stale format from a previous recording
     * session cannot remain active (and be read by the blob handler, which
     * synchronizes on the same lock) once recording has stopped
     */
    pthread_mutex_lock(&(spice_client->message_lock));
    spice_client->record_rate = 0;
    spice_client->record_channels = 0;
    pthread_mutex_unlock(&(spice_client->message_lock));

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
