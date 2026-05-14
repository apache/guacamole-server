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

#include "color.h"
#include "channels/rail.h"
#include "plugins/channels.h"
#include "rdp.h"
#include "settings.h"

#ifdef HAVE_RDPGFX_SURFACE_AREA_UPDATE
#include <freerdp/codec/color.h>
#include <freerdp/codec/region.h>
#include <freerdp/gdi/gdi.h>
#include <freerdp/gdi/gfx.h>
#include <guacamole/display.h>
#include <guacamole/rect.h>
#endif

#include <freerdp/client/rail.h>
#include <freerdp/event.h>
#include <freerdp/freerdp.h>
#include <freerdp/rail.h>
#include <freerdp/window.h>
#include <guacamole/client.h>
#include <guacamole/mem.h>
#include <winpr/wtypes.h>
#include <winpr/wtsapi.h>

#include <stddef.h>
#include <string.h>

/*
 * RemoteApp (RAIL) with the RDPGFX pipeline.
 *
 * When both RAIL and GFX are active, the RDP server delivers per-window
 * graphical updates as RDPGFX surfaces tagged with a non-zero windowId
 * (gdiGfxSurface->windowId). The default GDI graphics pipeline only knows how
 * to paint into a single primary desktop buffer, so without intervention these
 * window-mapped surfaces are silently dropped on the floor.
 *
 * This module tracks each RAIL window with its own guac_display_layer,
 * positioned by the RAIL window-order stream and painted from the corresponding
 * RDPGFX surface via either:
 *
 *   - UpdateWindowFromSurface (preferred; called once per surface per frame
 *     with the surface's accumulated invalid region), or
 *
 *   - UpdateSurfaceArea (fallback for older FreeRDP without the window
 *     callback; called with rect lists for any surface update).
 *
 * We never install both at once on the same surface: when the window callback
 * is available, UpdateSurfaceArea is either not installed at all or no-ops on
 * window-mapped surfaces.
 */

struct guac_rdp_rail_window {

    /**
     * The server-side ID of the RAIL window.
     */
    UINT64 window_id;

    /**
     * The Guacamole layer that renders the contents of this window.
     */
    guac_display_layer* layer;

    /**
     * The position of this window within the remote desktop.
     */
    int x;
    int y;

    /**
     * Whether this window should be visible (per RAIL show state).
     */
    int visible;

    /**
     * Whether this window has received at least one graphical update from an
     * RDPGFX surface.
     */
    int has_surface;

    /**
     * Whether a RAIL WindowCreate/Update order has been observed for this
     * window. Layers created speculatively from a surface update (before any
     * RAIL order arrives) are held off-screen until this is set.
     */
    int has_rail_order;

    /**
     * The current dimensions of the Guacamole layer.
     */
    int layer_width;
    int layer_height;

    /**
     * The next RAIL window in the list.
     */
    guac_rdp_rail_window* next;

};

/**
 * Returns whether RAIL window surfaces should be rendered through Guacamole
 * layers for the given RDP client.
 */
static int guac_rdp_rail_uses_gfx(guac_rdp_client* rdp_client) {
    return rdp_client->settings->remote_app != NULL
        && rdp_client->settings->enable_gfx;
}

/**
 * Returns the RAIL window having the given ID, or NULL if no such window is
 * being tracked. The rail_windows_lock must be held.
 */
static guac_rdp_rail_window* guac_rdp_rail_get_window(
        guac_rdp_client* rdp_client, UINT64 window_id) {

    guac_rdp_rail_window* current = rdp_client->rail_windows;

    while (current != NULL) {
        if (current->window_id == window_id)
            return current;
        current = current->next;
    }

    return NULL;

}

/**
 * Returns the RAIL window having the given ID, creating it if necessary.
 * The rail_windows_lock must be held.
 */
static guac_rdp_rail_window* guac_rdp_rail_get_or_create_window(
        guac_client* client, UINT64 window_id) {

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    guac_rdp_rail_window* rail_window =
        guac_rdp_rail_get_window(rdp_client, window_id);

    if (rail_window != NULL)
        return rail_window;

    rail_window = guac_mem_zalloc(sizeof(guac_rdp_rail_window));
    if (rail_window == NULL) {
        guac_client_log(client, GUAC_LOG_WARNING, "RAIL window 0x%08x could "
                "not be tracked. Window contents may not render.",
                (unsigned int) window_id);
        return NULL;
    }

    rail_window->layer = guac_display_alloc_layer(rdp_client->display, 0);
    if (rail_window->layer == NULL) {
        guac_client_log(client, GUAC_LOG_WARNING, "RAIL window 0x%08x could "
                "not be tracked. Window contents may not render.",
                (unsigned int) window_id);
        guac_mem_free(rail_window);
        return NULL;
    }

    rail_window->window_id = window_id;
    rail_window->visible = 1;

    /* Keep the layer hidden until we have both a RAIL order (so we know where
     * to put it) and at least one surface paint (so we have something to
     * show). */
    guac_display_layer_set_opacity(rail_window->layer, 0);

    rail_window->next = rdp_client->rail_windows;
    rdp_client->rail_windows = rail_window;

    return rail_window;

}

/**
 * Updates the visibility of the Guacamole layer backing the given RAIL window.
 * The rail_windows_lock must be held.
 */
static void guac_rdp_rail_update_window_opacity(
        guac_rdp_rail_window* rail_window) {

    int opacity = (rail_window->visible
            && rail_window->has_surface
            && rail_window->has_rail_order) ? 0xFF : 0;
    guac_display_layer_set_opacity(rail_window->layer, opacity);

}

/**
 * Removes the given RAIL window from the list and frees its resources. The
 * rail_windows_lock must be held.
 */
static void guac_rdp_rail_free_window(guac_rdp_client* rdp_client,
        guac_rdp_rail_window* previous, guac_rdp_rail_window* rail_window) {

    if (previous != NULL)
        previous->next = rail_window->next;
    else
        rdp_client->rail_windows = rail_window->next;

    guac_display_free_layer(rail_window->layer);
    guac_mem_free(rail_window);

}

/**
 * Completes initialization of the RemoteApp session, responding to the server
 * handshake, sending client status and system parameters, and executing the
 * desired RemoteApp command. This is accomplished using the Handshake PDU,
 * Client Information PDU, one or more Client System Parameters Update PDUs,
 * and the Client Execute PDU respectively. These PDUs MUST be sent for the
 * desired RemoteApp to run, and MUST NOT be sent until after a Handshake or
 * HandshakeEx PDU has been received. See:
 *
 * https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-rdperp/cec4eb83-b304-43c9-8378-b5b8f5e7082a (Handshake PDU)
 * https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-rdperp/743e782d-f59b-40b5-a0f3-adc74e68a2ff (Client Information PDU)
 * https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-rdperp/60344497-883f-4711-8b9a-828d1c580195 (System Parameters Update PDU)
 * https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-rdperp/98a6e3c3-c2a9-42cc-ad91-0d9a6c211138 (Client Execute PDU)
 *
 * @param rail
 *     The RailClientContext structure used by FreeRDP to handle the RAIL
 *     channel for the current RDP session.
 *
 * @return
 *     CHANNEL_RC_OK (zero) if the PDUs were sent successfully, an error code
 *     (non-zero) otherwise.
 */
static UINT guac_rdp_rail_complete_handshake(RailClientContext* rail) {

    UINT status;

    guac_client* client = (guac_client*) rail->custom;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    RAIL_CLIENT_STATUS_ORDER client_status = {
        .flags =
                TS_RAIL_CLIENTSTATUS_ALLOWLOCALMOVESIZE
              | TS_RAIL_CLIENTSTATUS_APPBAR_REMOTING_SUPPORTED
    };

    /* Send client status */
    guac_client_log(client, GUAC_LOG_TRACE, "Sending RAIL client status.");
    pthread_mutex_lock(&(rdp_client->message_lock));
    status = rail->ClientInformation(rail, &client_status);
    pthread_mutex_unlock(&(rdp_client->message_lock));

    if (status != CHANNEL_RC_OK)
        return status;

    RAIL_SYSPARAM_ORDER sysparam = {

        .dragFullWindows = FALSE,

        .highContrast = {
            .flags =
                  HCF_AVAILABLE
                | HCF_CONFIRMHOTKEY
                | HCF_HOTKEYACTIVE
                | HCF_HOTKEYAVAILABLE
                | HCF_HOTKEYSOUND
                | HCF_INDICATOR,
            .colorScheme = {
                .string = NULL,
                .length = 0
            }
        },

        .keyboardCues = FALSE,
        .keyboardPref = FALSE,
        .mouseButtonSwap = FALSE,

        .workArea = {
            .left   = 0,
            .top    = 0,
            .right  = rdp_client->settings->width,
            .bottom = rdp_client->settings->height
        },

        .params =
              SPI_MASK_SET_HIGH_CONTRAST
            | SPI_MASK_SET_KEYBOARD_CUES
            | SPI_MASK_SET_KEYBOARD_PREF
            | SPI_MASK_SET_MOUSE_BUTTON_SWAP
            | SPI_MASK_SET_WORK_AREA

    };

    /* Send client system parameters */
    guac_client_log(client, GUAC_LOG_TRACE, "Sending RAIL client system parameters.");
    pthread_mutex_lock(&(rdp_client->message_lock));
    status = rail->ClientSystemParam(rail, &sysparam);
    pthread_mutex_unlock(&(rdp_client->message_lock));

    if (status != CHANNEL_RC_OK)
        return status;

    RAIL_EXEC_ORDER exec = {
        .flags = RAIL_EXEC_FLAG_EXPAND_ARGUMENTS,
        .RemoteApplicationProgram = rdp_client->settings->remote_app,
        .RemoteApplicationWorkingDir = rdp_client->settings->remote_app_dir,
        .RemoteApplicationArguments = rdp_client->settings->remote_app_args,
    };

    /* Execute desired RemoteApp command */
    guac_client_log(client, GUAC_LOG_TRACE, "Executing remote application.");
    pthread_mutex_lock(&(rdp_client->message_lock));
    status = rail->ClientExecute(rail, &exec);
    pthread_mutex_unlock(&(rdp_client->message_lock));

    return status;

}

/**
 * A callback function that is invoked when the RDP server sends the result
 * of the Remote App (RAIL) execution command back to the client, so that the
 * client can handle any required actions associated with the result.
 * 
 * @param context
 *     A pointer to the RAIL data structure associated with the current
 *     RDP connection.
 *
 * @param execResult
 *     A data structure containing the result of the RAIL command.
 *
 * @return
 *     CHANNEL_RC_OK (zero) if the result was handled successfully, otherwise
 *     a non-zero error code. This implementation always returns
 *     CHANNEL_RC_OK.
 */
static UINT guac_rdp_rail_execute_result(RailClientContext* context,
        const RAIL_EXEC_RESULT_ORDER* execResult) {

    guac_client* client = (guac_client*) context->custom;

    if (execResult->execResult != RAIL_EXEC_S_OK) {
        guac_client_log(client, GUAC_LOG_DEBUG, "Failed to execute RAIL command on server: %d", execResult->execResult);
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_UPSTREAM_UNAVAILABLE, "Failed to execute RAIL command.");
    }

    return CHANNEL_RC_OK;

}

/**
 * Callback which is invoked when a Handshake PDU is received from the RDP
 * server. No communication for RemoteApp may occur until the Handshake PDU
 * (or, alternatively, the HandshakeEx PDU) is received. See:
 *
 * https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-rdperp/cec4eb83-b304-43c9-8378-b5b8f5e7082a
 *
 * @param rail
 *     The RailClientContext structure used by FreeRDP to handle the RAIL
 *     channel for the current RDP session.
 *
 * @param handshake
 *     The RAIL_HANDSHAKE_ORDER structure representing the Handshake PDU that
 *     was received.
 *
 * @return
 *     CHANNEL_RC_OK (zero) if the PDU was handled successfully, an error code
 *     (non-zero) otherwise.
 */
static UINT guac_rdp_rail_handshake(RailClientContext* rail,
        RAIL_CONST RAIL_HANDSHAKE_ORDER* handshake) {
    guac_client* client = (guac_client*) rail->custom;
    guac_client_log(client, GUAC_LOG_TRACE, "RAIL handshake callback.");
    return guac_rdp_rail_complete_handshake(rail);
}

/**
 * Callback which is invoked when a HandshakeEx PDU is received from the RDP
 * server. No communication for RemoteApp may occur until the HandshakeEx PDU
 * (or, alternatively, the Handshake PDU) is received. See:
 *
 * https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-rdperp/5cec5414-27de-442e-8d4a-c8f8b41f3899
 *
 * @param rail
 *     The RailClientContext structure used by FreeRDP to handle the RAIL
 *     channel for the current RDP session.
 *
 * @param handshake_ex
 *     The RAIL_HANDSHAKE_EX_ORDER structure representing the HandshakeEx PDU
 *     that was received.
 *
 * @return
 *     CHANNEL_RC_OK (zero) if the PDU was handled successfully, an error code
 *     (non-zero) otherwise.
 */
static UINT guac_rdp_rail_handshake_ex(RailClientContext* rail,
        RAIL_CONST RAIL_HANDSHAKE_EX_ORDER* handshake_ex) {
    guac_client* client = (guac_client*) rail->custom;
    guac_client_log(client, GUAC_LOG_TRACE, "RAIL handshake ex callback.");
    return guac_rdp_rail_complete_handshake(rail);
}

/**
 * A callback function that is executed when an update for a RAIL window is
 * received from the RDP server.
 *
 * @param context
 *     A pointer to the rdpContext structure used by FreeRDP to handle the
 *     window update.
 *
 * @param orderInfo
 *     A pointer to the data structure that contains information about what
 *     window was updated what updates were performed.
 *
 * @param windowState
 *     A pointer to the data structure that contains details of the updates
 *     to the window, as indicated by flags in the orderInfo field.
 *
 * @return
 *     TRUE if the client-side processing of the updates as successful; otherwise
 *     FALSE. This implementation always returns TRUE.
 */
static BOOL guac_rdp_rail_window_update(rdpContext* context,
        RAIL_CONST WINDOW_ORDER_INFO* orderInfo,
        RAIL_CONST WINDOW_STATE_ORDER* windowState) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    guac_client_log(client, GUAC_LOG_TRACE, "RAIL window update callback: %d", orderInfo->fieldFlags);

    UINT32 fieldFlags = orderInfo->fieldFlags;

    /* Track window location and visibility for RDPGFX RemoteApp rendering. */
    if (guac_rdp_rail_uses_gfx(rdp_client)) {

        pthread_mutex_lock(&(rdp_client->rail_windows_lock));

        guac_rdp_rail_window* rail_window =
            guac_rdp_rail_get_or_create_window(client, orderInfo->windowId);

        if (rail_window != NULL) {

            int needs_opacity_update = !rail_window->has_rail_order;
            rail_window->has_rail_order = 1;

            if (fieldFlags & WINDOW_ORDER_FIELD_WND_OFFSET) {
                rail_window->x = windowState->windowOffsetX;
                rail_window->y = windowState->windowOffsetY;
                guac_display_layer_move(rail_window->layer,
                        rail_window->x, rail_window->y);
                rdp_client->gdi_modified = 1;
            }

            if (fieldFlags & WINDOW_ORDER_FIELD_SHOW) {
                rail_window->visible =
                    windowState->showState != GUAC_RDP_RAIL_WINDOW_STATE_HIDDEN
                    && windowState->showState != GUAC_RDP_RAIL_WINDOW_STATE_MINIMIZED;
                needs_opacity_update = 1;
            }

            if (needs_opacity_update) {
                guac_rdp_rail_update_window_opacity(rail_window);
                rdp_client->gdi_modified = 1;
            }

        }

        pthread_mutex_unlock(&(rdp_client->rail_windows_lock));

    }

    /* If the flag for window visibilty is set, check visibility. */
    if (fieldFlags & WINDOW_ORDER_FIELD_SHOW) {
        guac_client_log(client, GUAC_LOG_TRACE, "RAIL window visibility change: %d", windowState->showState);

        /* State is either hidden or minimized - send restore command. */
        if (windowState->showState == GUAC_RDP_RAIL_WINDOW_STATE_MINIMIZED) {

            guac_client_log(client, GUAC_LOG_DEBUG, "RAIL window minimized, sending restore command.");

            RAIL_SYSCOMMAND_ORDER syscommand;
            syscommand.windowId = orderInfo->windowId;
            syscommand.command = SC_RESTORE;
            rdp_client->rail_interface->ClientSystemCommand(rdp_client->rail_interface, &syscommand);
        }
    }

    return TRUE;

}

/**
 * Handler for the RAIL WindowDelete order. Removes any tracked RAIL window
 * with the given ID.
 */
static BOOL guac_rdp_rail_window_delete(rdpContext* context,
        RAIL_CONST WINDOW_ORDER_INFO* orderInfo) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    if (!guac_rdp_rail_uses_gfx(rdp_client))
        return TRUE;

    pthread_mutex_lock(&(rdp_client->rail_windows_lock));

    guac_rdp_rail_window* previous = NULL;
    guac_rdp_rail_window* current = rdp_client->rail_windows;

    while (current != NULL) {
        if (current->window_id == orderInfo->windowId) {
            guac_rdp_rail_free_window(rdp_client, previous, current);
            rdp_client->gdi_modified = 1;
            break;
        }
        previous = current;
        current = current->next;
    }

    pthread_mutex_unlock(&(rdp_client->rail_windows_lock));

    return TRUE;

}

#ifdef HAVE_RDPGFX_SURFACE_AREA_UPDATE
/**
 * Copies updated regions from the given RDPGFX surface to the Guacamole layer
 * associated with that surface's RAIL window. The rail_windows_lock must NOT
 * be held on entry; this function takes it internally.
 *
 * The invalidRegion of the surface is left untouched. FreeRDP's GDI pipeline
 * owns that region and may consume or clear it independently; touching it
 * from these callbacks led to misplaced updates in early versions of this
 * code (menus appearing in the wrong location, etc.) because regions were
 * being double-consumed.
 */
static UINT guac_rdp_rail_paint_surface(guac_client* client,
        gdiGfxSurface* surface, UINT32 rect_count, const RECTANGLE_16* rects) {

    if (surface == NULL || surface->windowId == 0
            || rects == NULL || rect_count == 0)
        return CHANNEL_RC_OK;

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    pthread_mutex_lock(&(rdp_client->rail_windows_lock));

    guac_rdp_rail_window* rail_window =
        guac_rdp_rail_get_or_create_window(client, surface->windowId);

    if (rail_window == NULL) {
        pthread_mutex_unlock(&(rdp_client->rail_windows_lock));
        return CHANNEL_RC_OK;
    }

    /* Per FreeRDP's gdi/gfx.c and xf_gfx.c, mappedWidth/Height and
     * outputTargetWidth/Height are initialized to the same value at surface
     * creation, and only diverge when an explicit MapSurfaceToScaledWindow
     * order rescales the surface. We honor any divergence but optimize for
     * the common 1:1 case with a plain copy. */
    int source_width = surface->mappedWidth;
    int source_height = surface->mappedHeight;
    int target_width = surface->outputTargetWidth;
    int target_height = surface->outputTargetHeight;

    if (target_width <= 0) target_width = source_width;
    if (target_height <= 0) target_height = source_height;

    if (source_width <= 0 || source_height <= 0
            || target_width <= 0 || target_height <= 0) {
        rail_window->has_surface = 0;
        guac_rdp_rail_update_window_opacity(rail_window);
        pthread_mutex_unlock(&(rdp_client->rail_windows_lock));
        return CHANNEL_RC_OK;
    }

    int scaled = (source_width != target_width
            || source_height != target_height);

    if (rail_window->layer_width != target_width
            || rail_window->layer_height != target_height) {
        guac_display_layer_resize(rail_window->layer,
                target_width, target_height);
        rail_window->layer_width = target_width;
        rail_window->layer_height = target_height;
    }

    guac_display_layer_raw_context* layer_context =
        guac_display_layer_open_raw(rail_window->layer);

    guac_rect source_bounds;
    guac_rect_init(&source_bounds, 0, 0, source_width, source_height);

    for (UINT32 i = 0; i < rect_count; i++) {

        const RECTANGLE_16* rect = &rects[i];
        guac_rect src_rect;
        guac_rect_init(&src_rect, rect->left, rect->top,
                rect->right - rect->left, rect->bottom - rect->top);

        guac_rect_constrain(&src_rect, &source_bounds);
        if (guac_rect_is_empty(&src_rect))
            continue;

        guac_rect dst_rect;
        if (scaled) {
            dst_rect.left =
                ((UINT64) src_rect.left * target_width) / source_width;
            dst_rect.top =
                ((UINT64) src_rect.top * target_height) / source_height;
            dst_rect.right =
                (((UINT64) src_rect.right * target_width)
                    + source_width - 1) / source_width;
            dst_rect.bottom =
                (((UINT64) src_rect.bottom * target_height)
                    + source_height - 1) / source_height;
        }
        else {
            dst_rect = src_rect;
        }

        guac_rect_constrain(&dst_rect, &layer_context->bounds);
        if (guac_rect_is_empty(&dst_rect))
            continue;

        BOOL success;
        if (!scaled)
            success = freerdp_image_copy(
                    GUAC_DISPLAY_LAYER_RAW_BUFFER(layer_context, dst_rect),
                    guac_rdp_get_native_pixel_format(TRUE),
                    layer_context->stride,
                    0, 0,
                    guac_rect_width(&dst_rect),
                    guac_rect_height(&dst_rect),
                    surface->data, surface->format, surface->scanline,
                    src_rect.left, src_rect.top,
                    NULL, FREERDP_FLIP_NONE);
        else
            success = freerdp_image_scale(
                    GUAC_DISPLAY_LAYER_RAW_BUFFER(layer_context, dst_rect),
                    guac_rdp_get_native_pixel_format(TRUE),
                    layer_context->stride,
                    0, 0,
                    guac_rect_width(&dst_rect),
                    guac_rect_height(&dst_rect),
                    surface->data, surface->format, surface->scanline,
                    src_rect.left, src_rect.top,
                    guac_rect_width(&src_rect),
                    guac_rect_height(&src_rect));

        if (!success) {
            guac_display_layer_close_raw(rail_window->layer, layer_context);
            pthread_mutex_unlock(&(rdp_client->rail_windows_lock));
            return ERROR_INTERNAL_ERROR;
        }

        guac_rect_extend(&layer_context->dirty, &dst_rect);

    }

    guac_display_layer_close_raw(rail_window->layer, layer_context);

    rail_window->has_surface = 1;
    guac_rdp_rail_update_window_opacity(rail_window);
    rdp_client->gdi_modified = 1;

    pthread_mutex_unlock(&(rdp_client->rail_windows_lock));

    return CHANNEL_RC_OK;

}

UINT guac_rdp_rail_update_surface_area(RdpgfxClientContext* context,
        UINT16 surface_id, UINT32 rect_count, const RECTANGLE_16* rects) {

    rdpGdi* gdi = (rdpGdi*) context->custom;
    if (gdi == NULL || gdi->context == NULL || gdi->suppressOutput)
        return CHANNEL_RC_OK;

    if (context->GetSurfaceData == NULL)
        return CHANNEL_RC_OK;

    gdiGfxSurface* surface =
        (gdiGfxSurface*) context->GetSurfaceData(context, surface_id);

    /* Only window-mapped surfaces are routed through us; non-RAIL surfaces
     * are handled by FreeRDP's default GDI pipeline. We return early without
     * touching invalidRegion so the default path can consume it normally on
     * the next UpdateSurfaces tick. */
    if (surface == NULL || surface->windowId == 0)
        return CHANNEL_RC_OK;

    guac_client* client = ((rdp_freerdp_context*) gdi->context)->client;
    return guac_rdp_rail_paint_surface(client, surface, rect_count, rects);

}

#ifdef HAVE_RDPGFX_WINDOW_SURFACE_UPDATE
UINT guac_rdp_rail_update_window_from_surface(RdpgfxClientContext* context,
        gdiGfxSurface* surface) {

    rdpGdi* gdi = (rdpGdi*) context->custom;
    if (gdi == NULL || gdi->context == NULL || gdi->suppressOutput)
        return CHANNEL_RC_OK;

    if (surface == NULL || surface->windowId == 0)
        return CHANNEL_RC_OK;

    guac_client* client = ((rdp_freerdp_context*) gdi->context)->client;

    /* Snapshot the rect list before painting. region16_rects() returns a
     * pointer into the region's internal storage, which is only valid until
     * the region is next modified. FreeRDP's GDI pipeline clears
     * invalidRegion after this callback returns. */
    UINT32 rect_count = 0;
    const RECTANGLE_16* rects =
        region16_rects(&surface->invalidRegion, &rect_count);

    return guac_rdp_rail_paint_surface(client, surface, rect_count, rects);

}
#endif
#endif

void guac_rdp_rail_free_windows(guac_rdp_client* rdp_client) {

    pthread_mutex_lock(&(rdp_client->rail_windows_lock));

    guac_rdp_rail_window* current = rdp_client->rail_windows;
    rdp_client->rail_windows = NULL;

    while (current != NULL) {
        guac_rdp_rail_window* next = current->next;
        guac_display_free_layer(current->layer);
        guac_mem_free(current);
        current = next;
    }

    pthread_mutex_unlock(&(rdp_client->rail_windows_lock));

}

/**
 * Callback which associates handlers specific to Guacamole with the
 * RailClientContext instance allocated by FreeRDP to deal with received
 * RAIL (RemoteApp) messages.
 *
 * This function is called whenever a channel connects via the PubSub event
 * system within FreeRDP, but only has any effect if the connected channel is
 * the RAIL channel. This specific callback is registered with the PubSub
 * system of the relevant rdpContext when guac_rdp_rail_load_plugin() is
 * called.
 *
 * @param context
 *     The rdpContext associated with the active RDP session.
 *
 * @param args
 *     Event-specific arguments, mainly the name of the channel, and a
 *     reference to the associated plugin loaded for that channel by FreeRDP.
 */
static void guac_rdp_rail_channel_connected(rdpContext* context,
        ChannelConnectedEventArgs* args) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Ignore connection event if it's not for the RAIL channel */
    if (strcmp(args->name, RAIL_SVC_CHANNEL_NAME) != 0)
        return;

    /* The structure pointed to by pInterface is guaranteed to be a
     * RailClientContext if the channel is RAIL */
    RailClientContext* rail = (RailClientContext*) args->pInterface;
    rdp_client->rail_interface = rail;

    /* Init FreeRDP RAIL context, ensuring the guac_client can be accessed from
     * within any RAIL-specific callbacks */
    rail->custom = client;
    rail->ServerExecuteResult = guac_rdp_rail_execute_result;
    rail->ServerHandshake = guac_rdp_rail_handshake;
    rail->ServerHandshakeEx = guac_rdp_rail_handshake_ex;
    context->update->window->WindowCreate = guac_rdp_rail_window_update;
    context->update->window->WindowUpdate = guac_rdp_rail_window_update;
    context->update->window->WindowDelete = guac_rdp_rail_window_delete;

    guac_client_log(client, GUAC_LOG_DEBUG, "RAIL (RemoteApp) channel "
            "connected.");

}

void guac_rdp_rail_load_plugin(rdpContext* context) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;

    /* Attempt to load FreeRDP support for the RAIL channel */
    if (guac_freerdp_channels_load_plugin(context, "rail", context->settings)) {
        guac_client_log(client, GUAC_LOG_WARNING,
                "Support for the RAIL channel (RemoteApp) could not be "
                "loaded. This support normally takes the form of a plugin "
                "which is built into FreeRDP. Lacking this support, "
                "RemoteApp will not work.");
        return;
    }

    /* Complete RDP side of initialization when channel is connected */
    PubSub_SubscribeChannelConnected(context->pubSub,
            (pChannelConnectedEventHandler) guac_rdp_rail_channel_connected);

    guac_client_log(client, GUAC_LOG_DEBUG, "Support for RAIL (RemoteApp) "
            "registered. Awaiting channel connection.");

}
