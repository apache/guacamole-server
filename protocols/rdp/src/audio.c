
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

#include <stdlib.h>
#include <string.h>
#include <guacamole/protocol.h>
#include <guacamole/client.h>
#include <guacamole/stream.h>

#include "audio.h"

audio_stream* audio_stream_alloc(guac_client* client, audio_encoder* encoder) {

    /* Allocate stream */
    audio_stream* audio = (audio_stream*) malloc(sizeof(audio_stream));
    audio->client = client;

    /* Reset buffer stats */
    audio->used = 0;
    audio->length = 0x40000;

    audio->encoded_data_used = 0;
    audio->encoded_data_length = 0x40000;

    /* Allocate bufferis */
    audio->pcm_data = malloc(audio->length);
    audio->encoded_data = malloc(audio->encoded_data_length);

    /* Assign encoder */
    audio->encoder = encoder;
    audio->stream = guac_client_alloc_stream(client);

    return audio;
}

void audio_stream_begin(audio_stream* audio) {
    audio->encoder->begin_handler(audio);
}

void audio_stream_end(audio_stream* audio) {
    audio_stream_flush(audio);
    audio->encoder->end_handler(audio);
}

void audio_stream_free(audio_stream* audio) {
    free(audio->pcm_data);
    free(audio);
}

void audio_stream_write_pcm(audio_stream* audio, 
        unsigned char* data, int length) {

    /* Resize audio buffer if necessary */
    if (length > audio->length) {

        /* Resize to double provided length */
        audio->length = length*2;
        audio->pcm_data = realloc(audio->pcm_data, audio->length);

    }

    /* Flush if necessary */
    if (audio->used + length > audio->length)
        audio_stream_flush(audio);

    /* Append to buffer */
    memcpy(&(audio->pcm_data[audio->used]), data, length);
    audio->used += length;

}

void audio_stream_flush(audio_stream* audio) {

    /* If data in buffer */
    if (audio->used != 0) {

        /* Write data */
        audio->encoder->write_handler(audio,
                audio->pcm_data, audio->used);

        /* Reset buffer */
        audio->used = 0;

    }

}

void audio_stream_append_data(audio_stream* audio,
        unsigned char* data, int length) {

    /* Resize audio buffer if necessary */
    if (audio->encoded_data_used + length > audio->encoded_data_length) {

        /* Increase to double concatenated size to accomodate */
        audio->encoded_data_length = (audio->encoded_data_length + length)*2;
        audio->encoded_data = realloc(audio->encoded_data,
                audio->encoded_data_length);

    }

    /* Append to buffer */
    memcpy(&(audio->encoded_data[audio->encoded_data_used]), data, length);
    audio->encoded_data_used += length;

}

void audio_stream_clear_data(audio_stream* audio) {
    audio->encoded_data_used = 0;
}

