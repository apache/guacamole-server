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
#include "video.h"

#include <guacamole/timestamp.h>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

guacenc_video* guacenc_video_alloc(const char* path, int width, int height,
        int framerate, int bitrate) {

    /* Allocate video structure */
    guacenc_video* video = malloc(sizeof(guacenc_video));
    if (video == NULL)
        return NULL;

    /* Init properties of video */
    video->width = width;
    video->height = height;
    video->frame_duration = 1000 / framerate;
    video->bitrate = bitrate;

    /* No frames have been written or prepared yet */
    video->last_timestamp = 0;
    video->current_time = 0;
    video->next_frame = NULL;

    return video;

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

    /* STUB: Write frame as PNG */
    char filename[256];
    sprintf(filename, "frame-%012" PRId64 ".png", video->current_time);
    cairo_surface_t* surface = buffer->surface;
    if (surface != NULL)
        cairo_surface_write_to_png(surface, filename);

    /* Update internal timestamp */
    video->current_time += video->frame_duration;

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

    free(video);
    return 0;

}

