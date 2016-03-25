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
#include "ffmpeg-compat.h"
#include "log.h"
#include "video.h"

#include <libavcodec/avcodec.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <guacamole/client.h>

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int guacenc_avcodec_encode_video(guacenc_video* video, AVFrame* frame) {

/* For libavcodec < 54.1.0: avcodec_encode_video2() did not exist */
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(54,1,0)

    AVCodecContext* context = video->context;

    /* Calculate appropriate buffer size */
    int length = FF_MIN_BUFFER_SIZE + 12 * context->width * context->height;

    /* Allocate space for output */
    uint8_t* data = malloc(length);
    if (data == NULL)
        return -1;

    /* Encode packet of video data */
    int used = avcodec_encode_video(context, data, length, frame);
    if (used < 0) {
        guacenc_log(GUAC_LOG_WARNING, "Error encoding frame #%" PRId64,
                video->next_pts);
        free(data);
        return -1;
    }

    /* Report if no data needs to be written */
    if (used == 0) {
        free(data);
        return 0;
    }

    /* Write data, logging any errors */
    if (fwrite(data, 1, used, video->output) == 0) {
        guacenc_log(GUAC_LOG_ERROR, "Unable to write frame "
                "#%" PRId64 ": %s", video->next_pts, strerror(errno));
        free(data);
        return -1;
    }

    /* Data was written successfully */
    free(data);
    return 1;

#else

    /* Init video packet */
    AVPacket packet;
    av_init_packet(&packet);

    /* Request that encoder allocate data for packet */
    packet.data = NULL;
    packet.size = 0;

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

    return got_data;

#endif
}

