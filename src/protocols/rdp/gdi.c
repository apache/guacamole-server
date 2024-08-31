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

#include "color.h"
#include "guacamole/display.h"
#include "rdp.h"
#include "settings.h"

#include <cairo/cairo.h>
#include <freerdp/freerdp.h>
#include <freerdp/gdi/gdi.h>
#include <freerdp/graphics.h>
#include <freerdp/primary.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <winpr/wtypes.h>

#include <stddef.h>

void guac_rdp_gdi_mark_frame(rdpContext* context, int starting) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* The server supports defining explicit frames */
    rdp_client->frames_supported = 1;

    /* A new frame is beginning */
    if (starting) {
        rdp_client->in_frame = 1;
        return;
    }

    /* The current frame has ended */
    guac_timestamp frame_end = guac_timestamp_current();
    int time_elapsed = frame_end - client->last_sent_timestamp;
    rdp_client->in_frame = 0;

    /* A new frame has been received from the RDP server and processed */
    rdp_client->frames_received++;

    /* Flush a new frame if the client is ready for it */
    if (time_elapsed >= guac_client_get_processing_lag(client)) {
        guac_display_end_multiple_frames(rdp_client->display, rdp_client->frames_received);
        guac_socket_flush(client->socket);
        rdp_client->frames_received = 0;
    }

}

BOOL guac_rdp_gdi_frame_marker(rdpContext* context, const FRAME_MARKER_ORDER* frame_marker) {
    guac_rdp_gdi_mark_frame(context, frame_marker->action == FRAME_START);
    return TRUE;
}

BOOL guac_rdp_gdi_surface_frame_marker(rdpContext* context, const SURFACE_FRAME_MARKER* surface_frame_marker) {

    guac_rdp_gdi_mark_frame(context, surface_frame_marker->frameAction != SURFACECMD_FRAMEACTION_END);

    int frame_acknowledge;
#ifdef HAVE_SETTERS_GETTERS
    frame_acknowledge = freerdp_settings_get_uint32(context->settings, FreeRDP_FrameAcknowledge);
#else
    frame_acknowledge = context->settings->FrameAcknowledge;
#endif

    if (frame_acknowledge > 0)
        IFCALL(context->update->SurfaceFrameAcknowledge, context,
                surface_frame_marker->frameId);

    return TRUE;

}

BOOL guac_rdp_gdi_begin_paint(rdpContext* context) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Leverage BeginPaint handler to detect start of frame for RDPGFX channel */
    if (rdp_client->settings->enable_gfx && rdp_client->frames_supported)
        guac_rdp_gdi_mark_frame(context, 1);

    return TRUE;

}

BOOL guac_rdp_gdi_end_paint(rdpContext* context) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    rdpGdi* gdi = context->gdi;

    /* Ignore paint if GDI output is suppressed */
    if (gdi->suppressOutput)
        return TRUE;

    /* Ignore paint if nothing has been done (empty rect) */
    if (gdi->primary->hdc->hwnd->invalid->null)
        return TRUE;

    INT32 x = gdi->primary->hdc->hwnd->invalid->x;
    INT32 y = gdi->primary->hdc->hwnd->invalid->y;
    UINT32 w = gdi->primary->hdc->hwnd->invalid->w;
    UINT32 h = gdi->primary->hdc->hwnd->invalid->h;

    guac_display_layer* default_layer = guac_display_default_layer(rdp_client->display);
    guac_display_layer_raw_context* dst_context = guac_display_layer_open_raw(default_layer);

    guac_rect dst_rect;
    guac_rect_init(&dst_rect, x, y, w, h);
    guac_rect_constrain(&dst_rect, &dst_context->bounds);

    guac_display_layer_raw_context_put(dst_context, &dst_rect,
        GUAC_RECT_CONST_BUFFER(dst_rect, gdi->primary_buffer, gdi->stride, 4),
        gdi->stride);

    guac_rect_extend(&dst_context->dirty, &dst_rect);

    guac_display_layer_close_raw(default_layer, dst_context);

    /* Next frame */
    if (gdi->inGfxFrame) {
        guac_rdp_gdi_mark_frame(context, 0);
    }

    return TRUE;

}

BOOL guac_rdp_gdi_desktop_resize(rdpContext* context) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    int width = guac_rdp_get_width(context->instance);
    int height = guac_rdp_get_height(context->instance);

    guac_display_layer_resize(guac_display_default_layer(rdp_client->display), width, height);
    guac_client_log(client, GUAC_LOG_DEBUG, "Server resized display to %ix%i", width, height);

    return gdi_resize(context->gdi, width, height);

}
