/*
 * Copyright (C) 2016 Glyptodon, Inc.
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
#include "buffer.h"
#include "log.h"
#include "video.h"

#include <libavcodec/avcodec.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <guacamole/client.h>
#include <guacamole/timestamp.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

guacenc_video* guacenc_video_alloc(const char* path, const char* codec_name,
        int width, int height, int framerate, int bitrate) {

    /* Pull codec based on name */
    AVCodec* codec = avcodec_find_encoder_by_name(codec_name);
    if (codec == NULL) {
        guacenc_log(GUAC_LOG_ERROR, "Failed to locate codec \"%s\".",
                codec_name);
        return NULL;
    }

    /* Retrieve encoding context */
    AVCodecContext* context = avcodec_alloc_context3(codec);
    if (context == NULL) {
        guacenc_log(GUAC_LOG_ERROR, "Failed to allocate context for "
                "codec \"%s\".", codec_name);
        return NULL;
    }

    /* Init context with encoding parameters */
    context->bit_rate = bitrate;
    context->width = width;
    context->height = height;
    context->time_base = (AVRational) { 1, framerate };
    context->gop_size = 10;
    context->max_b_frames = 1;
    context->pix_fmt = AV_PIX_FMT_YUV420P;

    /* Open codec for use */
    if (avcodec_open2(context, codec, NULL) < 0) {
        guacenc_log(GUAC_LOG_ERROR, "Failed to open codec \"%s\".", codec_name);
        goto fail_context;
    }

    /* Allocate corresponding frame */
    AVFrame* frame = av_frame_alloc();
    if (frame == NULL) {
        goto fail_context;
    }

    /* Copy necessary data for frame from context */
    frame->format = context->pix_fmt;
    frame->width = context->width;
    frame->height = context->height;

    /* Allocate actual backing data for frame */
    if (av_image_alloc(frame->data, frame->linesize, frame->width,
                frame->height, frame->format, 32) < 0) {
        goto fail_frame;
    }

    /* Allocate video structure */
    guacenc_video* video = malloc(sizeof(guacenc_video));
    if (video == NULL) {
        goto fail_frame_data;
    }

    /* Init properties of video */
    video->context = context;
    video->frame = frame;
    video->width = width;
    video->height = height;
    video->frame_duration = 1000 / framerate;
    video->bitrate = bitrate;

    /* No frames have been written or prepared yet */
    video->last_timestamp = 0;
    video->next_pts = 0;
    video->next_frame = NULL;

    return video;

    /* Free all allocated data in case of failure */
fail_frame_data:
    av_freep(&frame->data[0]);

fail_frame:
    av_frame_free(&frame);

fail_context:
    avcodec_free_context(&context);
    return NULL;

}

/**
 * Flushes the frame previously specified by guacenc_video_prepare_frame() as a
 * new frame of video, updating the internal video timestamp by one frame's
 * worth of time.
 *
 * @param video
 *     The video to flush.
 *
 * @return
 *     Zero if flushing was successful, non-zero if an error occurs.
 */
static int guacenc_video_flush_frame(guacenc_video* video) {

    /* Ignore empty frames */
    guacenc_buffer* buffer = video->next_frame;
    if (buffer == NULL)
        return 0;

    /* Init video packet */
    AVPacket packet;
    av_init_packet(&packet);

    /* Request that encoder allocate data for packet */
    packet.data = NULL;
    packet.size = 0;

    /* TODO: Write frame to video */
    guacenc_log(GUAC_LOG_DEBUG, "Encoding frame #%08" PRId64, video->next_pts);

    /* Update presentation timestamp for next frame */
    video->next_pts++;

    return 0;

}

int guacenc_video_advance_timeline(guacenc_video* video,
        guac_timestamp timestamp) {

    /* Flush frames as necessary if previously updated */
    if (video->last_timestamp != 0) {

        /* Calculate the number of frames that should have been written */
        int elapsed = (timestamp - video->last_timestamp)
                    / video->frame_duration;

        /* Keep previous timestamp if insufficient time has elapsed */
        if (elapsed == 0)
            return 0;

        /* Flush frames to bring timeline in sync, duplicating if necessary */
        do {
            guacenc_video_flush_frame(video);
        } while (--elapsed != 0);

    }

    /* Update timestamp */
    video->last_timestamp = timestamp;
    return 0;

}

void guacenc_video_prepare_frame(guacenc_video* video, guacenc_buffer* buffer) {
    video->next_frame = buffer;
}

int guacenc_video_free(guacenc_video* video) {

    /* Ignore NULL video */
    if (video == NULL)
        return 0;

    /* Write final frame */
    guacenc_video_flush_frame(video);

    /* Free frame encoding data */
    av_freep(&video->frame->data[0]);
    av_frame_free(&video->frame);

    /* Clean up encoding context */
    avcodec_close(video->context);
    avcodec_free_context(&(video->context));

    free(video);
    return 0;

}

