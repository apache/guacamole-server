
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

#ifndef __GUAC_WAV_ENCODER_H
#define __GUAC_WAV_ENCODER_H

#include "audio.h"

typedef struct wav_encoder_riff_header {

    /**
     * The RIFF chunk header, normally the string "RIFF".
     */
    unsigned char chunk_id[4];

    /**
     * Size of the entire file, not including chunk_id or chunk_size.
     */
    unsigned char chunk_size[4];

    /**
     * The format of this file, normally the string "WAVE".
     */
    unsigned char chunk_format[4];

} wav_encoder_riff_header;

typedef struct wav_encoder_fmt_header {

    /**
     * ID of this subchunk. For the fmt subchunk, this should be "fmt ".
     */
    unsigned char subchunk_id[4];

    /**
     * The size of the rest of this subchunk. For PCM, this will be 16.
     */
    unsigned char subchunk_size[4];

    /**
     * Format of this subchunk. For PCM, this will be 1.
     */
    unsigned char subchunk_format[2];

    /**
     * The number of channels in the PCM data.
     */
    unsigned char subchunk_channels[2];

    /**
     * The sample rate of the PCM data.
     */
    unsigned char subchunk_sample_rate[4];

    /**
     * The sample rate of the PCM data in bytes per second.
     */
    unsigned char subchunk_byte_rate[4];

    /**
     * The number of bytes per sample.
     */
    unsigned char subchunk_block_align[2];

    /**
     * The number of bits per sample.
     */
    unsigned char subchunk_bps[2];

} wav_encoder_fmt_header;

typedef struct wav_encoder_state {

    /**
     * Arbitrary PCM data available for writing when the overall WAV is
     * flushed.
     */
    unsigned char* data_buffer;

    /**
     * The number of bytes currently present in the data buffer.
     */
    int used;

    /**
     * The total number of bytes that can be written into the data buffer
     * without requiring resizing.
     */
    int length;

} wav_encoder_state;

typedef struct wav_encoder_data_header {

    /**
     * ID of this subchunk. For the data subchunk, this should be "data".
     */
    unsigned char subchunk_id[4];

    /**
     * The number of bytes in the PCM data.
     */
    unsigned char subchunk_size[4];

} wav_encoder_data_header;

extern audio_encoder* wav_encoder;

#endif

