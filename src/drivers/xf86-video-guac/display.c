
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
#include "common/cursor.h"
#include "common/display.h"
#include "daemon.h"
#include "display.h"
#include "drawable.h"
#include "user.h"
#include "list.h"
#include "log.h"

#include <xf86.h>
#include <xf86str.h>
#include <randrstr.h>
#include <guacamole/client.h>
#include <guacamole/timestamp.h>

#include <sys/time.h>

/**
 * Populate the given timespec with the curren time, plus the given sec/usec offsets.
 */
static void _get_absolute_time(struct timespec* ts, int offset_sec, int offset_usec) {

    /* Get timeval */
    struct timeval tv;
    gettimeofday(&tv, NULL);

    /* Update with offset */
    tv.tv_sec  += offset_sec;
    tv.tv_usec += offset_usec;

    /* Wrap to next second if necessary */
    if (tv.tv_usec >= 1000000L) {
        tv.tv_sec++;
        tv.tv_usec -= 1000000L;
    }

    /* Convert to timespec */
    ts->tv_sec  = tv.tv_sec;
    ts->tv_nsec = tv.tv_usec * 1000;

}

/**
 * Waits until changes have been made to visible content of the given
 * guac_drv_display, and thus those changes should be flushed to connected
 * users. If the timeout elapses before data is available, zero is returned.
 *
 * @param display
 *     The guac_drv_display to wait for.
 *
 * @param timeout
 *     The maximum amount of time to wait, in microseconds.
 *
 * @returns
 *     A non-zero value if changes were made to the display, or zero if the
 *     timeout elapses prior to any such changes.
 */
static int guac_drv_wait_for_changes(guac_drv_display* display, int msecs) {

    int retval = 1;

    pthread_mutex_t* mod_lock = &(display->modified_lock);
    pthread_cond_t* mod_cond = &(display->modified_cond);

    /* Split provided milliseconds into microseconds and whole seconds */
    int secs  =  msecs / 1000;
    int usecs = (msecs % 1000) * 1000;

    /* Calculate absolute timestamp from provided relative timeout */
    struct timespec timeout;
    _get_absolute_time(&timeout, secs, usecs);

    /* Test for display modification */
    pthread_mutex_lock(mod_lock);
    if (display->modified)
        goto wait_complete;

    /* If not yet modified, wait for modification condition to be signaled */
    retval = pthread_cond_timedwait(mod_cond, mod_lock, &timeout) != ETIMEDOUT;

wait_complete:
    display->modified = 0;
    pthread_mutex_unlock(mod_lock);
    return retval;

}

void* guac_drv_render_thread(void* arg) {

    guac_drv_display* display = (guac_drv_display*) arg;
    guac_client* client = display->client;

    guac_timestamp last_frame_end = guac_timestamp_current();

    /* Handle display changes while client is running */
    while (client->state == GUAC_CLIENT_RUNNING) {

        /* Wait for start of frame */
        int display_changed = guac_drv_wait_for_changes(display,
                GUAC_DRV_FRAME_START_TIMEOUT);
        if (display_changed) {

            int processing_lag = guac_client_get_processing_lag(client);
            guac_timestamp frame_start = guac_timestamp_current();

            /* Continue waiting until frame is complete */
            do {

                /* Calculate time remaining in frame */
                guac_timestamp frame_end = guac_timestamp_current();
                int frame_remaining = frame_start + GUAC_DRV_FRAME_MAX_DURATION
                                    - frame_end;

                /* Calculate time that client needs to catch up */
                int time_elapsed = frame_end - last_frame_end;
                int required_wait = processing_lag - time_elapsed;

                /* Increase the duration of this frame if client is lagging */
                if (required_wait > GUAC_DRV_FRAME_TIMEOUT)
                    display_changed = guac_drv_wait_for_changes(display,
                            required_wait);

                /* Wait again if frame remaining */
                else if (frame_remaining > 0)
                    display_changed = guac_drv_wait_for_changes(display,
                            GUAC_DRV_FRAME_TIMEOUT);

                else
                    break;

            } while (display_changed);

            /* Record end of frame, excluding server-side rendering time (we
             * assume server-side rendering time will be consistent between any
             * two subsequent frames, and that this time should thus be
             * excluded from the required wait period of the next frame). */
            last_frame_end = frame_start;

        } /* end if display modified in time */

        /* End frame */
        guac_drv_display_flush(display);

    }

    return NULL;

}

guac_drv_display* guac_drv_display_alloc(ScreenPtr screen,
        const char* address, const char* port) {

    guac_drv_display* display = malloc(sizeof(guac_drv_display));

    /* Init underlying client */
    guac_client* client = guac_client_alloc();
    client->join_handler = guac_drv_user_join_handler;
    client->log_handler = guac_drv_client_log;
    client->data = display;

    display->listen_address = address;
    display->listen_port = port;
    display->screen = screen;
    display->display = guac_common_display_alloc(client,
            screen->width, screen->height);
    display->client = client;
    display->modified = 0;

    /* Set default pointer */
    guac_common_cursor_set_pointer(display->display->cursor);

    /* Init watchdog condition */
    if (pthread_cond_init(&(display->modified_cond), NULL)) {
        return NULL;
    }

    /* Init watchdog mutex */
    if (pthread_mutex_init(&(display->modified_lock), NULL)) {
        return NULL;
    }

    /* Start watchdog thread */
    if (pthread_create(&(display->render_thread), NULL,
            guac_drv_render_thread, display)) {
        return NULL;
    }

    /* Start listen thread */
    if (pthread_create(&(display->listen_thread), NULL,
            guac_drv_listen_thread, display)) {
        return NULL;
    }

    return display;

}

void guac_drv_display_resize(guac_drv_display* display, int w, int h) {
    xf86Msg(X_INFO, "guac: Resizing surface to %ix%i\n", w, h);
    guac_common_surface_resize(display->display->default_surface, w, h);
}

void guac_drv_display_request_resize(guac_drv_display* display, int w, int h) {
    xf86Msg(X_INFO, "guac: Requesting resize to %ix%i\n", w, h);
    RRScreenSizeSet(display->screen, w, h, 0, 0);
}

guac_drv_drawable* guac_drv_display_create_layer(guac_drv_display* display,
        guac_drv_drawable* parent, int x, int y, int z,
        int width, int height, int opacity) {

    /* Create drawable */
    guac_common_display_layer* layer =
        guac_common_display_alloc_layer(display->display, width, height);

    guac_drv_drawable* drawable = guac_drv_drawable_alloc(layer);

    guac_drv_drawable_move(drawable, x, y);
    guac_drv_drawable_stack(drawable, z);
    guac_drv_drawable_reparent(drawable, parent);
    guac_drv_drawable_shade(drawable, opacity);

    drawable->data = display;

    return drawable;

}

void guac_drv_display_destroy_layer(guac_drv_display* display,
        guac_drv_drawable* drawable) {

    /* Get underlying layer from drawable */
    guac_common_display_layer* layer = drawable->layer;
    guac_drv_drawable_free(drawable);

    /* Free layer */
    guac_common_display_free_layer(display->display, layer);

}

void guac_drv_display_touch(guac_drv_display* display) {

    pthread_mutex_t* mod_lock = &(display->modified_lock);
    pthread_cond_t* mod_cond = &(display->modified_cond);

    pthread_mutex_lock(mod_lock);

    /* Signal modification */
    display->modified = 1;
    pthread_cond_signal(mod_cond);

    pthread_mutex_unlock(mod_lock);

}

void guac_drv_display_flush(guac_drv_display* display) {

    /* Flush entire display */
    guac_common_display_flush(display->display);

    /* End frame */
    guac_client_end_frame(display->client);
    guac_socket_flush(display->client->socket);

}

