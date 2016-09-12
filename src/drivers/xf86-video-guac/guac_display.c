
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
#include "daemon.h"
#include "default_pointer.h"
#include "guac_display.h"
#include "guac_drawable.h"
#include "guac_user.h"
#include "list.h"
#include "log.h"

#include <xf86.h>
#include <xf86str.h>
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

void* guac_drv_render_thread(void* arg) {

    int frame_timeout_sec  =  GUAC_DRV_FRAME_TIMEOUT / 1000;
    int frame_timeout_usec = (GUAC_DRV_FRAME_TIMEOUT % 1000) * 1000;

    int sync_sec  =  GUAC_DRV_SYNC_INTERVAL / 1000;
    int sync_usec = (GUAC_DRV_SYNC_INTERVAL % 1000) * 1000;

    struct timespec timeout;

    /* Get guac_drv_display */
    guac_drv_display* display = (guac_drv_display*) arg;
    pthread_mutex_t* mod_lock = &(display->modified_lock);
    pthread_cond_t* mod_cond = &(display->modified);

    pthread_mutex_lock(mod_lock);

    for (;;) {

        /* Construct timeout until sync */
        _get_absolute_time(&timeout, sync_sec, sync_usec);

        /* Wait indefinitely for display to be modified */
        if (pthread_cond_timedwait(mod_cond, mod_lock, &timeout)
                != ETIMEDOUT) {

            /* Get frame start */
            guac_timestamp start = guac_timestamp_current();

            /* Continue until lag is reasonable and timeout or frame duration exceeded */
            int lag;
            do {

                /* Compute lag */
                lag = guac_client_get_processing_lag(display->client);

                /* Construct timeout until frame end */
                _get_absolute_time(&timeout, frame_timeout_sec, frame_timeout_usec);

                if (pthread_cond_timedwait(mod_cond, mod_lock, &timeout) == ETIMEDOUT
                        && lag < GUAC_DRV_MAX_LAG)
                        break;

            } while (guac_timestamp_current() - start < GUAC_DRV_FRAME_MAX_DURATION
                        || lag >= GUAC_DRV_MAX_LAG);

            /* End frame */
            guac_drv_display_flush(display);
            guac_drv_log(GUAC_LOG_INFO, "FRAME");

        } /* end if display modified in time */

        else {
            guac_drv_log(GUAC_LOG_INFO, "NOP");
            guac_protocol_send_nop(display->client->socket);
            guac_socket_flush(display->client->socket);
        }

    }

}

guac_drv_display* guac_drv_display_alloc() {

    guac_drv_display* display = malloc(sizeof(guac_drv_display));

    /* Init underlying client */
    guac_client* client = guac_client_alloc();
    client->join_handler = guac_drv_user_join_handler;
    client->log_handler = guac_drv_client_log;
    client->data = display;

    display->display = guac_common_display_alloc(client, 1024, 768);
    display->client = client;

    /* Init drawables */
    display->drawables = guac_drv_list_alloc();

    /* Init watchdog condition */
    if (pthread_cond_init(&(display->modified), NULL)) {
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

guac_drv_drawable* guac_drv_display_create_layer(guac_drv_display* display,
        guac_drv_drawable* parent, int x, int y, int z,
        int width, int height, int opacity) {

    guac_drv_list_element* drawable_element;
    guac_drv_list_lock(display->drawables);

    /* Create drawable */
    guac_common_display_layer* layer =
        guac_common_display_alloc_layer(display->display, width, height);

    guac_drv_drawable* drawable = guac_drv_drawable_alloc(layer);
    guac_drv_drawable_stub(drawable, 0, 0, width, height);

    guac_drv_drawable_move(drawable, x, y);
    guac_drv_drawable_stack(drawable, z);
    guac_drv_drawable_reparent(drawable, parent);
    guac_drv_drawable_shade(drawable, opacity);

    /* Add to list */
    drawable_element = guac_drv_list_add(display->drawables, drawable);
    drawable->data = drawable_element;

    guac_drv_list_unlock(display->drawables);
    return drawable;

}

guac_drv_drawable* guac_drv_display_create_buffer(guac_drv_display* display,
        int width, int height) {

    guac_drv_list_element* drawable_element;
    guac_drv_list_lock(display->drawables);

    /* Create drawable */
    guac_common_display_layer* buffer =
        guac_common_display_alloc_buffer(display->display, width, height);

    guac_drv_drawable* drawable = guac_drv_drawable_alloc(buffer);
    guac_drv_drawable_stub(drawable, 0, 0, width, height);

    /* Add to list */
    drawable_element = guac_drv_list_add(display->drawables, drawable);
    drawable->data = drawable_element;

    guac_drv_list_unlock(display->drawables);
    return drawable;

}

void guac_drv_display_touch(guac_drv_display* display) {
    pthread_cond_signal(&(display->modified));
}

void guac_drv_display_flush(guac_drv_display* display) {

    guac_drv_list_lock(display->drawables);

    /* Flush entire display */
    guac_common_display_flush(display->display);

    /* End frame */
    guac_client_end_frame(display->client);
    guac_socket_flush(display->client->socket);

    guac_drv_list_unlock(display->drawables);

}

