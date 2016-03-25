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


#ifndef __GUAC_VNC_PULSE_H
#define __GUAC_VNC_PULSE_H

#include "config.h"

#include <guacamole/client.h>

/**
 * The number of bytes to request for the audio fragments received from
 * PulseAudio.
 */
#define GUAC_VNC_AUDIO_FRAGMENT_SIZE 8192

/**
 * The minimum number of PCM bytes to wait for before flushing an audio
 * packet. The current value is 48K, which works out to be around 280ms.
 */
#define GUAC_VNC_PCM_WRITE_RATE 49152

/**
 * Rate of audio to stream, in Hz.
 */
#define GUAC_VNC_AUDIO_RATE     44100

/**
 * The number of channels to stream.
 */
#define GUAC_VNC_AUDIO_CHANNELS 2

/**
 * The number of bits per sample.
 */
#define GUAC_VNC_AUDIO_BPS      16

/**
 * Starts streaming audio from PulseAudio to the given Guacamole client.
 *
 * @param client The client to stream data to.
 */
void guac_pa_start_stream(guac_client* client);

/**
 * Stops streaming audio from PulseAudio to the given Guacamole client.
 *
 * @param client The client to stream data to.
 */
void guac_pa_stop_stream(guac_client* client);

#endif

