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

#ifndef GUAC_RDP_CLIENT_H
#define GUAC_RDP_CLIENT_H

#include "config.h"

#include <guacamole/client.h>

/**
 * The maximum duration of a frame in milliseconds.
 */
#define GUAC_RDP_FRAME_DURATION 60

/**
 * The amount of time to allow per message read within a frame, in
 * milliseconds. If the server is silent for at least this amount of time, the
 * frame will be considered finished.
 */
#define GUAC_RDP_FRAME_TIMEOUT 10

/**
 * The native resolution of most RDP connections. As Windows and other systems
 * rely heavily on forced 96 DPI, we must assume 96 DPI.
 */
#define GUAC_RDP_NATIVE_RESOLUTION 96

/**
 * The resolution of an RDP connection that would be considered high, but is
 * tolerable in the case that the client display would be unreasonably small
 * otherwise.
 */
#define GUAC_RDP_HIGH_RESOLUTION 120

/**
 * The smallest area, in pixels^2, that would be considered reasonable large
 * screen DPI needs to be adjusted.
 */
#define GUAC_RDP_REASONABLE_AREA (800*600)

/**
 * The maximum number of bytes to allow within the clipboard.
 */
#define GUAC_RDP_CLIPBOARD_MAX_LENGTH 262144

/**
 * Initial rate of audio to stream, in Hz. If the RDP server uses a different
 * value, the Guacamole audio stream will simply be reset appropriately.
 */
#define GUAC_RDP_AUDIO_RATE 44100

/**
 * The number of channels to stream for audio. If the RDP server uses a
 * different value, the Guacamole audio stream will simply be reset
 * appropriately.
 */
#define GUAC_RDP_AUDIO_CHANNELS 2

/**
 * The number of bits per sample within the audio stream. If the RDP server
 * uses a different value, the Guacamole audio stream will simply be reset
 * appropriately.
 */
#define GUAC_RDP_AUDIO_BPS 16

/**
 * Handler which frees all data associated with the guac_client.
 *
 * @param client The guac_client whose data should be freed.
 * @return Zero if the client was successfully freed, non-zero otherwise.
 */
int guac_rdp_client_free_handler(guac_client* client);

#endif
