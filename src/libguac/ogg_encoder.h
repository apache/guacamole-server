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


#ifndef __GUAC_OGG_ENCODER_H
#define __GUAC_OGG_ENCODER_H

#include "config.h"

#include "audio.h"

#include <vorbis/vorbisenc.h>

typedef struct ogg_encoder_state {

    /**
     * Ogg state
     */
    ogg_stream_state ogg_state;
    ogg_page ogg_page;
    ogg_packet ogg_packet;

    /**
     * Vorbis state
     */
    vorbis_info info;
    vorbis_comment comment;
    vorbis_dsp_state vorbis_state;
    vorbis_block vorbis_block;

} ogg_encoder_state;

extern guac_audio_encoder* ogg_encoder;

#endif

