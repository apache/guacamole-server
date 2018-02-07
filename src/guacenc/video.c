/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "config.h"
#include "buffer.h"
#include "ffmpeg-compat.h"
#include "log.h"
#include "video.h"

#include <cairo/cairo.h>
#include <libavcodec/avcodec.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <guacamole/client.h>
#include <guacamole/timestamp.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
    int fd = open(path, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR);
    if (fd == -1) {
        guacenc_log(GUAC_LOG_ERROR, "Failed to open output file \"%s\": %s",
                path, strerror(errno));
        goto fail_output_fd;
    }

    /* Create stream for output file */
    FILE* output = fdopen(fd, "wb");
    if (output == NULL) {
        guacenc_log(GUAC_LOG_ERROR, "Failed to allocate stream for output "
                "file \"%s\": %s", path, strerror(errno));
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
    close(fd);

fail_output_fd:
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

    /* Set timestamp of frame, if frame given */
    if (frame != NULL)
        frame->pts = video->next_pts;

    /* Write frame to video */
    int got_data = guacenc_avcodec_encode_video(video, frame);
    if (got_data < 0)
        return -1;

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

    guac_timestamp next_timestamp = timestamp;

    /* Flush frames as necessary if previously updated */
    if (video->last_timestamp != 0) {

        /* Calculate the number of frames that should have been written */
        int elapsed = (timestamp - video->last_timestamp)
                    * GUACENC_VIDEO_FRAMERATE / 1000;

        /* Keep previous timestamp if insufficient time has elapsed */
        if (elapsed == 0)
            return 0;

        /* Use frame time as last_timestamp */
        next_timestamp = video->last_timestamp
                        + elapsed * 1000 / GUACENC_VIDEO_FRAMERATE;

        /* Flush frames to bring timeline in sync, duplicating if necessary */
        do {
            if (guacenc_video_flush_frame(video)) {
                guacenc_log(GUAC_LOG_ERROR, "Unable to flush frame to video "
                        "stream.");
                return 1;
            }
        } while (--elapsed != 0);

    }

    /* Update timestamp */
    video->last_timestamp = next_timestamp;
    return 0;

}

/**
 * Converts the given Guacamole video encoder buffer to a frame in the format
 * required by libavcodec / libswscale. Black margins of the specified sizes
 * will be added. No scaling is performed; the image data is copied verbatim.
 *
 * @param buffer
 *     The guacenc_buffer to copy as a new AVFrame.
 *
 * @param lsize
 *     The size of the letterboxes to add, in pixels. Letterboxes are the
 *     horizontal black boxes added to images which are scaled down to fit the
 *     destination because they are too wide (the width is scaled to exactly
 *     fit the destination, resulting in extra space at the top and bottom).
 *
 * @param psize
 *     The size of the pillarboxes to add, in pixels. Pillarboxes are the
 *     vertical black boxes added to images which are scaled down to fit the
 *     destination because they are too tall (the height is scaled to exactly
 *     fit the destination, resulting in extra space on the sides).
 *
 * @return
 *     A pointer to a newly-allocated AVFrame containing exactly the same image
 *     data as the given buffer. The image data within the frame and the frame
 *     itself must be manually freed later.
 */
static AVFrame* guacenc_video_frame_convert(guacenc_buffer* buffer, int lsize,
        int psize) {

    /* Init size of left/right pillarboxes */
    int left = psize;
    int right = psize;

    /* Init size of top/bottom letterboxes */
    int top = lsize;
    int bottom = lsize;

    /* Prepare source frame for buffer */
    AVFrame* frame = av_frame_alloc();
    if (frame == NULL)
        return NULL;

    /* Copy buffer properties to frame */
    frame->format = AV_PIX_FMT_RGB32;
    frame->width = buffer->width + left + right;
    frame->height = buffer->height + top + bottom;

    /* Allocate actual backing data for frame */
    if (av_image_alloc(frame->data, frame->linesize, frame->width,
                frame->height, frame->format, 32) < 0) {
        av_frame_free(&frame);
        return NULL;
    }

    /* Flush any pending operations */
    cairo_surface_flush(buffer->surface);

    /* Get pointer to source image data */
    unsigned char* src_data = buffer->image;
    int src_stride = buffer->stride;

    /* Get pointer to destination image data */
    unsigned char* dst_data = frame->data[0];
    int dst_stride = frame->linesize[0];

    /* Get source/destination dimensions */
    int width = buffer->width;
    int height = buffer->height;

    /* Source buffer is guaranteed to fit within destination buffer */
    assert(width <= frame->width);
    assert(height <= frame->height);

    /* Add top margin */
    while (top > 0) {
        memset(dst_data, 0, frame->width * 4);
        dst_data += dst_stride;
        top--;
    }

    /* Copy all data from source buffer to destination frame */
    while (height > 0) {

        /* Calculate size of margin and data regions */
        int left_size = left * 4;
        int data_size = width * 4;
        int right_size = right * 4;

        /* Add left margin */
        memset(dst_data, 0, left_size);

        /* Copy data */
        memcpy(dst_data + left_size, src_data, data_size);

        /* Add right margin */
        memset(dst_data + left_size + data_size, 0, right_size);

        dst_data += dst_stride;
        src_data += src_stride;

        height--;

    }

    /* Add bottom margin */
    while (bottom > 0) {
        memset(dst_data, 0, frame->width * 4);
        dst_data += dst_stride;
        bottom--;
    }

    /* Frame converted */
    return frame;

}

void guacenc_video_prepare_frame(guacenc_video* video, guacenc_buffer* buffer) {

    int lsize;
    int psize;

    /* Ignore NULL buffers */
    if (buffer == NULL || buffer->surface == NULL)
        return;

    /* Obtain destination frame */
    AVFrame* dst = video->next_frame;

    /* Determine width of image if height is scaled to match destination */
    int scaled_width = buffer->width * dst->height / buffer->height;

    /* Determine height of image if width is scaled to match destination */
    int scaled_height = buffer->height * dst->width / buffer->width;

    /* If height-based scaling results in a fit width, add pillarboxes */
    if (scaled_width <= dst->width) {
        lsize = 0;
        psize = (dst->width - scaled_width)
               * buffer->height / dst->height / 2;
    }

    /* If width-based scaling results in a fit width, add letterboxes */
    else {
        assert(scaled_height <= dst->height);
        psize = 0;
        lsize = (dst->height - scaled_height)
               * buffer->width / dst->width / 2;
    }

    /* Prepare source frame for buffer */
    AVFrame* src = guacenc_video_frame_convert(buffer, lsize, psize);
    if (src == NULL) {
        guacenc_log(GUAC_LOG_WARNING, "Failed to allocate source frame. "
                "Frame dropped.");
        return;
    }

    /* Prepare scaling context */
    struct SwsContext* sws = sws_getContext(src->width, src->height,
            AV_PIX_FMT_RGB32, dst->width, dst->height, AV_PIX_FMT_YUV420P,
            SWS_BICUBIC, NULL, NULL, NULL);

    /* Abort if scaling context could not be created */
    if (sws == NULL) {
        guacenc_log(GUAC_LOG_WARNING, "Failed to allocate software scaling "
                "context. Frame dropped.");
        av_freep(&src->data[0]);
        av_frame_free(&src);
        return;
    }

    /* Apply scaling, copying the source frame to the destination */
    sws_scale(sws, (const uint8_t* const*) src->data, src->linesize,
            0, src->height, dst->data, dst->linesize);

    /* Free scaling context */
    sws_freeContext(sws);

    /* Free source frame */
    av_freep(&src->data[0]);
    av_frame_free(&src);

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

