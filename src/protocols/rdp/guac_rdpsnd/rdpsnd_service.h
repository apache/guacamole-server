/*
 * Copyright (C) 2015 Glyptodon LLC
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


#ifndef __GUAC_RDPSND_SERVICE_H
#define __GUAC_RDPSND_SERVICE_H

#include "config.h"

#include <freerdp/utils/svc_plugin.h>
#include <guacamole/client.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

/**
 * The maximum number of PCM formats to accept during the initial RDPSND
 * handshake with the RDP server.
 */
#define GUAC_RDP_MAX_FORMATS 16

/**
 * Abstract representation of a PCM format, including the sample rate, number
 * of channels, and bits per sample.
 */
typedef struct guac_pcm_format {

    /**
     * The sample rate of this PCM format.
     */
    int rate;

    /**
     * The number off channels used by this PCM format. This will typically
     * be 1 or 2.
     */
    int channels;

    /**
     * The number of bits per sample within this PCM format. This should be
     * either 8 or 16.
     */
    int bps;

} guac_pcm_format;

/**
 * Structure representing the current state of the Guacamole RDPSND plugin for
 * FreeRDP.
 */
typedef struct guac_rdpsndPlugin {

    /**
     * The FreeRDP parts of this plugin. This absolutely MUST be first.
     * FreeRDP depends on accessing this structure as if it were an instance
     * of rdpSvcPlugin.
     */
    rdpSvcPlugin plugin;

    /**
     * The Guacamole client associated with the guac_audio_stream that this
     * plugin should use to stream received audio packets.
     */
    guac_client* client;

    /**
     * The block number of the last SNDC_WAVE (WaveInfo) PDU received.
     */
    int waveinfo_block_number;

    /**
     * Whether the next PDU coming is a SNDWAVE (Wave) PDU. Wave PDUs do not
     * have headers, and are indicated by the receipt of a WaveInfo PDU.
     */
    int next_pdu_is_wave;

    /**
     * The wave data received within the last SNDC_WAVE (WaveInfo) PDU.
     */
    unsigned char initial_wave_data[4];

    /**
     * The size, in bytes, of the wave data in the coming Wave PDU, if any.
     * This does not include the initial wave data received within the last
     * SNDC_WAVE (WaveInfo) PDU, which is always the first four bytes of the
     * actual wave data block.
     */
    int incoming_wave_size;

    /**
     * The last received server timestamp.
     */
    int server_timestamp;

    /**
     * All formats agreed upon by server and client during the initial format
     * exchange. All of these formats will be PCM, which is the only format
     * guaranteed to be supported (based on the official RDP documentation).
     */
    guac_pcm_format formats[GUAC_RDP_MAX_FORMATS];

    /**
     * The total number of formats.
     */
    int format_count;

} guac_rdpsndPlugin;

/**
 * Handler called when this plugin is loaded by FreeRDP.
 */
void guac_rdpsnd_process_connect(rdpSvcPlugin* plugin);

/**
 * Handler called when this plugin receives data along its designated channel.
 */
void guac_rdpsnd_process_receive(rdpSvcPlugin* plugin,
        wStream* input_stream);

/**
 * Handler called when this plugin is being unloaded.
 */
void guac_rdpsnd_process_terminate(rdpSvcPlugin* plugin);

/**
 * Handler called when this plugin receives an event. For the sake of RDPSND,
 * all events will be ignored and simply free'd.
 */
void guac_rdpsnd_process_event(rdpSvcPlugin* plugin, wMessage* event);

#endif

