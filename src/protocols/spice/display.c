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

#include "display.h"
#include "spice.h"

#include <guacamole/client.h>
#include <guacamole/display.h>
#include <guacamole/rect.h>
#include <spice-client.h>

#include <pthread.h>
#include <string.h>

/**
 * Signal handler for the SPICE display channel "display-primary-create"
 * signal. Records the location and dimensions of the new primary surface and
 * resizes the Guacamole display accordingly.
 */
static void guac_spice_display_primary_create(SpiceChannel* channel,
        gint format, gint width, gint height, gint stride, gint shmid,
        gpointer imgdata, gpointer data) {

    guac_client* client = (guac_client*) data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    pthread_mutex_lock(&spice_client->surface_lock);

    /* Record details of the new primary surface */
    spice_client->surface_data = imgdata;
    spice_client->surface_format = format;
    spice_client->surface_width = width;
    spice_client->surface_height = height;
    spice_client->surface_stride = stride;

    pthread_mutex_unlock(&spice_client->surface_lock);

    guac_client_log(client, GUAC_LOG_DEBUG,
            "SPICE primary surface created: %dx%d (stride %d, format %d).",
            width, height, stride, format);

    /* Resize the Guacamole display to match the new primary surface */
    if (spice_client->display != NULL)
        guac_display_layer_resize(
                guac_display_default_layer(spice_client->display),
                width, height);

}

/**
 * Signal handler for the SPICE display channel "display-primary-destroy"
 * signal. Clears the recorded primary surface, preventing any further reads
 * from the (now invalid) surface buffer.
 */
static void guac_spice_display_primary_destroy(SpiceChannel* channel,
        gpointer data) {

    guac_client* client = (guac_client*) data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    pthread_mutex_lock(&spice_client->surface_lock);
    spice_client->surface_data = NULL;
    pthread_mutex_unlock(&spice_client->surface_lock);

}

/**
 * Signal handler for the SPICE display channel "display-invalidate" signal.
 * Copies the damaged region of the primary surface into the Guacamole display.
 */
static void guac_spice_display_invalidate(SpiceChannel* channel,
        gint x, gint y, gint w, gint h, gpointer data) {

    guac_client* client = (guac_client*) data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    if (spice_client->display == NULL)
        return;

    pthread_mutex_lock(&spice_client->surface_lock);

    /* Ignore updates received while no primary surface exists */
    const unsigned char* surface = (const unsigned char*) spice_client->surface_data;
    if (surface == NULL) {
        pthread_mutex_unlock(&spice_client->surface_lock);
        return;
    }

    int surface_stride = spice_client->surface_stride;
    int surface_width = spice_client->surface_width;
    int surface_height = spice_client->surface_height;

    guac_display_layer* default_layer =
            guac_display_default_layer(spice_client->display);

    /* Acquire exclusive access to the layer for drawing */
    guac_display_layer_raw_context* context =
            guac_display_layer_open_raw(default_layer);

    /* Constrain the damaged region to the bounds of both the SPICE surface
     * and the current pending frame */
    guac_rect op_bounds;
    guac_rect_init(&op_bounds, x, y, w, h);

    guac_rect surface_bounds;
    guac_rect_init(&surface_bounds, 0, 0, surface_width, surface_height);
    guac_rect_constrain(&op_bounds, &surface_bounds);
    guac_rect_constrain(&op_bounds, &context->bounds);

    int dst_x = guac_rect_width(&op_bounds) > 0 ? op_bounds.left : 0;
    int dst_y = op_bounds.top;
    int width = guac_rect_width(&op_bounds);
    int height = guac_rect_height(&op_bounds);

    /* The SPICE primary surface (SPICE_SURFACE_FMT_32_xRGB) shares the same
     * little-endian 32-bit memory layout as the Guacamole raw layer buffer, so
     * each damaged row can be copied directly. */
    if (width > 0 && height > 0) {

        unsigned char* dst_row = GUAC_RECT_MUTABLE_BUFFER(op_bounds,
                context->buffer, context->stride, GUAC_DISPLAY_LAYER_RAW_BPP);

        const unsigned char* src_row = surface
                + (size_t) dst_y * surface_stride
                + (size_t) dst_x * GUAC_DISPLAY_LAYER_RAW_BPP;

        size_t row_length = (size_t) width * GUAC_DISPLAY_LAYER_RAW_BPP;

        /* Swap the red and blue channels per-pixel if requested (for the rare
         * server which reports BGR instead of RGB); otherwise copy each row
         * directly, as the formats are byte-compatible. */
        if (spice_client->settings->swap_red_blue) {
            for (int row = 0; row < height; row++) {
                for (int col = 0; col < width; col++) {
                    const unsigned char* sp = src_row + (size_t) col * GUAC_DISPLAY_LAYER_RAW_BPP;
                    unsigned char* dp = dst_row + (size_t) col * GUAC_DISPLAY_LAYER_RAW_BPP;
                    dp[0] = sp[2];
                    dp[1] = sp[1];
                    dp[2] = sp[0];
                    dp[3] = sp[3];
                }
                dst_row += context->stride;
                src_row += surface_stride;
            }
        }
        else {
            for (int row = 0; row < height; row++) {
                memcpy(dst_row, src_row, row_length);
                dst_row += context->stride;
                src_row += surface_stride;
            }
        }

        /* Mark the modified region as dirty */
        guac_rect_extend(&context->dirty, &op_bounds);

    }

    pthread_mutex_unlock(&spice_client->surface_lock);

    /* Drawing is complete */
    guac_display_layer_close_raw(default_layer, context);
    guac_display_render_thread_notify_modified(spice_client->render_thread);

}

void guac_spice_display_channel_connect(guac_client* client,
        SpiceChannel* channel) {

    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    spice_client->display_channel = channel;

    /* Mirror the remote framebuffer into the Guacamole display */
    g_signal_connect(channel, "display-primary-create",
            G_CALLBACK(guac_spice_display_primary_create), client);
    g_signal_connect(channel, "display-primary-destroy",
            G_CALLBACK(guac_spice_display_primary_destroy), client);
    g_signal_connect(channel, "display-invalidate",
            G_CALLBACK(guac_spice_display_invalidate), client);

}
