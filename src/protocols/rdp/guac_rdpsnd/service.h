
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is libguac-client-rdp.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef __GUAC_RDPSND_SERVICE_H
#define __GUAC_RDPSND_SERVICE_H

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
     * The block number of the last SNDC_WAVE (WaveInfo) PDU received.
     */
    int waveinfo_block_number;

    /**
     * Whether the next PDU coming is a SNDWAVE (Wave) PDU. Wave PDUs do not
     * have headers, and are indicated by the receipt of a WaveInfo PDU.
     */
    int next_pdu_is_wave;

    /**
     * The size, in bytes, of the wave data in the coming Wave PDU, if any.
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
        STREAM* input_stream);

/**
 * Handler called when this plugin is being unloaded.
 */
void guac_rdpsnd_process_terminate(rdpSvcPlugin* plugin);

/**
 * Handler called when this plugin receives an event. For the sake of RDPSND,
 * all events will be ignored and simply free'd.
 */
void guac_rdpsnd_process_event(rdpSvcPlugin* plugin, RDP_EVENT* event);

#endif

