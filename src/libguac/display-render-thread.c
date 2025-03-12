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
#include "display-priv.h"
#include "guacamole/client.h"
#include "guacamole/display.h"
#include "guacamole/flag.h"
#include "guacamole/mem.h"
#include "guacamole/timestamp.h"

/**
 * The maximum duration of a frame in milliseconds. This ensures we at least
 * meet a reasonable minimum framerate in the case that the remote desktop
 * server provides no frame boundaries and streams data continuously enough
 * that frame boundaries are not discernable through timing.
 *
 * The current value of 100 is equivalent to 10 frames per second.
 */
#define GUAC_DISPLAY_RENDER_THREAD_MAX_FRAME_DURATION 100

/**
 * The minimum duration of a frame in milliseconds. This ensures we don't start
 * flushing a ton of tiny frames if a remote desktop server provides no frame
 * boundaries and streams data inconsistently enough that timing would suggest
 * frame boundaries in the middle of a frame.
 *
 * The current value of 10 is equivalent to 100 frames per second.
 */
#define GUAC_DISPLAY_RENDER_THREAD_MIN_FRAME_DURATION 10

/**
 * The start routine for the display render thread, consisting of a single
 * render loop. The render loop will proceed until signalled to stop,
 * determining frame boundaries via a combination of heuristics and explicit
 * marking (if available).
 *
 * @param data
 *     The guac_display_render_thread structure containing the render thread
 *     state.
 *
 * @return
 *     Always NULL.
 */
static void* guac_display_render_loop(void* data) {

    guac_display_render_thread* render_thread = (guac_display_render_thread*) data;
    guac_display* display = render_thread->display;

    for (;;) {

        /* Wait indefinitely for any change to the frame state */
        guac_flag_wait_and_lock(&render_thread->state,
                  GUAC_DISPLAY_RENDER_THREAD_STATE_STOPPING
                | GUAC_DISPLAY_RENDER_THREAD_STATE_FRAME_READY
                | GUAC_DISPLAY_RENDER_THREAD_STATE_FRAME_MODIFIED);

        /* Bail out immediately upon upcoming disconnect */
        if (render_thread->state.value & GUAC_DISPLAY_RENDER_THREAD_STATE_STOPPING) {
            guac_flag_unlock(&render_thread->state);
            return NULL;
        }

        int rendered_frames = 0;

        /* Lacking explicit frame boundaries, handle the change in frame state,
         * continuing to accumulate frame modifications while still within
         * heuristically determined frame boundaries */
        int allowed_wait = 0;
        guac_timestamp frame_start = guac_timestamp_current();
        do {

            /* Continue processing messages for up to a reasonable
             * minimum framerate without an explicit frame boundary
             * indicating that the frame is not yet complete */
            int frame_duration = guac_timestamp_current() - frame_start;
            if (frame_duration > GUAC_DISPLAY_RENDER_THREAD_MAX_FRAME_DURATION) {
                guac_flag_unlock(&render_thread->state);
                break;
            }

            /* Use explicit frame boundaries whenever available */
            if (render_thread->state.value & GUAC_DISPLAY_RENDER_THREAD_STATE_FRAME_READY) {

                rendered_frames = render_thread->frames;
                render_thread->frames = 0;

                guac_flag_clear(&render_thread->state,
                          GUAC_DISPLAY_RENDER_THREAD_STATE_FRAME_READY
                        | GUAC_DISPLAY_RENDER_THREAD_STATE_FRAME_MODIFIED);
                guac_flag_unlock(&render_thread->state);
                break;

            }

            /* Do not exceed a reasonable maximum framerate without an
             * explicit frame boundary terminating the frame early */
            allowed_wait = GUAC_DISPLAY_RENDER_THREAD_MIN_FRAME_DURATION - frame_duration;
            if (allowed_wait < 0)
                allowed_wait = 0;

            /* Wait for further modifications or other changes to frame state */

            guac_flag_clear(&render_thread->state, GUAC_DISPLAY_RENDER_THREAD_STATE_FRAME_MODIFIED);
            guac_flag_unlock(&render_thread->state);

        } while (guac_flag_timedwait_and_lock(&render_thread->state,
                      GUAC_DISPLAY_RENDER_THREAD_STATE_STOPPING
                    | GUAC_DISPLAY_RENDER_THREAD_STATE_FRAME_READY
                    | GUAC_DISPLAY_RENDER_THREAD_STATE_FRAME_MODIFIED, allowed_wait));

        guac_display_end_multiple_frames(display, rendered_frames);

    }

    return NULL;

}

guac_display_render_thread* guac_display_render_thread_create(guac_display* display) {

    guac_display_render_thread* render_thread = guac_mem_alloc(sizeof(guac_display_render_thread));

    guac_flag_init(&render_thread->state);
    render_thread->display = display;
    render_thread->frames = 0;

    /* Start render thread (this will immediately begin blocking until frame
     * modification or readiness is signalled) */
    pthread_create(&render_thread->thread, NULL, guac_display_render_loop, render_thread);

    return render_thread;

}

void guac_display_render_thread_notify_modified(guac_display_render_thread* render_thread) {
    guac_flag_set(&render_thread->state, GUAC_DISPLAY_RENDER_THREAD_STATE_FRAME_MODIFIED);
}

void guac_display_render_thread_notify_frame(guac_display_render_thread* render_thread) {
    guac_flag_set_and_lock(&render_thread->state, GUAC_DISPLAY_RENDER_THREAD_STATE_FRAME_READY);
    render_thread->frames++;
    guac_flag_unlock(&render_thread->state);
}

void guac_display_render_thread_destroy(guac_display_render_thread* render_thread) {

    /* Clean up render thread after signalling it to stop */
    guac_flag_set(&render_thread->state, GUAC_DISPLAY_RENDER_THREAD_STATE_STOPPING);
    pthread_join(render_thread->thread, NULL);

    /* Free remaining resources */
    guac_flag_destroy(&render_thread->state);
    guac_mem_free(render_thread);

}

