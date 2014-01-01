/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#ifndef __GUAC_VNC_PULSE_H
#define __GUAC_VNC_PULSE_H

#include "config.h"

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

