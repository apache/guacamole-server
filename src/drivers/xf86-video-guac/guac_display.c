
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
#include "guac_protocol.h"
#include "guac_user.h"
#include "list.h"
#include "log.h"

#include <xf86.h>
#include <xf86str.h>
#include <guacamole/client.h>
#include <guacamole/pool.h>
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

        } /* end if display modified in time */

        /* End frame */
        guac_drv_display_flush(display);

    }

}

guac_drv_display* guac_drv_display_alloc() {

    guac_drv_display* display = malloc(sizeof(guac_drv_display));

    /* Init underlying client */
    guac_client* client = guac_client_alloc();
    client->join_handler = guac_drv_user_join_handler;
    client->log_handler = guac_drv_client_log;
    client->data = display;
    display->client = client;

    /* Init drawables */
    display->drawables= guac_drv_list_alloc();
    display->layer_pool = guac_pool_alloc(0);
    display->buffer_pool = guac_pool_alloc(0);

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

    /* Create drawable */
    guac_drv_drawable* drawable = guac_drv_drawable_alloc(
            GUAC_DRV_DRAWABLE_LAYER, parent, x, y, z,
            width, height,
            opacity, 1);

    /* Add to list */
    guac_drv_list_lock(display->drawables);
    drawable_element = guac_drv_list_add(display->drawables, drawable);
    drawable->data = drawable_element;
    guac_drv_list_unlock(display->drawables);

    return drawable;

}

guac_drv_drawable* guac_drv_display_create_buffer(guac_drv_display* display,
        int width, int height) {

    guac_drv_list_element* drawable_element;

    /* Create drawable */
    guac_drv_drawable* drawable = guac_drv_drawable_alloc(
            GUAC_DRV_DRAWABLE_BUFFER, NULL, 0, 0, 0,
            width, height,
            0xFF, 1);

    /* Add to list */
    guac_drv_list_lock(display->drawables);
    drawable_element = guac_drv_list_add(display->drawables, drawable);
    drawable->data = drawable_element;
    guac_drv_list_unlock(display->drawables);

    return drawable;

}

void guac_drv_display_realize_drawable(guac_drv_display* display,
        guac_drv_drawable* drawable) {

    switch (drawable->type) {

        /* Layers */
        case GUAC_DRV_DRAWABLE_LAYER:

            if (drawable->pending.parent != NULL)
                drawable->index = guac_pool_next_int(display->layer_pool)+1;
            else
                drawable->index = 0;

            drawable->realized = 1;
            break;

        /* Buffers */
        case GUAC_DRV_DRAWABLE_BUFFER:
            drawable->index = -1 - guac_pool_next_int(display->buffer_pool);
            drawable->realized = 1;
            break;

    }

}

void guac_drv_display_unrealize_drawable(guac_drv_display* display,
        guac_drv_drawable* drawable) {

    switch (drawable->type) {

        /* Layers */
        case GUAC_DRV_DRAWABLE_LAYER:
            if (drawable->realized && drawable->index != 0)
                guac_pool_free_int(display->layer_pool, drawable->index-1);
            break;

        /* Buffers */
        case GUAC_DRV_DRAWABLE_BUFFER:
            if (drawable->realized)
                guac_pool_free_int(display->buffer_pool, -1 - drawable->index);
            break;

    }

}

void guac_drv_display_touch(guac_drv_display* display) {
    pthread_cond_signal(&(display->modified));
}

void guac_drv_display_flush(guac_drv_display* display) {

    guac_drv_list_element* current;

    guac_drv_list_lock(display->drawables);

    /* Realize all new drawables prior to flush */
    current = display->drawables->head;
    while (current != NULL) {

        guac_drv_drawable* drawable = (guac_drv_drawable*) current->data;
        if (drawable->sync_state == GUAC_DRV_DRAWABLE_NEW)
            guac_drv_display_realize_drawable(display, drawable);

        current = current->next;

    }

    /* For each drawable */
    current = display->drawables->head;
    while (current != NULL) {

        guac_drv_list_element* next = current->next;

        /* Flush drawable on the client */
        guac_drv_drawable* drawable = (guac_drv_drawable*) current->data;
        guac_drv_display_flush_drawable(display, drawable);

        current = next;

    }

    /* End frame */
    guac_drv_client_end_frame(display->client);

    guac_drv_list_unlock(display->drawables);

}

/**
 * Flush all contiguous, pending COPY operations which begin at the upper-left
 * corner of the given rectangle.
 */
static void guac_drv_display_flush_collect_copy(guac_drv_display* display,
        guac_drv_drawable* drawable, int sx, int sy, int w, int h,
        guac_drv_display_copy_operation* copy) {

    int x, y;
    unsigned char* row = (unsigned char*) drawable->operations
                       + sy*drawable->operations_stride
                       + sx*sizeof(guac_drv_drawable_operation);

    unsigned char* output_row = drawable->image_data
                              + sy*drawable->image_stride
                              + sx*4;

    guac_drv_drawable_operation* first_op = (guac_drv_drawable_operation*) row;

    /* Get expected source */
    guac_drv_drawable* expected_source = first_op->source;

    /* Calculate expected start and end coordinates */
    int expected_sx = first_op->x;
    int expected_sy = first_op->y;
    int expected_ex = expected_sx + w;
    int expected_ey = expected_sy + h;

    int width = -1;
    int height = 0;

    /* Flush each COPY operation as a pixel */
    for (y=expected_sy; y!=expected_ey; y++) {

        guac_drv_drawable_operation* current = (guac_drv_drawable_operation*) row;
        uint32_t* output_pixel;
        int row_width;

        /* Calculate width of row */
        row_width = 0;
        for (x=expected_sx; x!=expected_ex; x++) {
            if (   current->type != GUAC_DRV_DRAWABLE_COPY
                || current->x != x
                || current->y != y
                || current->source != expected_source)
                break;
            row_width++;
            current++;
        }

        /* If width not yet known, copy it */
        if (width == -1)
            width = row_width;
        
        /* If row width less than expected width, we are past the bottom */
        else if (row_width < width)
            break;

        /* Flush row */
        current = (guac_drv_drawable_operation*) row;
        output_pixel = (uint32_t*) output_row;
        for (x=0; x<width; x++) {

            *output_pixel = current->color;
            current->old_color = current->color;
            current->type = GUAC_DRV_DRAWABLE_NOP;

            output_pixel++;
            current++;
        }

        /* Next row */
        height++;
        row += drawable->operations_stride;
        output_row += drawable->image_stride;

    }

    /* Save update */
    if (height > 0) {
        copy->order = first_op->order;
        copy->source = expected_source;
        copy->dx = sx;
        copy->dy = sy;
        guac_drv_rect_init(&(copy->source_rect),
                expected_sx, expected_sy, width, height);
    }

}

static void guac_drv_display_flush_copy(guac_drv_display* display,
        guac_drv_drawable* drawable, int sx, int sy, int w, int h) {

    int i;
    int x, y;
    unsigned char* row = (unsigned char*) drawable->operations
                       + sy*drawable->operations_stride
                       + sx*sizeof(guac_drv_drawable_operation);

    guac_drv_display_copy_operation updates[GUAC_DRV_MAX_QUEUE];
    int update_count = 0;

    /* Flush each COPY operation */
    for (y=0; y<h; y++) {

        guac_drv_drawable_operation* current = (guac_drv_drawable_operation*) row;
        for (x=0; x<w; x++) {

            /* If COPY operation, flush as greedy rectangle */
            if (current->type == GUAC_DRV_DRAWABLE_COPY
                    && update_count < GUAC_DRV_MAX_QUEUE)
                guac_drv_display_flush_collect_copy(display, drawable,
                        x, y, w-x, h-y, &(updates[update_count++]));

            /* Next operation */
            current++;

        }

        row += drawable->operations_stride;

    }

    /* Sort queue */
    qsort(updates, update_count, sizeof(guac_drv_display_copy_operation),
            guac_drv_display_copy_operation_compare);

    /* Write all updates in queue */
    for (i=0; i<update_count; i++) {

        guac_drv_display_copy_operation* update = &(updates[i]);

        guac_drv_send_copy(display->client->socket,
                update->source,
                update->source_rect.x, update->source_rect.y,
                update->source_rect.width, update->source_rect.height,
                drawable, update->dx, update->dy);

    }

}

/**
 * Flush all contiguous, pending SET operations which begin at the upper-left
 * corner of the given rectangle.
 */
static void guac_drv_display_flush_collect_set(guac_drv_display* display,
        guac_drv_drawable* drawable, int sx, int sy, int w, int h,
        guac_drv_rect* update) {

    int x, y;
    unsigned char* row = (unsigned char*) drawable->operations
                       + sy*drawable->operations_stride
                       + sx*sizeof(guac_drv_drawable_operation);

    unsigned char* output_row = drawable->image_data
                              + sy*drawable->image_stride
                              + sx*4;

    int width = -1;
    int height = 0;

    /* Flush each SET operation as a pixel */
    for (y=0; y<h; y++) {

        guac_drv_drawable_operation* current;
        uint32_t* output_pixel;

        /* Calculate width of row */
        int row_width = 0;
        current = (guac_drv_drawable_operation*) row;
        for (x=0; x<w; x++) {
            if (current->type != GUAC_DRV_DRAWABLE_SET)
                break;
            row_width++;
            current++;
        }

        /* If width not yet known, set it */
        if (width == -1)
            width = row_width;
        
        /* If row width less than expected width, we are past the bottom */
        else if (row_width < width)
            break;

        /* Flush row */
        current = (guac_drv_drawable_operation*) row;
        output_pixel = (uint32_t*) output_row;
        for (x=0; x<width; x++) {

            *output_pixel = current->color;
            current->old_color = current->color;
            current->type = GUAC_DRV_DRAWABLE_NOP;

            output_pixel++;
            current++;
        }

        /* Next row */
        height++;
        row += drawable->operations_stride;
        output_row += drawable->image_stride;

    }

    /* Save update rect */
    if (height > 0)
        guac_drv_rect_init(update, sx, sy, width, height);
    else
        guac_drv_rect_clear(update);

}

static int __guac_drv_rect_cost(guac_drv_rect* rect) {
    return rect->width * rect->height + 256;
}

static void guac_drv_display_flush_set(guac_drv_display* display,
        guac_drv_drawable* drawable, int sx, int sy, int w, int h) {

    int i;
    int x, y;
    unsigned char* row = (unsigned char*) drawable->operations
                       + sy*drawable->operations_stride
                       + sx*sizeof(guac_drv_drawable_operation);

    guac_drv_rect updates[GUAC_DRV_MAX_QUEUE];
    int update_count = 0;

    /* Flush each SET operation */
    for (y=0; y<h; y++) {

        guac_drv_drawable_operation* current = (guac_drv_drawable_operation*) row;
        for (x=0; x<w; x++) {

            /* If SET operation, flush as greedy rectangle */
            if (current->type == GUAC_DRV_DRAWABLE_SET) {

                guac_drv_rect update;
                guac_drv_display_flush_collect_set(display, drawable,
                        x, y, w-x, h-y, &update);

                /* If enough space in queue, store update */
                if (update_count < GUAC_DRV_MAX_QUEUE) {
                    updates[update_count] = update;
                    update_count++;
                }

                /* Otherwise, flush now */
                else {
                    xf86Msg(X_INFO, "guac: condense: too many\n");
                    guac_drv_client_draw(display->client, drawable,
                        update.x, update.y, update.width, update.height);
                }

            }

            /* Next operation */
            current++;

        }

        row += drawable->operations_stride;

    }

    /* Send updates, combining if possible */
    for (i=0; i<update_count; i++) {

        /* If update is valid, attempt to combine with future updates */
        guac_drv_rect* current = &(updates[i]);
        if (current->width > 0 && current->height > 0) {

            int j;
            int combined = 0;
            int cost = current->width * current->height + 256;

            /* Compare cost of all future updates */
            for (j=i+1; j<update_count; j++) {

                guac_drv_rect* compare = &(updates[j]);

                /* Combine for sake of testing */
                guac_drv_rect extended = *compare;
                guac_drv_rect_extend(&extended, current);

                /* If combined cost is less, combine */
                if (__guac_drv_rect_cost(&extended) <=
                        cost +__guac_drv_rect_cost(compare)) {

                    *compare = extended;
                    combined = 1;
                    break;

                }

            } /* end for each future update */

            /* If unable to combine with anything, send now */
            if (!combined)
                guac_drv_client_draw(display->client, drawable,
                    current->x, current->y, current->width, current->height);

        }

    } /* end for each update */

}

void guac_drv_display_flush_drawable(guac_drv_display* display,
        guac_drv_drawable* drawable) {

    guac_drv_drawable_lock(drawable);

    /* If new, create on all clients */
    if (drawable->sync_state == GUAC_DRV_DRAWABLE_NEW) {

        guac_drv_send_create_drawable(display->client->socket, drawable);
        drawable->sync_state = GUAC_DRV_DRAWABLE_SYNCED;

        /* Flush draw operations */
        if (drawable->dirty.width > 0 && drawable->dirty.height > 0) {
            guac_drv_display_flush_set(display, drawable,
                    drawable->dirty.x, drawable->dirty.y,
                    drawable->dirty.width, drawable->dirty.height);
        }

        /* Drawable flushed */
        guac_drv_rect_clear(&drawable->dirty);
        drawable->current = drawable->pending;

    }

    /* If destroyed, destroy on all clients */
    else if (drawable->sync_state == GUAC_DRV_DRAWABLE_DESTROYED) {
        guac_drv_send_destroy_drawable(display->client->socket, drawable);
        guac_drv_display_unrealize_drawable(display, drawable);
        guac_drv_list_remove(display->drawables,
                (guac_drv_list_element*) drawable->data);
        guac_drv_drawable_free(drawable);
    }

    /* If synced, update any changes from last flush */
    else if (drawable->sync_state == GUAC_DRV_DRAWABLE_SYNCED) {

        /* Update change in location */
        if (   drawable->pending.rect.x != drawable->current.rect.x
            || drawable->pending.rect.y != drawable->current.rect.y
            || drawable->pending.z      != drawable->current.z
            || drawable->pending.parent != drawable->current.parent)
            guac_drv_send_move_drawable(display->client->socket, drawable);

        /* Update change in size */
        if (   drawable->pending.rect.width  != drawable->current.rect.width
            || drawable->pending.rect.height != drawable->current.rect.height)
            guac_drv_send_resize_drawable(display->client->socket, drawable);

        /* Update change in opacity*/
        if (drawable->pending.opacity != drawable->current.opacity)
            guac_drv_send_shade_drawable(display->client->socket, drawable);

        /* Flush draw operations */
        if (drawable->dirty.width > 0 && drawable->dirty.height > 0) {

            guac_drv_display_flush_copy(display, drawable,
                    drawable->dirty.x, drawable->dirty.y,
                    drawable->dirty.width, drawable->dirty.height);

            guac_drv_display_flush_set(display, drawable,
                    drawable->dirty.x, drawable->dirty.y,
                    drawable->dirty.width, drawable->dirty.height);

        }

        /* Drawable flushed */
        guac_drv_rect_clear(&drawable->dirty);
        drawable->current = drawable->pending;

    }

    /* Operations now cleared */
    drawable->operations_pending = 0;

    guac_drv_drawable_unlock(drawable);

}

int guac_drv_display_copy_operation_compare(const void* a, const void* b) {

    guac_drv_display_copy_operation* a_op =
        (guac_drv_display_copy_operation*) a;

    guac_drv_display_copy_operation* b_op =
        (guac_drv_display_copy_operation*) b;

    /* Sort by draw order */
    return a_op->order - b_op->order;

}

