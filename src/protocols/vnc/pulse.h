
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
 * The Original Code is libguac-client-vnc.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2010
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

#ifndef __GUAC_VNC_PULSE_H
#define __GUAC_VNC_PULSE_H

/**
 * The number of bytes to request for the PulseAudio buffers.
 */
#define GUAC_VNC_AUDIO_BUFFER_SIZE 10240

/**
 * The minimum number of PCM bytes to wait for before flushing an audio
 * packet.
 */
#define GUAC_VNC_PCM_WRITE_RATE 10240

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

