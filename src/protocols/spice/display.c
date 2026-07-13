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
#include "input.h"
#include "spice.h"

#include <guacamole/client.h>
#include <guacamole/display.h>
#include <guacamole/protocol.h>
#include <guacamole/protocol-constants.h>
#include <guacamole/rect.h>
#include <guacamole/socket.h>
#include <spice-client.h>

#include <pthread.h>
#include <stdio.h>
#include <string.h>

/**
 * Appends a single monitor entry to the "multimon-layout" JSON object being
 * built in the given buffer, e.g. {@code "0":{"left":0,"top":0,"width":1024,
 * "height":768}}. Every value is an integer, so no escaping is required.
 *
 * @param json
 *     The buffer holding the JSON object under construction.
 *
 * @param pos
 *     The current write offset within json (the index of the next free byte).
 *
 * @param size
 *     The total size of json, in bytes.
 *
 * @param written
 *     The number of entries already appended, used to decide whether a leading
 *     comma separator is required.
 *
 * @param index
 *     The monitor index to use as the JSON key.
 *
 * @param left
 * @param top
 * @param width
 * @param height
 *     The position and size of the monitor within the combined framebuffer.
 *
 * @return
 *     The new write offset within json, or a negative value if the entry would
 *     not fit (in which case json is left unmodified past pos).
 */
static int guac_spice_layout_append(char* json, int pos, int size,
        int written, int index, int left, int top, int width, int height) {

    int remaining = size - pos;
    int length = snprintf(json + pos, remaining,
            "%s\"%d\":{\"left\":%d,\"top\":%d,\"width\":%d,\"height\":%d}",
            (written ? "," : ""), index, left, top, width, height);

    if (length < 0 || length >= remaining)
        return -1;

    return pos + length;

}

/**
 * Sends the current monitor layout to the connected client as a JSON
 * "multimon-layout" parameter on the default layer, allowing a multi-monitor
 * client to split the combined framebuffer into per-monitor windows. Does
 * nothing unless multi-monitor support is enabled for this connection. Must be
 * called on the SPICE event-loop thread.
 *
 * The layout is derived from the guest's ACTUAL monitor configuration (the
 * SpiceDisplayMonitorConfig reported by the display channel), clamped to the
 * bounds of the current combined surface, so that a guest which mirrors,
 * overlaps, or otherwise diverges from the client-requested geometry cannot
 * publish an out-of-range layout. Until the guest has reported a usable
 * configuration, the client-requested layout is published as a fallback.
 *
 * @param client
 *     The guac_client whose monitor layout should be sent.
 *
 * @param channel
 *     The SPICE display channel whose guest monitor configuration should be
 *     published.
 */
static void guac_spice_display_send_monitor_layout(guac_client* client,
        SpiceChannel* channel) {

    guac_spice_client* spice_client = (guac_spice_client*) client->data;

    /* Only relevant when multi-monitor support is enabled for this connection */
    if (spice_client->settings->max_secondary_monitors <= 0 || channel == NULL)
        return;

    /* Read the bounds of the current combined surface for clamping. Zero
     * dimensions mean no primary surface currently exists (never created, or
     * destroyed), in which case the guest-reported geometry cannot be validated
     * against the framebuffer and the client-requested fallback is used. */
    pthread_mutex_lock(&spice_client->surface_lock);
    int surface_width = spice_client->surface_width;
    int surface_height = spice_client->surface_height;
    pthread_mutex_unlock(&spice_client->surface_lock);

    /* Build a JSON object mapping each monitor index to its position and size.
     * Every value is an integer, so the fixed buffer cannot be overrun for the
     * supported number of monitors; the length is bounded-checked regardless. */
    char json[GUAC_SPICE_MULTIMON_LAYOUT_SIZE];
    int pos = 0;
    int written = 0;

    json[pos++] = '{';

    /* Prefer the guest's actual monitor configuration as the source of truth,
     * but only once a primary surface exists to validate it against */
    GArray* monitors = NULL;
    if (surface_width > 0 && surface_height > 0)
        g_object_get(SPICE_DISPLAY_CHANNEL(channel), "monitors", &monitors, NULL);

    int had_guest_config = (monitors != NULL);

    if (monitors != NULL) {

        for (guint i = 0; i < monitors->len; i++) {

            SpiceDisplayMonitorConfig* config =
                    &g_array_index(monitors, SpiceDisplayMonitorConfig, i);

            /* Key by the guest's head id (not the array index) so that a
             * disabled intermediate head does not renumber the surviving
             * monitors from the client's perspective */
            if (config->id >= GUAC_SPICE_MAX_MONITORS)
                continue;

            /* SpiceDisplayMonitorConfig coordinates are unsigned. Trim the
             * guest-reported region to the combined surface using subtraction
             * ordered to avoid any signed overflow: a head whose origin is at
             * or beyond a surface bound is treated as disabled. */
            guint gx = config->x, gy = config->y;
            guint gw = config->width, gh = config->height;

            if (gx >= (guint) surface_width || gy >= (guint) surface_height)
                continue;

            int left = (int) gx;
            int top = (int) gy;
            int width = (gw > (guint) (surface_width - left))
                    ? surface_width - left : (int) gw;
            int height = (gh > (guint) (surface_height - top))
                    ? surface_height - top : (int) gh;

            /* Skip disabled or zero-extent heads */
            if (width <= 0 || height <= 0)
                continue;

            int next = guac_spice_layout_append(json, pos, sizeof(json),
                    written, (int) config->id, left, top, width, height);
            if (next < 0)
                break;

            pos = next;
            written++;

        }

        g_clear_pointer(&monitors, g_array_unref);

    }

    /* Whether the published entries came from the guest (vs the fallback) */
    int from_guest = (written > 0);

    /* Fall back to the client-requested layout if the guest has not yet
     * reported any usable monitor configuration */
    if (!written) {
        for (int i = 0; i < spice_client->monitors_count; i++) {

            guac_spice_monitor* monitor = &spice_client->monitors[i];
            if (monitor->width <= 0 || monitor->height <= 0)
                continue;

            int next = guac_spice_layout_append(json, pos, sizeof(json),
                    written, i, monitor->left_offset, monitor->top_offset,
                    monitor->width, monitor->height);
            if (next < 0)
                break;

            pos = next;
            written++;

        }
    }

    /* Nothing to publish (no guest config yet and no client-requested layout) */
    if (!written) {
        guac_client_log(client, GUAC_LOG_DEBUG, "multimon-layout: nothing to "
                "publish (surface %dx%d, guest config %s)", surface_width,
                surface_height, had_guest_config ? "present" : "absent");
        return;
    }

    /* Terminate the JSON object, leaving room for '}' and the null terminator */
    if (pos > (int) sizeof(json) - 2)
        pos = (int) sizeof(json) - 2;
    json[pos++] = '}';
    json[pos] = '\0';

    /* Set the layout parameter on the default layer (layer 0) */
    guac_protocol_send_set(client->socket, GUAC_DEFAULT_LAYER,
            GUAC_PROTOCOL_LAYER_PARAMETER_MULTIMON_LAYOUT, json);
    guac_socket_flush(client->socket);

    guac_client_log(client, GUAC_LOG_DEBUG,
            "multimon-layout published (%s source, %d monitor(s)): %s",
            from_guest ? "guest-actual" : "client-requested",
            written, json);

}

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

    /* Resize the Guacamole display to match the new primary surface, guarding
     * against implausible server-supplied dimensions (defense-in-depth against
     * an oversized allocation; CWE-400/CWE-789). */
    if (spice_client->display != NULL
            && width > 0 && height > 0
            && width <= GUAC_DISPLAY_MAX_WIDTH
            && height <= GUAC_DISPLAY_MAX_HEIGHT)
        guac_display_layer_resize(
                guac_display_default_layer(spice_client->display),
                width, height);

    /* The display is now ready to receive a guest resize; flush any queued
     * client-requested resize that was waiting on the primary surface */
    spice_client->resize_display_ready = 1;
    guac_spice_resize_try(client);

    /* Inform any multi-monitor client of the layout of the (now resized)
     * combined framebuffer so it can split it into per-monitor windows */
    guac_spice_display_send_monitor_layout(client, channel);

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

    /* Reset the recorded dimensions so a subsequent monitor-layout publish does
     * not validate guest geometry against the bounds of the destroyed surface */
    spice_client->surface_width = 0;
    spice_client->surface_height = 0;
    spice_client->surface_stride = 0;
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

/**
 * Signal handler for the SPICE display channel "notify::monitors" signal,
 * fired whenever the guest publishes a new monitor configuration. Re-publishes
 * the (now updated) guest-actual layout to the connected client so that a
 * repositioning which does not recreate the primary surface is still reflected.
 */
static void guac_spice_display_monitors_updated(SpiceChannel* channel,
        GParamSpec* pspec, gpointer data) {

    guac_client* client = (guac_client*) data;
    guac_spice_display_send_monitor_layout(client, channel);

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

    /* Re-publish the monitor layout whenever the guest changes its monitor
     * configuration, keeping the client's per-monitor split in sync with the
     * guest's actual geometry even without a primary-surface recreate */
    g_signal_connect(channel, "notify::monitors",
            G_CALLBACK(guac_spice_display_monitors_updated), client);

}
