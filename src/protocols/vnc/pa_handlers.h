
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
 * The Original Code is pa_handlers.
 *
 * The Initial Developers of the Original Code are
 *   Craig Hokanson <craig.hokanson@sv.cmu.edu>
 *   Sion Chaudhuri <sion.chaudhuri@sv.cmu.edu>
 *   Gio Perez <gio.perez@sv.cmu.edu>
 * Portions created by the Initial Developer are Copyright (C) 2013
 * the Initial Developers. All Rights Reserved.
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

#ifndef __GUAC_VNC_PA_HANDLERS_H
#define __GUAC_VNC_PA_HANDLERS_H

#include <guacamole/audio.h>

/**
 * The size of each data element in the audio buffer.
 */
#define BUF_DATA_SIZE 1024

/**
 * The length of the audio buffer 
 */
#define BUF_LENGTH 100

/**
 * The number of samples per second of PCM data sent to this stream.
 */
#define SAMPLE_RATE 44100

/**
 * The number of audio channels per sample of PCM data. Legal values are
 * 1 or 2.
 */
#define CHANNELS 2

/**
 * The number of bits per sample per channel for PCM data. Only 16 is supported.
 */
#define BPS 16

/**
 * Minimum interval between two audio send instructions
 */
#define SEND_INTERVAL 500

/**
 * Reads audio data from Pulse Audio and inserts it into the
 * audio buffer
 *
 * @param data arguments for the read audio thread
 */
void* guac_pa_read_audio(void* data);

/**
 * Gets audio data from the audio buffer and sends it to
 * guacamole
 *
 * @param data arguments for the send audio thread
 */
void* guac_pa_send_audio(void* data);

/**
 * Sleep for the given number of milliseconds.
 *
 * @param millis The number of milliseconds to sleep.
 */
void guac_pa_sleep(int millis);

#endif
