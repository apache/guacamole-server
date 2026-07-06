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

#include "cursor.h"
#include "spice.h"

#include <guacamole/client.h>
#include <guacamole/display.h>
#include <guacamole/rect.h>
#include <spice-client.h>

#include <stdint.h>

/**
 * Signal handler for the SPICE cursor channel "cursor-set" signal. Renders the
 * given cursor shape into the cursor layer of the Guacamole display.
 *
 * The SPICE cursor image data is provided as 32 bits per pixel in R, G, B, A
 * byte order (as consumed by GdkPixbuf within spice-gtk), which is converted
 * here to the native-endian 0xAARRGGBB layout used by the Guacamole display.
 */
static void guac_spice_cursor_set(SpiceCursorChannel* channel,
        gint width, gint height, gint hot_x, gint hot_y, gpointer rgba,
        gpointer data) {

    guac_client* client = (guac_client*) data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    if (spice_client->display == NULL || rgba == NULL)
        return;

    /* Ignore implausible cursor geometry supplied by the (untrusted) SPICE
     * server. The cursor is a buffer layer, whose resize path does not clamp to
     * GUAC_DISPLAY_MAX_*, so an oversized width/height would drive a huge
     * allocation (CWE-400/CWE-789). */
    if (width <= 0 || height <= 0
            || width > GUAC_DISPLAY_MAX_WIDTH
            || height > GUAC_DISPLAY_MAX_HEIGHT) {
        guac_client_log(client, GUAC_LOG_DEBUG,
                "Ignoring cursor with implausible dimensions %dx%d.",
                width, height);
        return;
    }

    /* Begin drawing operation directly to the cursor layer */
    guac_display_layer* cursor_layer = guac_display_cursor(spice_client->display);
    guac_display_layer_resize(cursor_layer, width, height);
    guac_display_set_cursor_hotspot(spice_client->display, hot_x, hot_y);

    guac_display_layer_raw_context* context =
            guac_display_layer_open_raw(cursor_layer);

    guac_rect op_bounds;
    guac_rect_init(&op_bounds, 0, 0, width, height);
    guac_rect_constrain(&op_bounds, &context->bounds);

    const unsigned char* src_row = (const unsigned char*) rgba;
    unsigned char* dst_row = GUAC_RECT_MUTABLE_BUFFER(op_bounds,
            context->buffer, context->stride, GUAC_DISPLAY_LAYER_RAW_BPP);

    for (int dy = 0; dy < height; dy++) {

        const unsigned char* src_pixel = src_row;
        uint32_t* dst_pixel = (uint32_t*) dst_row;

        for (int dx = 0; dx < width; dx++) {

            uint8_t red   = src_pixel[0];
            uint8_t green = src_pixel[1];
            uint8_t blue  = src_pixel[2];
            uint8_t alpha = src_pixel[3];

            *(dst_pixel++) = ((uint32_t) alpha << 24)
                           | ((uint32_t) red   << 16)
                           | ((uint32_t) green << 8)
                           |  (uint32_t) blue;

            src_pixel += 4;

        }

        src_row += (size_t) width * 4;
        dst_row += context->stride;

    }

    /* Mark entire cursor as modified */
    guac_rect_extend(&context->dirty, &op_bounds);

    guac_display_layer_close_raw(cursor_layer, context);
    guac_display_render_thread_notify_modified(spice_client->render_thread);

}

/**
 * Signal handler for the SPICE cursor channel "cursor-hide" signal. Hides the
 * Guacamole cursor by switching to an empty (fully transparent) cursor.
 */
static void guac_spice_cursor_hide(SpiceCursorChannel* channel,
        gpointer data) {

    guac_client* client = (guac_client*) data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    if (spice_client->display != NULL)
        guac_display_set_cursor(spice_client->display, GUAC_DISPLAY_CURSOR_NONE);

}

/**
 * Signal handler for the SPICE cursor channel "cursor-reset" signal. Restores
 * the default Guacamole cursor.
 */
static void guac_spice_cursor_reset(SpiceCursorChannel* channel,
        gpointer data) {

    guac_client* client = (guac_client*) data;
    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    if (spice_client->display != NULL)
        guac_display_set_cursor(spice_client->display, GUAC_DISPLAY_CURSOR_POINTER);

}

void guac_spice_cursor_channel_connect(guac_client* client,
        SpiceChannel* channel) {

    guac_spice_client* spice_client = (guac_spice_client*) client->data;
    spice_client->cursor_channel = channel;

    g_signal_connect(channel, "cursor-set",
            G_CALLBACK(guac_spice_cursor_set), client);
    g_signal_connect(channel, "cursor-hide",
            G_CALLBACK(guac_spice_cursor_hide), client);
    g_signal_connect(channel, "cursor-reset",
            G_CALLBACK(guac_spice_cursor_reset), client);

}
