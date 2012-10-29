
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

#ifndef __GUAC_TEST_AUDIO_H
#define __GUAC_TEST_AUDIO_H

#include <guacamole/client.h>
#include <guacamole/stream.h>

typedef struct audio_stream audio_stream;

/**
 * Handler which is called when the audio stream is opened.
 */
typedef void audio_encoder_begin_handler(audio_stream* audio);

/**
 * Handler which is called when the audio stream is closed.
 */
typedef void audio_encoder_end_handler(audio_stream* audio);

/**
 * Handler which is called when the audio stream is flushed.
 */
typedef void audio_encoder_write_handler(audio_stream* audio,
        unsigned char* pcm_data, int length);

/**
 * Arbitrary audio codec encoder.
 */
typedef struct audio_encoder {

    /**
     * Handler which will be called when the audio stream is opened.
     */
    audio_encoder_begin_handler* begin_handler;

    /**
     * Handler which will be called when the audio stream is flushed.
     */
    audio_encoder_write_handler* write_handler;

    /**
     * Handler which will be called when the audio stream is closed.
     */
    audio_encoder_end_handler* end_handler;

} audio_encoder;

/**
 * Basic audio stream. PCM data is added to the stream. When the stream is
 * flushed, a write handler receives PCM data packets and, presumably, streams
 * them to the guac_stream provided.
 */
struct audio_stream {

    /**
     * PCM data buffer, 16-bit samples, 2-channel, 44100 Hz.
     */
    unsigned char* pcm_data;

    /**
     * Number of bytes in buffer.
     */
    int used;

    /**
     * Maximum number of bytes in buffer.
     */
    int length;

    /**
     * Encoded audio data buffer, as written by the encoder.
     */
    unsigned char* encoded_data;

    /**
     * Number of bytes in the encoded data buffer.
     */
    int encoded_data_used;

    /**
     * Maximum number of bytes in the encoded data buffer.
     */
    int encoded_data_length;

    /**
     * Arbitrary codec encoder. When the PCM buffer is flushed, PCM data will
     * be sent to this encoder.
     */
    audio_encoder* encoder;

    /**
     * The client associated with this audio stream.
     */
    guac_client* client;

    /**
     * The actual stream associated with this audio stream.
     */
    guac_stream* stream;

    /**
     * The number of samples per second of PCM data sent to this stream.
     */
    int rate;

    /**
     * The number of audio channels per sample of PCM data. Legal values are
     * 1 or 2.
     */
    int channels;

    /**
     * The number of bits per sample per channel for PCM data. Legal values are
     * 8 or 16.
     */
    int bps;

    /**
     * Encoder-specific state data.
     */
    void* data;

};

/**
 * Allocates a new audio stream.
 */
audio_stream* audio_stream_alloc(guac_client* client,
        audio_encoder* encoder);

/**
 * Frees the given audio stream.
 */
void audio_stream_free(audio_stream* stream);

/**
 * Begins a new audio stream.
 */
void audio_stream_begin(audio_stream* stream, int rate, int channels, int bps);

/**
 * Ends the current audio stream.
 */
void audio_stream_end(audio_stream* stream);

/**
 * Writes PCM data to the given audio stream.
 */
void audio_stream_write_pcm(audio_stream* stream,
        unsigned char* data, int length);

/**
 * Flushes the given audio stream.
 */
void audio_stream_flush(audio_stream* stream);

/**
 * Appends arbitrarily-encoded data to the encoded_data buffer
 * within the given audio stream.
 */
void audio_stream_write_encoded(audio_stream* audio,
        unsigned char* data, int length);

#endif

