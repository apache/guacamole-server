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
        int width, int height, int bitrate) {

    /* Pull codec based on name */
    AVCodec* codec = avcodec_find_encoder_by_name(codec_name);
    if (codec == NULL) {
        guacenc_log(GUAC_LOG_ERROR, "Failed to locate codec \"%s\".",
                codec_name);
        goto fail_codec;
    }

    /* Retrieve encoding context */
    AVCodecContext* context = avcodec_alloc_context3(codec);
    if (context == NULL) {
        guacenc_log(GUAC_LOG_ERROR, "Failed to allocate context for "
                "codec \"%s\".", codec_name);
        goto fail_context;
    }

    /* Init context with encoding parameters */
    context->bit_rate = bitrate;
    context->width = width;
    context->height = height;
    context->time_base = (AVRational) { 1, GUACENC_VIDEO_FRAMERATE };
    context->gop_size = 10;
    context->max_b_frames = 1;
    context->pix_fmt = AV_PIX_FMT_YUV420P;

    /* Open codec for use */
    if (avcodec_open2(context, codec, NULL) < 0) {
        guacenc_log(GUAC_LOG_ERROR, "Failed to open codec \"%s\".", codec_name);
        goto fail_codec_open;
    }

    /* Allocate corresponding frame */
    AVFrame* frame = av_frame_alloc();
    if (frame == NULL) {
        goto fail_frame;
    }

    /* Copy necessary data for frame from context */
    frame->format = context->pix_fmt;
    frame->width = context->width;
    frame->height = context->height;

    /* Allocate actual backing data for frame */
    if (av_image_alloc(frame->data, frame->linesize, frame->width,
                frame->height, frame->format, 32) < 0) {
        goto fail_frame_data;
    }

    /* Open output file */
    FILE* output = fopen(path, "wb");
    if (output == NULL) {
        guacenc_log(GUAC_LOG_ERROR, "Failed to open output file \"%s\": %s",
                path, strerror(errno));
        goto fail_output_file;
    }

    /* Allocate video structure */
    guacenc_video* video = malloc(sizeof(guacenc_video));
    if (video == NULL) {
        goto fail_video;
    }

    /* Init properties of video */
    video->output = output;
    video->context = context;
    video->next_frame = frame;
    video->width = width;
    video->height = height;
    video->bitrate = bitrate;

    /* No frames have been written or prepared yet */
    video->last_timestamp = 0;
    video->next_pts = 0;

    return video;

    /* Free all allocated data in case of failure */
fail_video:
    fclose(output);

fail_output_file:
    av_freep(&frame->data[0]);

fail_frame_data:
    av_frame_free(&frame);

fail_frame:
fail_codec_open:
    avcodec_free_context(&context);

fail_context:
fail_codec:
    return NULL;

}

/**
 * Flushes the specied frame as a new frame of video, updating the internal
 * video timestamp by one frame's worth of time. The pts member of the given
 * frame structure will be updated with the current presentation timestamp of
 * the video. If pending frames of the video are being flushed, the given frame
 * may be NULL (as required by avcodec_encode_video2()).
 *
 * @param video
 *     The video to write the given frame to.
 *
 * @param frame
 *     The frame to write to the video, or NULL if previously-written frames
 *     are being flushed.
 *
 * @return
 *     A positive value if the frame was successfully written, zero if the
 *     frame has been saved for later writing / reordering, negative if an
 *     error occurs.
 */
static int guacenc_video_write_frame(guacenc_video* video, AVFrame* frame) {

    /* Init video packet */
    AVPacket packet;
    av_init_packet(&packet);

    /* Request that encoder allocate data for packet */
    packet.data = NULL;
    packet.size = 0;

    /* Set timestamp of frame, if frame given */
    if (frame != NULL)
        frame->pts = video->next_pts;

    /* Write frame to video */
    int got_data;
    if (avcodec_encode_video2(video->context, &packet, frame, &got_data) < 0) {
        guacenc_log(GUAC_LOG_WARNING, "Error encoding frame #%" PRId64,
                video->next_pts);
        return -1;
    }

    /* Write corresponding data to file */
    if (got_data) {

        /* Write data, logging any errors */
        if (fwrite(packet.data, 1, packet.size, video->output) == 0) {
            guacenc_log(GUAC_LOG_ERROR, "Unable to write frame "
                    "#%" PRId64 ": %s", video->next_pts, strerror(errno));
            return -1;
        }

        /* Data was written successfully */
        guacenc_log(GUAC_LOG_DEBUG, "Frame #%08" PRId64 ": wrote %i bytes",
                video->next_pts, packet.size);
        av_packet_unref(&packet);

    }

    /* Frame may have been queued for later writing / reordering */
    else
        guacenc_log(GUAC_LOG_DEBUG, "Frame #%08" PRId64 ": queued for later",
                video->next_pts);

    /* Update presentation timestamp for next frame */
    video->next_pts++;

    /* Write was successful */
    return got_data;

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

    /* Write frame to video */
    return guacenc_video_write_frame(video, video->next_frame) < 0;

}

int guacenc_video_advance_timeline(guacenc_video* video,
        guac_timestamp timestamp) {

    /* Flush frames as necessary if previously updated */
    if (video->last_timestamp != 0) {

        /* Calculate the number of frames that should have been written */
        int elapsed = (timestamp - video->last_timestamp)
                    * GUACENC_VIDEO_FRAMERATE / 1000;

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

    /* TODO: Convert frame buffer to video->next_frame */

}

int guacenc_video_free(guacenc_video* video) {

    /* Ignore NULL video */
    if (video == NULL)
        return 0;

    /* Write final frame */
    guacenc_video_flush_frame(video);

    /* Init video packet for final flush of encoded data */
    AVPacket packet;
    av_init_packet(&packet);

    /* Flush any unwritten frames */
    int retval;
    do {
        retval = guacenc_video_write_frame(video, NULL);
    } while (retval > 0);

    /* File is now completely written */
    fclose(video->output);

    /* Free frame encoding data */
    av_freep(&video->next_frame->data[0]);
    av_frame_free(&video->next_frame);

    /* Clean up encoding context */
    avcodec_close(video->context);
    avcodec_free_context(&(video->context));

    free(video);
    return 0;

}

