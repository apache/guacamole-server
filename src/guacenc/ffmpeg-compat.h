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

#ifndef GUACENC_FFMPEG_COMPAT_H
#define GUACENC_FFMPEG_COMPAT_H

#include "config.h"
#include "video.h"

#include <libavcodec/avcodec.h>

/*
 * For a full list of FFmpeg API changes over the years, see:
 *
 *     https://github.com/FFmpeg/FFmpeg/blob/master/doc/APIchanges
 */

/* For libavcodec < 55.28.1: av_frame_*() was avcodec_*_frame(). */
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,28,1)
#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame
#endif

/* For libavcodec < 54.28.0: old avcodec_free_frame() did not exist. */
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(54,28,0)
#define avcodec_free_frame av_freep
#endif

/* For libavcodec < 55.52.0: avcodec_free_context did not exist */
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55,52,0)
#define avcodec_free_context av_freep
#endif

/* For libavcodec < 57.7.0: av_packet_unref() was av_free_packet() */
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(57,7,0)
#define av_packet_unref av_free_packet
#endif

/* For libavutil < 51.42.0: AV_PIX_FMT_* was PIX_FMT_* */
#if LIBAVUTIL_VERSION_INT < AV_VERSION_INT(51,42,0)
#define AV_PIX_FMT_RGB32 PIX_FMT_RGB32
#define AV_PIX_FMT_YUV420P PIX_FMT_YUV420P
#endif

/**
 * Writes the specied frame as a new frame of video. If pending frames of the
 * video are being flushed, the given frame may be NULL (as required by
 * avcodec_encode_video2()). If avcodec_encode_video2() does not exist, this
 * function will transparently use avcodec_encode_video().
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
int guacenc_avcodec_encode_video(guacenc_video* video, AVFrame* frame);

#endif

