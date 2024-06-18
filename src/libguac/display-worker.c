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

#include "display-plan.h"
#include "display-priv.h"
#include "guacamole/client.h"
#include "guacamole/display.h"
#include "guacamole/fifo.h"
#include "guacamole/layer.h"
#include "guacamole/protocol.h"
#include "guacamole/rect.h"
#include "guacamole/rwlock.h"
#include "guacamole/socket.h"
#include "guacamole/timestamp.h"

#include <inttypes.h>
#include <limits.h>
#include <cairo/cairo.h>
#include <pthread.h>

/**
 * Sends the contents of the given dirty rectangle from the given layer using
 * lossless PNG compression. The resulting instructions will be sent over the
 * client-wide broadcast socket associated with the given layer. The graphical
 * contents sent will be pulled from the layer's last_frame buffer. If sending
 * the contents of a pending frame, that pending frame must have been copied
 * over to the last_frame buffer before calling this function.
 *
 * @param display_layer
 *     The layer whose data should be sent to connected users.
 *
 * @param dirty
 *     The region of the layer that should be sent.
 */
static void LFR_guac_display_layer_flush_to_png(guac_display_layer* display_layer,
        guac_rect* dirty) {

    guac_display* display = display_layer->display;
    guac_client* client = display->client;
    guac_socket* socket = client->socket;
    const guac_layer* layer = display_layer->layer;

    /* Get Cairo surface covering dirty rect */
    unsigned char* buffer = GUAC_DISPLAY_LAYER_STATE_MUTABLE_BUFFER(display_layer->last_frame, *dirty);
    cairo_surface_t* rect;

    /* Use RGB24 if the image is fully opaque */
    if (display_layer->opaque)
        rect = cairo_image_surface_create_for_data(buffer,
                CAIRO_FORMAT_RGB24, guac_rect_width(dirty),
                guac_rect_height(dirty), display_layer->last_frame.buffer_stride);

    /* Otherwise ARGB32 is needed, and the destination must be cleared */
    else {

        rect = cairo_image_surface_create_for_data(buffer,
                CAIRO_FORMAT_ARGB32, guac_rect_width(dirty),
                guac_rect_height(dirty), display_layer->last_frame.buffer_stride);

        /* Clear destination rect first */
        pthread_mutex_lock(&display->op_path_lock);
        guac_protocol_send_rect(socket, layer,
                dirty->left, dirty->top,
                guac_rect_width(dirty), guac_rect_height(dirty));
        guac_protocol_send_cfill(socket, GUAC_COMP_ROUT, layer,
                0x00, 0x00, 0x00, 0xFF);
        pthread_mutex_unlock(&display->op_path_lock);

    }

    /* Send PNG for rect */
    guac_client_stream_png(client, socket, GUAC_COMP_OVER,
            layer, dirty->left, dirty->top, rect);

    cairo_surface_destroy(rect);

}

/**
 * Returns an appropriate quality between 0 and 100 for lossy encoding
 * depending on the current processing lag calculated for the given client.
 *
 * @param client
 *     The client for which the lossy quality is being calculated.
 *
 * @return
 *     A value between 0 and 100 inclusive which seems appropriate for the
 *     client based on lag measurements.
 */
static int guac_display_suggest_quality(guac_client* client) {

    int lag = guac_client_get_processing_lag(client);

    /* Scale quality linearly from 90 to 30 as lag varies from 20ms to 80ms */
    int quality = 90 - (lag - 20);

    /* Do not exceed 90 for quality */
    if (quality > 90)
        return 90;

    /* Do not go below 30 for quality */
    if (quality < 30)
        return 30;

    return quality;

}

/**
 * Sends the contents of the given dirty rectangle from the given layer using
 * lossy JPEG compression. The resulting instructions will be sent over the
 * client-wide broadcast socket associated with the given layer. The graphical
 * contents sent will be pulled from the layer's last_frame buffer. If sending
 * the contents of a pending frame, that pending frame must have been copied
 * over to the last_frame buffer before calling this function.
 *
 * @param layer
 *     The layer whose data should be sent to connected users.
 *
 * @param dirty
 *     The region of the layer that should be sent.
 */
static void LFR_guac_display_layer_flush_to_jpeg(guac_display_layer* display_layer,
        guac_rect* dirty) {

    guac_display* display = display_layer->display;
    guac_client* client = display->client;
    guac_socket* socket = client->socket;
    const guac_layer* layer = display_layer->layer;

    guac_rect max = {
        .left   = 0,
        .top    = 0,
        .right  = display_layer->last_frame.width,
        .bottom = display_layer->last_frame.height
    };

    /* Expand the dirty rect size to fit in a grid with cells equal to the
     * minimum JPEG block size */
    guac_rect_align(dirty, GUAC_SURFACE_JPEG_BLOCK_SIZE);
    guac_rect_constrain(dirty, &max);

    /* Get Cairo surface covering dirty rect */
    unsigned char* buffer = GUAC_DISPLAY_LAYER_STATE_MUTABLE_BUFFER(display_layer->last_frame, *dirty);
    cairo_surface_t* rect = cairo_image_surface_create_for_data(buffer,
            CAIRO_FORMAT_RGB24, guac_rect_width(dirty),
            guac_rect_height(dirty), display_layer->last_frame.buffer_stride);

    /* Send JPEG for rect */
    guac_client_stream_jpeg(client, socket, GUAC_COMP_OVER, layer,
            dirty->left, dirty->top, rect,
            guac_display_suggest_quality(client));

    cairo_surface_destroy(rect);

}

/**
 * Sends the contents of the given dirty rectangle from the given layer using
 * WebP compression. Whether that WebP compression is lossless depends on the
 * lossless setting of the layer's last frame. The resulting instructions will
 * be sent over the client-wide broadcast socket associated with the given
 * layer. The graphical contents sent will be pulled from the layer's
 * last_frame buffer. If sending the contents of a pending frame, that pending
 * frame must have been copied over to the last_frame buffer before calling
 * this function.
 *
 * @param layer
 *     The layer whose data should be sent to connected users.
 *
 * @param dirty
 *     The region of the layer that should be sent.
 */
static void LFR_guac_display_layer_flush_to_webp(guac_display_layer* display_layer,
        guac_rect* dirty) {

    guac_display* display = display_layer->display;
    guac_client* client = display->client;
    guac_socket* socket = client->socket;
    const guac_layer* layer = display_layer->layer;

    guac_rect max = {
        .left   = 0,
        .top    = 0,
        .right  = display_layer->last_frame.width,
        .bottom = display_layer->last_frame.height
    };

    /* Expand the dirty rect size to fit in a grid with cells equal to the
     * minimum WebP block size */
    guac_rect_align(dirty, GUAC_SURFACE_WEBP_BLOCK_SIZE);
    guac_rect_constrain(dirty, &max);

    /* Get Cairo surface covering dirty rect */
    unsigned char* buffer = GUAC_DISPLAY_LAYER_STATE_MUTABLE_BUFFER(display_layer->last_frame, *dirty);
    cairo_surface_t* rect = cairo_image_surface_create_for_data(buffer,
            display_layer->opaque ? CAIRO_FORMAT_RGB24 : CAIRO_FORMAT_ARGB32,
            guac_rect_width(dirty), guac_rect_height(dirty),
            display_layer->last_frame.buffer_stride);

    /* Send WebP for rect */
    guac_client_stream_webp(client, socket, GUAC_COMP_OVER, layer,
            dirty->left, dirty->top, rect,
            guac_display_suggest_quality(client),
            display_layer->last_frame.lossless ? 1 : 0);

    cairo_surface_destroy(rect);

}

/**
 * Guesses whether a rectangle within a particular layer would be better
 * compressed as PNG or using a lossy format like JPEG. Positive values
 * indicate PNG is likely to be superior, while negative values indicate the
 * opposite.
 *
 * @param layer
 *     The layer containing the image data to check.
 *
 * @param rect
 *     The rect to check within the given layer.
 *
 * @return
 *     Positive values if PNG compression is likely to perform better than
 *     lossy alternatives, or negative values if PNG is likely to perform
 *     worse.
 */
static int LFR_guac_display_layer_png_optimality(guac_display_layer* layer,
        const guac_rect* rect) {

    int x, y;

    int num_same = 0;
    int num_different = 1;

    /* Get buffer from layer */
    size_t stride = layer->last_frame.buffer_stride;
    const unsigned char* buffer = GUAC_DISPLAY_LAYER_STATE_CONST_BUFFER(layer->last_frame, *rect);

    /* Image must be at least 1x1 */
    if (rect->right - rect->left < 1 || rect->bottom - rect->top< 1)
        return 0;

    /* For each row */
    for (y = rect->top; y < rect->bottom; y++) {

        uint32_t* row = (uint32_t*) buffer;
        uint32_t last_pixel = *(row++) | 0xFF000000;

        /* For each pixel in current row */
        for (x = rect->left + 1; x < rect->right; x++) {

            /* Get next pixel */
            uint32_t current_pixel = *(row++) | 0xFF000000;

            /* Update same/different counts according to pixel value */
            if (current_pixel == last_pixel)
                num_same++;
            else
                num_different++;

            last_pixel = current_pixel;

        }

        /* Advance to next row */
        buffer += stride;

    }

    /* Return rough approximation of optimality for PNG compression. As PNG
     * leverages lossless DEFLATE compression (which works by reducing the
     * number of bytes required to represent repeated data), an approximation
     * of the amount of repeated image data within the image is a reasonable
     * approximation for how well an image will compress. */
    return 0x100 * num_same / num_different - 0x400;

}

/**
 * Returns whether the given rectangle would be optimally encoded as JPEG
 * rather than PNG.
 *
 * @param layer
 *     The layer to be queried.
 *
 * @param rect
 *     The rectangle to check.
 *
 * @param framerate
 *     The rate that the region covered by the given rectangle has historically
 *     been being updated within the given layer, in frames per second.
 *
 * @return
 *     Non-zero if the rectangle would be optimally encoded as JPEG, zero
 *     otherwise.
 */
static int LFR_guac_display_layer_should_use_jpeg(guac_display_layer* layer,
        const guac_rect* rect, int framerate) {

    /* Do not use JPEG if lossless quality is required */
    if (layer->last_frame.lossless)
        return 0;

    int rect_width = rect->right - rect->left;
    int rect_height = rect->bottom - rect->top;
    int rect_size = rect_width * rect_height;

    /* JPEG is preferred if:
     * - frame rate is high enough
     * - image size is large enough
     * - PNG is not more optimal based on image contents */
    return framerate >= GUAC_DISPLAY_JPEG_FRAMERATE
        && rect_size > GUAC_DISPLAY_JPEG_MIN_BITMAP_SIZE
        && LFR_guac_display_layer_png_optimality(layer, rect) < 0;

}

/**
 * Returns whether the given rectangle would be optimally encoded as WebP
 * rather than PNG.
 *
 * @param layer
 *     The layer to be queried.
 *
 * @param rect
 *     The rectangle to check.
 *
 * @param framerate
 *     The rate that the region covered by the given rectangle has historically
 *     been being updated within the given layer, in frames per second.
 *
 * @return
 *     Non-zero if the rectangle would be optimally encoded as WebP, zero
 *     otherwise.
 */
static int LFR_guac_display_layer_should_use_webp(guac_display_layer* layer,
        const guac_rect* rect, int framerate) {

    /* Do not use WebP if not supported */
    if (!guac_client_supports_webp(layer->display->client))
        return 0;

    /* WebP is preferred if:
     * - frame rate is high enough
     * - PNG is not more optimal based on image contents */
    return framerate >= GUAC_DISPLAY_JPEG_FRAMERATE
        && LFR_guac_display_layer_png_optimality(layer, rect) < 0;

}

void* guac_display_worker_thread(void* data) {

    int framerate;
    int has_outstanding_frames = 0;

    guac_display* display = (guac_display*) data;
    guac_client* client = display->client;

    guac_display_plan_operation op;
    while (guac_fifo_dequeue_and_lock(&display->ops, &op)) {

        /* Notify any watchers of render_state that a frame is now in progress */
        guac_flag_set_and_lock(&display->render_state, GUAC_DISPLAY_RENDER_STATE_FRAME_IN_PROGRESS);
        guac_flag_clear(&display->render_state, GUAC_DISPLAY_RENDER_STATE_FRAME_NOT_IN_PROGRESS);
        guac_flag_unlock(&display->render_state);

        /* NOTE: Any thread that locks the operation queue can know that there
         * are no pending operations in progress if the queue is empty and
         * there are no active workers */
        display->active_workers++;
        guac_fifo_unlock(&display->ops);

        guac_rwlock_acquire_read_lock(&display->last_frame.lock);
        guac_display_layer* display_layer = op.layer;
        switch (op.type) {

            case GUAC_DISPLAY_PLAN_OPERATION_IMG:

                framerate = INT_MAX;
                if (op.current_frame > op.last_frame)
                    framerate = 1000 / (op.current_frame - op.last_frame);

                guac_rect* dirty = &op.dest;

                /* TODO: Determine whether to use PNG/WebP/JPEG purely
                 * based on whether lossless encoding is required, the
                 * expected time until another frame is received (time
                 * since last frame), and estimated encoding times. The
                 * time allowed per update should be divided up
                 * proportionately based on the dirty_size of the update. */

                /* TODO: Stream PNG/WebP/JPEG using progressive encoding such
                 * that a frame that is currently being encoded can be
                 * preempted by the next frame, with the connected client then
                 * simply receiving a lower-quality intermediate frame. If
                 * necessary, progressive encoding can be achieved by manually
                 * dividing images into multiple reduced-resolution stages,
                 * such that each image streamed is actually only one quarter
                 * the size of the original image. Compositing via Guacamole
                 * protocol instructions can reassemble those stages. */

                /* Prefer WebP when reasonable */
                if (LFR_guac_display_layer_should_use_webp(display_layer, dirty, framerate))
                    LFR_guac_display_layer_flush_to_webp(display_layer, dirty);

                /* If not WebP, JPEG is the next best (lossy) choice */
                else if (display_layer->opaque && LFR_guac_display_layer_should_use_jpeg(display_layer, dirty, framerate))
                    LFR_guac_display_layer_flush_to_jpeg(display_layer, dirty);

                /* Use PNG if no lossy formats are appropriate */
                else
                    LFR_guac_display_layer_flush_to_png(display_layer, dirty);

                break;

            case GUAC_DISPLAY_PLAN_OPERATION_COPY:
                guac_protocol_send_copy(client->socket, display_layer->last_frame_buffer,
                        op.src.rect.left, op.src.rect.top, guac_rect_width(&op.src.rect), guac_rect_height(&op.src.rect),
                        GUAC_COMP_OVER, display_layer->layer, op.dest.left, op.dest.top);
                break;

            case GUAC_DISPLAY_PLAN_OPERATION_RECT:

                pthread_mutex_lock(&display->op_path_lock);
                guac_protocol_send_rect(client->socket, display_layer->layer,
                        op.dest.left, op.dest.top, guac_rect_width(&op.dest), guac_rect_height(&op.dest));

                int alpha = (op.src.color & 0xFF000000) >> 24;
                int red   = (op.src.color & 0x00FF0000) >> 16;
                int green = (op.src.color & 0x0000FF00) >> 8;
                int blue  = (op.src.color & 0x000000FF);

                /* Clear before drawing if layer is not opaque (transparency
                 * will not be copied correctly otherwise) */
                if (!display_layer->opaque)
                    guac_protocol_send_cfill(client->socket, GUAC_COMP_ROUT, display_layer->layer,
                            0x00, 0x00, 0x00, 0xFF);

                guac_protocol_send_cfill(client->socket, GUAC_COMP_OVER, display_layer->layer,
                        red, green, blue, alpha);

                pthread_mutex_unlock(&display->op_path_lock);
                break;

            case GUAC_DISPLAY_PLAN_OPERATION_NOP:
                break;

            case GUAC_DISPLAY_PLAN_END_FRAME:

                guac_fifo_lock(&display->ops);
                int other_workers_busy = (display->active_workers > 1);
                guac_fifo_unlock(&display->ops);

                /* If other workers are still busy, push the frame boundary
                 * back on the queue so that it's picked up by one of those
                 * workers */
                if (other_workers_busy) {
                    guac_fifo_enqueue(&display->ops, &op);
                }

                /* Otherwise, we've reached the end of the frame, and this is
                 * the worker that will be sending that boundary to connected
                 * users */
                else {

                    /* Use the amount of time that the client has been waiting
                     * for a frame vs. the amount of time that it took the
                     * client to process the most recently acknowledged frame
                     * to calculate the amount of additional delay required to
                     * allow the client to catch up. This value is used later,
                     * after everything else related to the frame has been
                     * finalized. */
                    int time_since_last_frame = guac_timestamp_current() - client->last_sent_timestamp;
                    int processing_lag = guac_client_get_processing_lag(client);
                    int required_wait = processing_lag - time_since_last_frame;

                    /* Allow connected clients to move forward with rendering */
                    guac_client_end_multiple_frames(client, display->last_frame.frames);

                    /* Commit any changed contents to client-side backing
                     * buffer, while also determining whether any changes have
                     * been made to the mouse cursor graphic. */
                    int cursor_modified = 0;
                    guac_display_layer* cursor = display->cursor_buffer;
                    guac_display_layer* current = display->last_frame.layers;
                    while (current != NULL) {

                        /* Save a copy of the changed region if the layer has
                         * been modified since the last frame */
                        guac_rect* dirty = &current->last_frame.dirty;
                        if (!guac_rect_is_empty(dirty)) {

                            guac_protocol_send_copy(client->socket, current->layer,
                                    0, 0, current->last_frame.width, current->last_frame.height,
                                    GUAC_COMP_SRC, current->last_frame_buffer, 0, 0);

                            /* Additionally track whether the cursor is among
                             * the changed layers - we will later send a
                             * "cursor" instruction to actually update the
                             * remote cursor using the graphics from the cursor
                             * layer */
                            if (current == cursor)
                                cursor_modified = 1;

                        }

                        current = current->last_frame.next;

                    }

                    /* Update the mouse cursor if it's been changed since the
                     * last frame */
                    if (cursor_modified) {
                        guac_protocol_send_cursor(client->socket,
                                display->last_frame.cursor_hotspot_x,
                                display->last_frame.cursor_hotspot_y,
                                cursor->layer, 0, 0,
                                cursor->last_frame.width,
                                cursor->last_frame.height);
                    }

                    /* This is now absolutely everything for the current frame,
                     * and it's safe to flush any outstanding data */
                    guac_socket_flush(client->socket);

                    /* Notify any watchers of render_state that a frame is no longer in progress */
                    guac_flag_set_and_lock(&display->render_state, GUAC_DISPLAY_RENDER_STATE_FRAME_NOT_IN_PROGRESS);
                    guac_flag_clear(&display->render_state, GUAC_DISPLAY_RENDER_STATE_FRAME_IN_PROGRESS);
                    guac_flag_unlock(&display->render_state);

                    /* Exclude local, server-side frame processing latency from
                     * waiting period */
                    int latency = (int) (guac_timestamp_current() - display->last_frame.timestamp);
                    if (latency >= 0) {
                        guac_client_log(display->client, GUAC_LOG_TRACE,
                                "Rendering latency: %ims (%i:1 frame)\n",
                                latency, display->last_frame.frames);
                        required_wait -= latency;
                    }

                    /* Ensure we don't wait without bound when compensating for
                     * client-side processing delays */
                    if (required_wait > GUAC_DISPLAY_MAX_LAG_COMPENSATION)
                        required_wait = GUAC_DISPLAY_MAX_LAG_COMPENSATION;

                    /* Allow connected clients to catch up if they're taking
                     * longer to process frames than the server is taking to
                     * generate them */
                    if (required_wait > 0) {
                        guac_client_log(display->client, GUAC_LOG_TRACE,
                                "Waiting %ims to compensate for client-side "
                                "processing delays.\n", required_wait);
                        guac_timestamp_msleep(required_wait);
                    }

                    guac_fifo_lock(&display->ops);
                    has_outstanding_frames = display->frame_deferred;
                    guac_fifo_unlock(&display->ops);

                }

                break;

        }

        guac_rwlock_release_lock(&display->last_frame.lock);

        guac_fifo_lock(&display->ops);
        display->active_workers--;
        guac_fifo_unlock(&display->ops);

        /* Trigger additional flush if frames were completed while we were
         * still processing the previous frame */
        if (has_outstanding_frames) {
            guac_display_end_multiple_frames(display, 0);
            has_outstanding_frames = 0;
        }

    }

    return NULL;

}
