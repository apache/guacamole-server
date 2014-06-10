/*
 * Copyright (C) 2013 Glyptodon LLC
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

#include "config.h"

#include "audio.h"
#include "ogg_encoder.h"

#include <vorbis/vorbisenc.h>

#include <stdlib.h>

void ogg_encoder_begin_handler(guac_audio_stream* audio) {

    /* Allocate stream state */
    ogg_encoder_state* state = (ogg_encoder_state*)
        malloc(sizeof(ogg_encoder_state));

    /* Init state */
    vorbis_info_init(&(state->info));
    vorbis_encode_init_vbr(&(state->info), audio->channels, audio->rate, 0.4);

    vorbis_analysis_init(&(state->vorbis_state), &(state->info));
    vorbis_block_init(&(state->vorbis_state), &(state->vorbis_block));

    vorbis_comment_init(&(state->comment));
    vorbis_comment_add_tag(&(state->comment), "ENCODER", "libguac");

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
            guac_audio_stream_write_encoded(audio,
                    state->ogg_page.header,
                    state->ogg_page.header_len);

            /* Write packet body */
            guac_audio_stream_write_encoded(audio,
                    state->ogg_page.body,
                    state->ogg_page.body_len);
        }

    }

    audio->data = state;

}

void ogg_encoder_write_blocks(guac_audio_stream* audio) {

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
                guac_audio_stream_write_encoded(audio,
                        state->ogg_page.header,
                        state->ogg_page.header_len);

                /* Write packet body */
                guac_audio_stream_write_encoded(audio,
                        state->ogg_page.body,
                        state->ogg_page.body_len);

                if (ogg_page_eos(&(state->ogg_page)))
                    break;

            }

        }

    }

}

void ogg_encoder_end_handler(guac_audio_stream* audio) {

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

void ogg_encoder_write_handler(guac_audio_stream* audio, 
        const unsigned char* pcm_data, int length) {

    /* Get state */
    ogg_encoder_state* state = (ogg_encoder_state*) audio->data;

    /* Calculate samples */
    int samples = length / audio->channels * 8 / audio->bps;
    int i;

    /* Get buffer */
    float** buffer = vorbis_analysis_buffer(&(state->vorbis_state), samples);

    signed char* readbuffer = (signed char*) pcm_data;

    for (i=0; i<samples; i++) {

        /* FIXME: For now, assume 2 channels, 16-bit */
        int left  = ((readbuffer[i*4+1]<<8)|(0x00ff&(int)readbuffer[i*4]));
        int right = ((readbuffer[i*4+3]<<8)|(0x00ff&(int)readbuffer[i*4+2]));

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
guac_audio_encoder _ogg_encoder = {
    .mimetype      = "audio/ogg",
    .begin_handler = ogg_encoder_begin_handler,
    .write_handler = ogg_encoder_write_handler,
    .end_handler   = ogg_encoder_end_handler
};

/* Actual encoder */
guac_audio_encoder* ogg_encoder = &_ogg_encoder;

