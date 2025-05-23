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

#include "channels/disp.h"
#include "plugins/channels.h"
#include "fs.h"
#include "rdp.h"
#include "settings.h"

#include <freerdp/client/disp.h>
#include <freerdp/freerdp.h>
#include <freerdp/event.h>
#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <guacamole/rect.h>
#include <guacamole/timestamp.h>

#include <stdlib.h>
#include <string.h>

guac_rdp_disp* guac_rdp_disp_alloc(guac_client* client) {

    guac_rdp_disp* disp = guac_mem_alloc(sizeof(guac_rdp_disp));
    disp->client = client;

    /* Not yet connected */
    disp->disp = NULL;

    /* No requests have been made */
    disp->last_request = guac_timestamp_current();
    disp->reconnect_needed = 0;

    /* Init first monitor */
    disp->monitors = guac_mem_alloc(sizeof(guac_rdp_disp_monitor));
    disp->monitors[0].requested_width  = 0;
    disp->monitors[0].requested_height = 0;
    disp->monitors[0].x_position = 0;
    disp->monitors[0].top_offset = 0;
    disp->monitors_count = 1;

    return disp;

}

void guac_rdp_disp_free(guac_rdp_disp* disp) {
    guac_mem_free(disp->monitors);
    guac_mem_free(disp);
}

/**
 * Callback which associates handlers specific to Guacamole with the
 * DispClientContext instance allocated by FreeRDP to deal with received
 * Display Update (client-initiated dynamic display resizing) messages.
 *
 * This function is called whenever a channel connects via the PubSub event
 * system within FreeRDP, but only has any effect if the connected channel is
 * the Display Update channel. This specific callback is registered with the
 * PubSub system of the relevant rdpContext when guac_rdp_disp_load_plugin() is
 * called.
 *
 * @param context
 *     The rdpContext associated with the active RDP session.
 *
 * @param args
 *     Event-specific arguments, mainly the name of the channel, and a
 *     reference to the associated plugin loaded for that channel by FreeRDP.
 */
static void guac_rdp_disp_channel_connected(rdpContext* context,
        ChannelConnectedEventArgs* args) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_rdp_disp* guac_disp = rdp_client->disp;

    /* Ignore connection event if it's not for the Display Update channel */
    if (strcmp(args->name, DISP_DVC_CHANNEL_NAME) != 0)
        return;

    /* Init module with current display size */
    guac_rdp_disp_set_size(guac_disp, rdp_client->settings,
            context->instance, guac_rdp_get_width(context->instance),
            guac_rdp_get_height(context->instance), 0, 0);

    /* Store reference to the display update plugin once it's connected */
    DispClientContext* disp = (DispClientContext*) args->pInterface;
    guac_disp->disp = disp;

    guac_client_log(client, GUAC_LOG_DEBUG, "Display update channel "
            "will be used for display size changes.");

}

/**
 * Callback which disassociates Guacamole from the DispClientContext instance
 * that was originally allocated by FreeRDP and is about to be deallocated.
 *
 * This function is called whenever a channel disconnects via the PubSub event
 * system within FreeRDP, but only has any effect if the disconnected channel
 * is the Display Update channel. This specific callback is registered with the
 * PubSub system of the relevant rdpContext when guac_rdp_disp_load_plugin() is
 * called.
 *
 * @param context
 *     The rdpContext associated with the active RDP session.
 *
 * @param args
 *     Event-specific arguments, mainly the name of the channel, and a
 *     reference to the associated plugin loaded for that channel by FreeRDP.
 */
static void guac_rdp_disp_channel_disconnected(rdpContext* context,
        ChannelDisconnectedEventArgs* args) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_rdp_disp* guac_disp = rdp_client->disp;

    /* Ignore disconnection event if it's not for the Display Update channel */
    if (strcmp(args->name, DISP_DVC_CHANNEL_NAME) != 0)
        return;

    /* Channel is no longer connected */
    guac_disp->disp = NULL;

    guac_client_log(client, GUAC_LOG_DEBUG, "Display update channel "
            "disconnected.");

}

void guac_rdp_disp_load_plugin(rdpContext* context) {

    /* Subscribe to and handle channel connected events */
    PubSub_SubscribeChannelConnected(context->pubSub,
        (pChannelConnectedEventHandler) guac_rdp_disp_channel_connected);

    /* Subscribe to and handle channel disconnected events */
    PubSub_SubscribeChannelDisconnected(context->pubSub,
            (pChannelDisconnectedEventHandler) guac_rdp_disp_channel_disconnected);

    /* Add "disp" channel */
    guac_freerdp_dynamic_channel_collection_add(context->settings, "disp", NULL);

}

/**
 * Reallocates the monitors to the given size.
 *
 * @param disp
 *     The display update module to reallocate.
 *
 * @param requested_monitors
 *     The number of monitors to allocate.
 */
static void guac_rdp_disp_realloc_monitors(guac_rdp_disp* disp,
        int requested_monitors) {

    /* No need to reallocate if the number of monitors is unchanged */
    if (disp->monitors_count == requested_monitors)
        return;

    disp->monitors = guac_mem_realloc(disp->monitors,
            requested_monitors * sizeof(guac_rdp_disp_monitor));

    disp->monitors_count = requested_monitors;

}

/**
 * Returns the x-offset of the monitor at the given position from the left edge
 * of the screen.
 *
 * @param disp
 *     The display update module to query.
 *
 * @param x_position
 *     The position of the monitor to query.
 *
 * @return
 *     The offset of the monitor at the given position from the left edge of
 *     the screen, in pixels.
 */
static int guac_rdp_disp_get_x_offset(const guac_rdp_disp* disp, int x_position) {

    int x_offset = 0;

    /* Calculate the offset of the monitor from the left edge of the screen */
    for (int i = 0; i < x_position; i++)
        x_offset += disp->monitors[i].requested_width;

    return x_offset;

}

/**
 * Returns the "total" height of all monitors. This is not the sum of the
 * heights of all monitors, but rather the height of the entire screen.
 *
 * @param disp
 *     The display update module to query.
 *
 * @return
 *     The total height of the display, in pixels.
 */
static int guac_rdp_disp_get_total_height(const guac_rdp_disp* disp) {

    int total_height = 0;
    int height;

    /* Loop through all monitors to find the maximum height */
    for (int i = 0; i < disp->monitors_count; i++) {
        height = disp->monitors[i].requested_height + disp->monitors[i].top_offset;
        if (height > total_height)
            total_height = height;
    }

    return total_height;

}

/**
 * Closes the monitor at the given position, if possible. If the monitor at the
 * given position is at the end of the list and there are other monitors, it
 * will be closed. Do nothing otherwise.
 *
 * @param disp
 *     The display update module to close the monitor of.
 *
 * @param x_position
 *     The position of the monitor to close.
 *
 * @return
 *     true if the monitor was closed, false otherwise.
 */
static bool guac_rdp_disp_close_monitor(guac_rdp_disp* disp, int x_position) {

    /* Allow to close only the monitor at the end of the list if there is more
     * than one monitor */
    if (x_position == 0 || x_position + 1 != disp->monitors_count)
        return false;

    /* Close monitor */
    guac_rdp_disp_realloc_monitors(disp, x_position);

    return true;

}

void guac_rdp_disp_set_size(guac_rdp_disp* disp, guac_rdp_settings* settings,
        freerdp* rdp_inst, int width, int height, int x_position, int top_offset) {

    int min_monitors_requested = x_position + 1;

    /* Add one to account for the primary monitor */
    int max_monitors = settings->max_secondary_monitors + 1;

    /* Ignore invalid requests :
     * - Invalid monitor index.
     * - Too many monitors requested.
     * - Missing intermediate monitor(s).
     */
    if (x_position < 0 || max_monitors < min_monitors_requested
            || disp->monitors_count + 1 < min_monitors_requested)
        return;

    guac_rect resize = {
        .left = 0,
        .top = 0,
        .right = width,
        .bottom = height
    };

    /* Fit width and height within bounds, maintaining aspect ratio */
    guac_rect_shrink(&resize, GUAC_RDP_DISP_MAX_SIZE, GUAC_RDP_DISP_MAX_SIZE);

    width = guac_rect_width(&resize);
    height = guac_rect_height(&resize);

    if (width > 0 && height > 0) {

        /* As it's possible for a rectangle to exceed the maximum allowed
        * dimensions, yet fall below the minimum allowed dimensions once adjusted,
        * we don't bother preserving aspect ratio for the unlikely case that a
        * dimension is below the minimums (consider a rectangle like 16384x256) */
        if (width  < GUAC_RDP_DISP_MIN_SIZE) width  = GUAC_RDP_DISP_MIN_SIZE;
        if (height < GUAC_RDP_DISP_MIN_SIZE) height = GUAC_RDP_DISP_MIN_SIZE;

        /* Width must be even */
        if (width % 2 == 1)
            width -= 1;

        /* Reallocate monitors if needed */
        if (disp->monitors_count < min_monitors_requested)
            guac_rdp_disp_realloc_monitors(disp, min_monitors_requested);

        /* Store deferred size */
        disp->monitors[x_position].requested_width  = width;
        disp->monitors[x_position].requested_height = height;
        disp->monitors[x_position].x_position = x_position;
        disp->monitors[x_position].top_offset = top_offset;
    }

    /* Try to close monitor or ignore request */
    else if (!guac_rdp_disp_close_monitor(disp, x_position))
        return;

    /* Send display update notification if possible */
    guac_rdp_disp_update_size(disp, settings, rdp_inst);

}

void guac_rdp_disp_update_size(guac_rdp_disp* disp,
        guac_rdp_settings* settings, freerdp* rdp_inst) {

    guac_timestamp now = guac_timestamp_current();

    /* Limit display update frequency */
    if (now - disp->last_request <= GUAC_RDP_DISP_UPDATE_INTERVAL)
        return;

    int monitors_count = disp->monitors_count;
    int width = guac_rdp_disp_get_x_offset(disp, monitors_count);
    int height = guac_rdp_disp_get_total_height(disp);

    /* Do NOT send requests unless the size will change */
    if (rdp_inst != NULL
            && width == guac_rdp_get_width(rdp_inst)
            && height == guac_rdp_get_height(rdp_inst))
        return;

    disp->last_request = now;

    if (settings->resize_method == GUAC_RESIZE_RECONNECT) {

        /* Update settings with new dimensions */
        settings->width = width;
        settings->height = height;

        /* Signal reconnect */
        disp->reconnect_needed = 1;

    }

    /* Send display update notification if display channel is connected */
    else if (settings->resize_method == GUAC_RESIZE_DISPLAY_UPDATE
                && disp->disp != NULL) {

        /* Init monitors layout */
        DISPLAY_CONTROL_MONITOR_LAYOUT* monitors;
        monitors = guac_mem_alloc(monitors_count * sizeof(DISPLAY_CONTROL_MONITOR_LAYOUT));

        for (int i = 0; i < monitors_count; i++) {

            /* First monitor is the primary */
            int primary_monitor = (i == 0 ? 1 : 0);

            /* Set current monitor properties */
            monitors[i].Flags = primary_monitor;
            monitors[i].Left = guac_rdp_disp_get_x_offset(disp, i);
            monitors[i].Top = disp->monitors[i].top_offset;
            monitors[i].Width = disp->monitors[i].requested_width;
            monitors[i].Height = disp->monitors[i].requested_height;
            monitors[i].Orientation = 0;
            monitors[i].PhysicalWidth = 0;
            monitors[i].PhysicalHeight = 0;
        }

        /* Send display update notification */
        guac_client* client = disp->client;
        guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

        pthread_mutex_lock(&(rdp_client->message_lock));
        disp->disp->SendMonitorLayout(disp->disp, monitors_count, monitors);
        pthread_mutex_unlock(&(rdp_client->message_lock));

        guac_mem_free(monitors);

    }

}

int guac_rdp_disp_reconnect_needed(guac_rdp_disp* disp) {
    guac_rdp_client* rdp_client = (guac_rdp_client*) disp->client->data;

    /* Do not reconnect if files are open. */
    if (rdp_client->filesystem != NULL
            && rdp_client->filesystem->open_files > 0)
        return 0;

    /* Do not reconnect if an active print job is present */
    if (rdp_client->active_job != NULL)
        return 0;
    
    
    return disp->reconnect_needed;
}

void guac_rdp_disp_reconnect_complete(guac_rdp_disp* disp) {
    disp->reconnect_needed = 0;
    disp->last_request = guac_timestamp_current();
}

