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

#ifndef GUAC_SPICE_AUDIO_H
#define GUAC_SPICE_AUDIO_H

#include "config.h"

#include <guacamole/client.h>

#include <spice-client-glib-2.0/spice-client.h>

/**
 * A callback function invoked with the SPICE client receives audio playback
 * data from the Spice server.
 * 
 * @param channel
 *     The SpicePlaybackChannel on which the data was sent.
 * 
 * @param data
 *     The audio data.
 * 
 * @param size
 *     The number of bytes of data sent.
 * 
 * @param client
 *     The guac_client associated with this event.
 */
void guac_spice_client_audio_playback_data_handler(
        SpicePlaybackChannel* channel, gpointer data, gint size,
        guac_client* client);

/**
 * A callback function invoked when the Spice server requests the audio playback
 * delay value from the client.
 * 
 * @param channel
 *     The SpicePlaybackChannel channel on which this event occurred.
 * 
 * @param client
 *     The guac_client associated with this event.
 */
void guac_spice_client_audio_playback_delay_handler(
        SpicePlaybackChannel* channel, guac_client* client);

/**
 * A callback function invoked by the client when the Spice server sends a 
 * signal indicating that it is starting an audio transmission.
 * 
 * @param channel
 *     The SpicePlaybackChannel on which this event occurred.
 * 
 * @param format
 *     The SPICE_AUDIO_FMT value of the format of the audio data.
 * 
 * @param channels
 *     The number of channels of audio data present.
 * 
 * @param rate
 *     The audio sampling rate at which the data will be sent.
 * 
 * @param client
 *     The guac_client associated with this event.
 */
void guac_spice_client_audio_playback_start_handler(
        SpicePlaybackChannel* channel, gint format, gint channels, gint rate,
        guac_client* client);

/**
 * The callback function invoked by the client when the SPICE server sends
 * an event indicating that audio playback will cease.
 * 
 * @param channel
 *     The SpicePlaybackChannel on which the recording is being halted.
 * 
 * @param client
 *     The guac_client associated with this event.
 */
void guac_spice_client_audio_playback_stop_handler(
        SpicePlaybackChannel* channel, guac_client* client);

/**
 * Handler for inbound audio data (audio input).
 */
guac_user_audio_handler guac_spice_client_audio_record_handler;

/**
 * The callback function invoked by the Spice client when the Spice server
 * requests that the client begin recording audio data to send to the server.
 * 
 * @param channel
 *     The SpiceRecordChannel on which the record event is being requested.
 * 
 * @param format
 *     The SPICE_AUDIO_FMT value representing the required recording format.
 * 
 * @param channels
 *     The number of audio channels that should be recorded.
 * 
 * @param rate
 *     The rate at which the recording should take place.
 * 
 * @param client
 *     The guac_client associated with this event.
 */
void guac_spice_client_audio_record_start_handler(SpiceRecordChannel* channel,
        gint format, gint channels, gint rate, guac_client* client);

/**
 * The callback function invoked by the Spice client when the Spice server sends
 * an event requesting that recording be stopped.
 * 
 * @param channel
 *     The channel associated with the event.
 * 
 * @param client
 *     The guac_client associated with the event.
 */
void guac_spice_client_audio_record_stop_handler(SpiceRecordChannel* channel,
        guac_client* client);
#endif /* GUAC_SPICE_AUDIO_H */

