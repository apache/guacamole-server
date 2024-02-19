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

#include <errno.h>
#include <time.h>

void guac_spice_client_audio_playback_data_handler(
        SpicePlaybackChannel* channel, gpointer data, gint size,
        guac_client* client) {
    
    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    guac_audio_stream_write_pcm(spice_client->audio_playback, data, size);
    
}

void guac_spice_client_audio_playback_delay_handler(
        SpicePlaybackChannel* channel, guac_client* client) {
    
    guac_client_log(client, GUAC_LOG_WARNING,
            "Delay handler for audio playback is not currently implemented.");
    
}

void guac_spice_client_audio_playback_start_handler(
        SpicePlaybackChannel* channel, gint format, gint channels, gint rate,
        guac_client* client) {
    
    guac_client_log(client, GUAC_LOG_DEBUG, "Starting audio playback.");
    guac_client_log(client, GUAC_LOG_DEBUG, "Format: %d", format);
    guac_client_log(client, GUAC_LOG_DEBUG, "Channels: %d", channels);
    guac_client_log(client, GUAC_LOG_DEBUG, "Rate: %d", rate);

    /* Spice only supports a single audio format. */
    if (format != SPICE_AUDIO_FMT_S16) {
        guac_client_log(client, GUAC_LOG_WARNING, "Unknown Spice audio format: %d", format);
        return;
    }

    /* Allocate the stream. */
    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    spice_client->audio_playback = guac_audio_stream_alloc(client, NULL, rate, channels, 16);
    
}

void guac_spice_client_audio_playback_stop_handler(
        SpicePlaybackChannel* channel, guac_client* client) {
    
    guac_client_log(client, GUAC_LOG_DEBUG, "Stoppig audio playback..");

    /* Free the audio stream. */
    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    guac_audio_stream_free(spice_client->audio_playback);
    
}

/**
 * Parses the given raw audio mimetype, producing the corresponding rate,
 * number of channels, and bytes per sample.
 *
 * @param mimetype
 *     The raw audio mimetype to parse.
 *
 * @param rate
 *     A pointer to an int where the sample rate for the PCM format described
 *     by the given mimetype should be stored.
 *
 * @param channels
 *     A pointer to an int where the number of channels used by the PCM format
 *     described by the given mimetype should be stored.
 *
 * @param bps
 *     A pointer to an int where the number of bytes used the PCM format for
 *     each sample (independent of number of channels) described by the given
 *     mimetype should be stored.
 *
 * @return
 *     Zero if the given mimetype is a raw audio mimetype and has been parsed
 *     successfully, non-zero otherwise.
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

            /* Fail if value invalid / out of range */
            if (errno == EINVAL || errno == ERANGE)
                return 1;

        }

        /* Parse number of rate */
        else if (strncmp(mimetype, "rate=", 5) == 0) {

            mimetype += 5;
            parsed_rate = strtol(mimetype, (char**) &mimetype, 10);

            /* Fail if value invalid / out of range */
            if (errno == EINVAL || errno == ERANGE)
                return 1;

        }

        /* Advance to next parameter */
        mimetype = strchr(mimetype, ',');

    } while (mimetype != NULL);

    /* Mimetype is invalid if rate was not specified */
    if (parsed_rate == -1)
        return 1;

    /* Parse success */
    *rate = parsed_rate;
    *channels = parsed_channels;
    *bps = parsed_bps;

    return 0;

}

/**
 * A callback function that is invoked to send audio data from the given
 * stream to the Spice server.
 *
 * @param user
 *     The user who owns the connection and the stream. This is unused by
 *     this function.
 *
 * @param stream
 *     The stream where the audio data originated. This is unused by this
 *     function.
 * 
 * @param data
 *     The audio data to send.
 * 
 * @param length
 *     The number of bytes of audio data to send.
 *
 * @return
 *     Zero on success, non-zero on error.
 */
static int guac_spice_audio_blob_handler(guac_user* user, guac_stream* stream,
        void* data, int length) {

    guac_client* client = user->client;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /* Write blob to audio stream */
    spice_record_channel_send_data(spice_client->record_channel, data, length, (unsigned long) time(NULL));

    return 0;

}

/**
 * A callback function that is called when the audio stream ends sending data
 * to the Spice server.
 *
 * @param user
 *     The user who owns the connection and the stream.
 *
 * @param stream
 *     The stream that was sending the audio data.
 *
 * @return
 *     Zero on success, non-zero on failure.
 */
static int guac_spice_audio_end_handler(guac_user* user, guac_stream* stream) {

    /* Ignore - the RECORD_CHANNEL channel will simply not receive anything */
    return 0;

}


int guac_spice_client_audio_record_handler(guac_user* user, guac_stream* stream,
        char* mimetype) {

    guac_user_log(user, GUAC_LOG_DEBUG, "Calling audio input handler.");

    guac_client* client = user->client;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    spice_client->audio_input = stream;

    int rate;
    int channels;
    int bps;

    /* Parse mimetype, abort on parse error */
    if (guac_spice_audio_parse_mimetype(mimetype, &rate, &channels, &bps)) {
        guac_user_log(user, GUAC_LOG_WARNING, "Denying user audio stream with "
                "unsupported mimetype: \"%s\"", mimetype);
        guac_protocol_send_ack(user->socket, stream, "Unsupported audio "
                "mimetype", GUAC_PROTOCOL_STATUS_CLIENT_BAD_TYPE);
        return 0;
    }

    /* Initialize stream handlers */
    stream->blob_handler = guac_spice_audio_blob_handler;
    stream->end_handler = guac_spice_audio_end_handler;

    return 0;


}

/**
 * Sends an "ack" instruction over the socket associated with the Guacamole
 * stream over which audio data is being received. The "ack" instruction will
 * only be sent if the Guacamole audio stream has been established (through
 * receipt of an "audio" instruction), is still open (has not received an "end"
 * instruction nor been associated with an "ack" having an error code), and is
 * associated with an active Spice RECORD_CHANNEL channel.
 *
 * @param user
 *     The guac_user associated with the audio input stream.
 *
 * @param stream
 *     The guac_stream associated with the audio input for the client.
 *
 * @param message
 *     An arbitrary human-readable message to send along with the "ack".
 *
 * @param status
 *     The Guacamole protocol status code to send with the "ack". This should
 *     be GUAC_PROTOCOL_STATUS_SUCCESS if the audio stream has been set up
 *     successfully or GUAC_PROTOCOL_STATUS_RESOURCE_CLOSED if the audio stream
 *     has been closed (but may usable again if reopened).
 */
static void guac_spice_audio_stream_ack(guac_user* user, guac_stream* stream,
        const char* message, guac_protocol_status status) {

    /* Do not send if the connection owner or stream is null. */
    if (user == NULL || stream == NULL)
        return;

    /* Send ack instruction */
    guac_protocol_send_ack(user->socket, stream, message, status);
    guac_socket_flush(user->socket);

}

/**
 * A callback that is invoked for the connection owner when audio recording
 * starts, which will notify the client the owner is connected from to start
 * sending audio data.
 *
 * @param owner
 *     The owner of the connection.
 *
 * @param data
 *     A pointer to the guac_client associated with this connection.
 *
 * @return
 *     Always NULL;
 */
static void* spice_client_record_start_callback(guac_user* owner, void* data) {
    
    guac_spice_client* spice_client = (guac_spice_client*) data;

    guac_spice_audio_stream_ack(owner, spice_client->audio_input, "OK",
            GUAC_PROTOCOL_STATUS_SUCCESS);

    return NULL;

}

/**
 * A callback that is invoked for the connection owner when audio recording
 * is stopped, telling the client to stop sending audio data.
 * 
 * @param owner
 *     The user who owns this connection.
 *
 * @param data
 *     A pointer to the guac_client associated with this connection.
 *
 * @return
 *     Always NULL;
 */
static void* spice_client_record_stop_callback(guac_user* owner, void* data) {

    guac_spice_client* spice_client = (guac_spice_client*) data;

    /* The stream is now closed */
    guac_spice_audio_stream_ack(owner, spice_client->audio_input, "CLOSED",
            GUAC_PROTOCOL_STATUS_RESOURCE_CLOSED);

    return NULL;

}

void guac_spice_client_audio_record_start_handler(SpiceRecordChannel* channel,
        gint format, gint channels, gint rate, guac_client* client) {

    guac_client_log(client, GUAC_LOG_DEBUG, "Calling audio record start handler.");

    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    guac_client_for_owner(client, spice_client_record_start_callback, spice_client);
    
}

void guac_spice_client_audio_record_stop_handler(SpiceRecordChannel* channel,
        guac_client* client) {
    
    guac_client_log(client, GUAC_LOG_DEBUG, "Calling audio record stop handler.");

    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    guac_client_for_owner(client, spice_client_record_stop_callback, spice_client);

}