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

#include <guacamole/client.h>
#include <guacamole/stream.h>
#include <guacamole/user.h>
#include <spice-client.h>

/**
 * Connects the necessary signal handlers to the given SPICE playback channel
 * so that audio received from the SPICE server is streamed to connected
 * Guacamole users.
 *
 * @param client
 *     The guac_client associated with the SPICE connection.
 *
 * @param channel
 *     The SPICE playback channel to handle.
 */
void guac_spice_playback_channel_connect(guac_client* client,
        SpiceChannel* channel);

/**
 * Connects the necessary signal handlers to the given SPICE record channel so
 * that audio received from the connected Guacamole user (e.g. a microphone) is
 * forwarded to the SPICE server. Does nothing if audio input has not been
 * enabled for the connection.
 *
 * @param client
 *     The guac_client associated with the SPICE connection.
 *
 * @param channel
 *     The SPICE record channel to handle.
 */
void guac_spice_record_channel_connect(guac_client* client,
        SpiceChannel* channel);

/**
 * Handler for inbound audio ("audio" instruction) from a Guacamole user,
 * establishing an audio input stream whose data will be forwarded to the SPICE
 * record channel.
 */
int guac_spice_client_audio_record_handler(guac_user* user,
        guac_stream* stream, char* mimetype);

/**
 * Signal handler invoked when the SPICE server begins audio recording.
 */
void guac_spice_client_audio_record_start_handler(SpiceRecordChannel* channel,
        gint format, gint channels, gint rate, guac_client* client);

/**
 * Signal handler invoked when the SPICE server stops audio recording.
 */
void guac_spice_client_audio_record_stop_handler(SpiceRecordChannel* channel,
        guac_client* client);

#endif
