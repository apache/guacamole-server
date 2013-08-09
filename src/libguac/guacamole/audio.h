
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
 * The Original Code is libguac.
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

#ifndef __GUAC_AUDIO_H
#define __GUAC_AUDIO_H

#include <guacamole/client.h>
#include <guacamole/stream.h>

/**
 * Provides functions and structures used for providing simple streaming audio.
 *
 * @file audio.h
 */

typedef struct guac_audio_stream guac_audio_stream;

/**
 * Handler which is called when the audio stream is opened.
 */
typedef void guac_audio_encoder_begin_handler(guac_audio_stream* audio);

/**
 * Handler which is called when the audio stream is closed.
 */
typedef void guac_audio_encoder_end_handler(guac_audio_stream* audio);

/**
 * Handler which is called when the audio stream is flushed.
 */
typedef void guac_audio_encoder_write_handler(guac_audio_stream* audio,
        unsigned char* pcm_data, int length);

/**
 * Arbitrary audio codec encoder.
 */
typedef struct guac_audio_encoder {

    /**
     * The mimetype of the audio data encoded by this audio
     * encoder.
     */
    const char* mimetype;

    /**
     * Handler which will be called when the audio stream is opened.
     */
    guac_audio_encoder_begin_handler* begin_handler;

    /**
     * Handler which will be called when the audio stream is flushed.
     */
    guac_audio_encoder_write_handler* write_handler;

    /**
     * Handler which will be called when the audio stream is closed.
     */
    guac_audio_encoder_end_handler* end_handler;

} guac_audio_encoder;

/**
 * Basic audio stream. PCM data is added to the stream. When the stream is
 * flushed, a write handler receives PCM data packets and, presumably, streams
 * them to the guac_stream provided.
 */
struct guac_audio_stream {

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
    guac_audio_encoder* encoder;

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
     * The number of PCM bytes written since the audio chunk began.
     */
    int pcm_bytes_written;

    /**
     * Encoder-specific state data.
     */
    void* data;

};

/**
 * Allocates a new audio stream which encodes audio data using the given
 * encoder. If NULL is specified for the encoder, an appropriate encoder
 * will be selected based on the encoders built into libguac and the level
 * of client support.
 *
 * @param client The guac_client for which this audio stream is being
 *               allocated.
 * @param encoder The guac_audio_encoder to use when encoding audio, or
 *                NULL if libguac should select an appropriate built-in
 *                encoder on its own.
 * @return The newly allocated guac_audio_stream, or NULL if no audio
 *         stream could be allocated due to lack of client support.
 */
guac_audio_stream* guac_audio_stream_alloc(guac_client* client,
        guac_audio_encoder* encoder);

/**
 * Frees the given audio stream.
 *
 * @param stream The guac_audio_stream to free.
 */
void guac_audio_stream_free(guac_audio_stream* stream);

/**
 * Begins a new audio packet within the given audio stream. This packet will be
 * built up with repeated writes of PCM data, finally being sent when complete
 * via guac_audio_stream_end().
 *
 * @param stream The guac_audio_stream which should start a new audio packet.
 * @param rate The audio rate of the packet, in Hz.
 * @param channels The number of audio channels.
 * @param bps The number of bits per audio sample.
 */
void guac_audio_stream_begin(guac_audio_stream* stream, int rate, int channels, int bps);

/**
 * Ends the current audio packet, writing the finished packet as an audio
 * instruction.
 *
 * @param stream The guac_audio_stream whose current audio packet should be
 *               completed and sent.
 */
void guac_audio_stream_end(guac_audio_stream* stream);

/**
 * Writes PCM data to the given audio stream. This PCM data will be
 * automatically encoded by the audio encoder associated with this stream. This
 * function must only be called after an audio packet has been started with
 * guac_audio_stream_begin().
 *
 * @param stream The guac_audio_stream to write PCM data through.
 * @param data The PCM data to write.
 * @param length The number of bytes of PCM data provided.
 */
void guac_audio_stream_write_pcm(guac_audio_stream* stream,
        unsigned char* data, int length);

/**
 * Flushes the given audio stream.
 *
 * @param stream The guac_audio_stream to flush.
 */
void guac_audio_stream_flush(guac_audio_stream* stream);

/**
 * Appends arbitrarily-encoded data to the encoded_data buffer within the given
 * audio stream. This data must be encoded in the output format of the encoder
 * used by the stream. This function is mainly for use by encoder
 * implementations.
 *
 * @param audio The guac_audio_stream to write data through.
 * @param data Arbitrary encoded data to write through the audio stream.
 * @param length The number of bytes of data provided.
 */
void guac_audio_stream_write_encoded(guac_audio_stream* audio,
        unsigned char* data, int length);

#endif

