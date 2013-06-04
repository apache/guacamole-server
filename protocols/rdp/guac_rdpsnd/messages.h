
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

#ifndef __GUAC_RDPSND_MESSAGES_H
#define __GUAC_RDPSND_MESSAGES_H

/*
 * PDU Message Types
 */

/**
 * Close PDU
 */
#define SNDC_CLOSE         1

/**
 * WaveInfo PDU. This PDU is sent just before wave data is sent.
 */
#define SNDC_WAVE          2

/**
 * Wave Confirm PDU. This PDU is sent in response to the WaveInfo PDU,
 * confirming it has been received and played.
 */
#define SNDC_WAVECONFIRM   5

/**
 * Training PDU. This PDU is sent by the server occasionally and must be
 * responded to with another training PDU, similar to Guac's sync message.
 */
#define SNDC_TRAINING      6

/**
 * Server Audio Formats and Version PDU. This PDU is sent by the server to
 * advertise to the client which audio formats are supported.
 */
#define SNDC_FORMATS       7

/**
 * Quality Mode PDU. This PDU must be sent by the client to select an audio
 * quality mode if the server is at least version 6.
 */
#define SNDC_QUALITYMODE   12

/*
 * Quality Modes
 */

/**
 * Dynamic Quality. The server will choose the audio quality based on its
 * perception of latency.
 */
#define DYNAMIC_QUALITY    0x0000

/**
 * Medium Quality. The server prioritizes bandwidth over quality.
 */
#define MEDIUM_QUALITY     0x0001

/**
 * High Quality. The server prioritizes quality over bandwidth.
 */
#define HIGH_QUALITY       0x0002

/*
 * Capabilities
 */
#define TSSNDCAPS_ALIVE  1

/*
 * Sound Formats
 */
#define WAVE_FORMAT_PCM  1

/**
 * The header common to all RDPSND PDUs.
 */
typedef struct guac_rdpsnd_pdu_header {

    /**
     * The type of message represented by this PDU (SNDC_WAVE, etc.)
     */
    int message_type;

    /**
     * The size of the remainder of the message.
     */
    int body_size;

} guac_rdpsnd_pdu_header;

/**
 * Handler for the SNDC_FORMATS (Server Audio Formats and Version) PDU.
 */
void guac_rdpsnd_formats_handler(guac_rdpsndPlugin* rdpsnd,
        audio_stream* audio, STREAM* input_stream,
        guac_rdpsnd_pdu_header* header);

/**
 * Handler for the SNDC_TRAINING (Training) PDU.
 */
void guac_rdpsnd_training_handler(guac_rdpsndPlugin* rdpsnd,
        audio_stream* audio, STREAM* input_stream,
        guac_rdpsnd_pdu_header* header);

/**
 * Handler for the SNDC_WAVE (WaveInfo) PDU.
 */
void guac_rdpsnd_wave_info_handler(guac_rdpsndPlugin* rdpsnd,
        audio_stream* audio, STREAM* input_stream,
        guac_rdpsnd_pdu_header* header);

/**
 * Handler for the SNDWAV (Wave) PDU which follows any WaveInfo PDU.
 */
void guac_rdpsnd_wave_handler(guac_rdpsndPlugin* rdpsnd,
        audio_stream* audio, STREAM* input_stream,
        guac_rdpsnd_pdu_header* header);

/**
 * Handler for the SNDC_CLOSE (Close) PDU.
 */
void guac_rdpsnd_close_handler(guac_rdpsndPlugin* rdpsnd,
        audio_stream* audio, STREAM* input_stream,
        guac_rdpsnd_pdu_header* header);

#endif

