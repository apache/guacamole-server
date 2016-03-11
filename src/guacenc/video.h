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

#ifndef GUACENC_VIDEO_H
#define GUACENC_VIDEO_H

#include "config.h"
#include "buffer.h"

#include <guacamole/timestamp.h>

/**
 * A video which is actively being encoded. Frames can be added to the video
 * as they are generated, along with their associated timestamps, and the
 * corresponding video will be continuously written as it is encoded.
 */
typedef struct guacenc_video {

    /**
     * The width of the video, in pixels.
     */
    int width;

    /**
     * The height of the video, in pixels.
     */
    int height;

    /**
     * The duration of a single frame in milliseconds.
     */
    int frame_duration;

    /**
     * The desired output bitrate of the video, in bits per second.
     */
    int bitrate;

    /**
     * A pointer to the buffer containing the most recent frame submitted via
     * guacenc_video_prepare_frame(). This buffer MUST not be freed prior to
     * the call to guacenc_video_free().
     */
    guacenc_buffer* next_frame;

    /**
     * The timestamp associated with the last frame, or 0 if no frames have yet
     * been added.
     */
    guac_timestamp last_timestamp;

    /**
     * The relative position within the current video timeline, where 0 is the
     * first frame of video, in milliseconds. This value will be incremented as
     * frames are output.
     */
    guac_timestamp current_time;

} guacenc_video;

/**
 * Allocates a new guacenc_video which encodes video according to the given
 * specifications, saving the output in the given file. The output file will be
 * created if necessary and truncated if it already exists. Frames will be
 * scaled up or down as necessary to fit the given width and height. Note that
 * frames written to this guacenc_video may be buffered, and are not guaranteed
 * to be written until guacenc_video_free() is called.
 *
 * @param path
 *     The full path to the file in which encoded video should be written.
 *
 * @param codec_name
 *     The name of the codec to use for the video encoding, as defined by
 *     ffmpeg / libavcodec.
 *
 * @param width
 *     The width of the desired video, in pixels.
 *
 * @param height
 *     The height of the desired video, in pixels.
 *
 * @param framerate
 *     The rate at which frames should be encoded within the video, in frames
 *     per second.
 *
 * @param bitrate
 *     The desired overall bitrate of the resulting encoded video, in kilobits
 *     per second.
 */
guacenc_video* guacenc_video_alloc(const char* path, const char* codec_name,
        int width, int height, int framerate, int bitrate);

/**
 * Advances the timeline of the encoding process to the given timestamp, such
 * that frames added via guacenc_video_prepare_frame() will be encoded at the
 * proper frame boundaries within the video. Duplicate frames will be encoded
 * as necessary to ensure that the output is correctly timed with respect to
 * the given timestamp. This is particularly important as Guacamole does not
 * have a framerate per se, and the time between each Guacamole "frame" will
 * vary significantly.
 *
 * This function MUST be called prior to invoking guacenc_video_prepare_frame()
 * to ensure the prepared frame will be encoded at the correct point in time.
 *
 * @param video
 *     The video whose timeline should be adjusted.
 *
 * @param timestamp
 *     The Guacamole timestamp denoting the point in time that the video
 *     timeline should be advanced to, as dictated by a parsed "sync"
 *     instruction.
 *
 * @return
 *     Zero if the timeline was adjusted successfully, non-zero if an error
 *     occurs (such as during the encoding of duplicate frames).
 */
int guacenc_video_advance_timeline(guacenc_video* video,
        guac_timestamp timestamp);

/**
 * Stores the given buffer within the given video structure such that it will
 * be written if it falls within proper frame boundaries. If the timeline of
 * the video (as dictated by guacenc_video_advance_timeline()) is not at a
 * frame boundary with respect to the video framerate (it occurs between frame
 * boundaries), the prepared frame will only be written if another frame is not
 * prepared within the same pair of frame boundaries). The prepared frame will
 * not be written until it is implicitly flushed through updates to the video
 * timeline or through reaching the end of the encoding process
 * (guacenc_video_free()).
 *
 * Any given buffer MUST NOT be freed prior to the call to guacenc_video_free()
 * which ultimately ends the encoding process.
 *
 * @param video
 *     The video in which the given buffer should be queued for possible
 *     writing (depending on timing vs. video framerate).
 *
 * @param buffer
 *     The guacenc_buffer representing the image data of the frame that should
 *     be queued. This buffer MUST NOT be freed prior to the call to
 *     guacenc_video_free() which ultimately ends the encoding process.
 */
void guacenc_video_prepare_frame(guacenc_video* video, guacenc_buffer* buffer);

/**
 * Frees all resources associated with the given video, finalizing the encoding
 * process. Any buffered frames which have not yet been written will be written
 * at this point. Once this function is invoked, it is safe to resume freeing
 * any buffers provided to guacenc_video_prepare_frame().
 *
 * @return
 *     Zero if the video was successfully written and freed, non-zero if the
 *     video could not be written due to an error.
 */
int guacenc_video_free(guacenc_video* video);

#endif

