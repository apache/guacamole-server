
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

#include <guacamole/client.h>
#include <guacamole/protocol.h>

#include <vorbis/vorbisenc.h>

#include "audio.h"
#include "ogg_encoder.h"

void ogg_encoder_begin_handler(audio_stream* audio) {

    /* Allocate stream state */
    ogg_encoder_state* state = (ogg_encoder_state*)
        malloc(sizeof(ogg_encoder_state));

    /* Init state */
    vorbis_info_init(&(state->info));
    vorbis_encode_init_vbr(&(state->info), audio->channels, audio->rate, 0.4);

    vorbis_analysis_init(&(state->vorbis_state), &(state->info));
    vorbis_block_init(&(state->vorbis_state), &(state->vorbis_block));

    vorbis_comment_init(&(state->comment));
    vorbis_comment_add_tag(&(state->comment), "ENCODER", "libguac-client-rdp");

    ogg_stream_init(&(state->ogg_state), rand());

    /* Write headers */
    {
        ogg_packet header;
        ogg_packet header_comm;
        ogg_packet header_code;

        vorbis_analysis_headerout(
                &(state->vorbis_state),
                &(state->comment),
                &header, &header_comm, &header_code);

        ogg_stream_packetin(&(state->ogg_state), &header);
        ogg_stream_packetin(&(state->ogg_state), &header_comm);
        ogg_stream_packetin(&(state->ogg_state), &header_code);

        /* For each packet */
        while (ogg_stream_flush(&(state->ogg_state), &(state->ogg_page)) != 0) {

            /* Write packet header */
            audio_stream_write_encoded(audio,
                    state->ogg_page.header,
                    state->ogg_page.header_len);

            /* Write packet body */
            audio_stream_write_encoded(audio,
                    state->ogg_page.body,
                    state->ogg_page.body_len);
        }

    }

    audio->data = state;

}

void ogg_encoder_write_blocks(audio_stream* audio) {

    /* Get state */
    ogg_encoder_state* state = (ogg_encoder_state*) audio->data;

    while (vorbis_analysis_blockout(&(state->vorbis_state),
                &(state->vorbis_block)) == 1) {

        /* Analyze */
        vorbis_analysis(&(state->vorbis_block), NULL);
        vorbis_bitrate_addblock(&(state->vorbis_block));

        /* Flush Ogg pages */
        while (vorbis_bitrate_flushpacket(&(state->vorbis_state),
                    &(state->ogg_packet))) {

            /* Weld packet into bitstream */
            ogg_stream_packetin(&(state->ogg_state), &(state->ogg_packet));

            /* Write out pages */
            while (ogg_stream_pageout(&(state->ogg_state),
                        &(state->ogg_page)) != 0) {

                /* Write packet header */
                audio_stream_write_encoded(audio,
                        state->ogg_page.header,
                        state->ogg_page.header_len);

                /* Write packet body */
                audio_stream_write_encoded(audio,
                        state->ogg_page.body,
                        state->ogg_page.body_len);

                if (ogg_page_eos(&(state->ogg_page)))
                    break;

            }

        }

    }

}

void ogg_encoder_end_handler(audio_stream* audio) {

    /* Get state */
    ogg_encoder_state* state = (ogg_encoder_state*) audio->data;

    /* Write end-of-stream */
    vorbis_analysis_wrote(&(state->vorbis_state), 0);
    ogg_encoder_write_blocks(audio);

    /* Clean up encoder */
    ogg_stream_clear(&(state->ogg_state));
    vorbis_block_clear(&(state->vorbis_block));
    vorbis_dsp_clear(&(state->vorbis_state));
    vorbis_comment_clear(&(state->comment));
    vorbis_info_clear(&(state->info));

    /* Free stream state */
    free(audio->data);

}

void ogg_encoder_write_handler(audio_stream* audio, 
        unsigned char* pcm_data, int length) {

    /* Get state */
    ogg_encoder_state* state = (ogg_encoder_state*) audio->data;

    /* Calculate samples */
    int samples = length / audio->channels * 8 / audio->bps;
    int i;

    /* Get buffer */
    float** buffer = vorbis_analysis_buffer(&(state->vorbis_state), samples);

    for (i=0; i<samples; i++) {

        /* FIXME: For now, assume 2 channels, 16-bit */
        int left  = ((pcm_data[i*4+1] & 0xFF) << 8) | (pcm_data[i*4]   & 0xFF);
        int right = ((pcm_data[i*4+3] & 0xFF) << 8) | (pcm_data[i*4+2] & 0xFF);

        /* Store sample in buffer */
        buffer[0][i] = left  / 32768.f;
        buffer[1][i] = right / 32768.f;

    }

    /* Submit data */
    vorbis_analysis_wrote(&(state->vorbis_state), samples);

    /* Write data */
    ogg_encoder_write_blocks(audio);

}

/* Encoder handlers */
audio_encoder _ogg_encoder = {
    .begin_handler = ogg_encoder_begin_handler,
    .write_handler = ogg_encoder_write_handler,
    .end_handler   = ogg_encoder_end_handler
};

/* Actual encoder */
audio_encoder* ogg_encoder = &_ogg_encoder;

