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
#include "common/list.h"
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
#include <winpr/string.h>
#include <winpr/wtypes.h>
#include <winpr/wtsapi.h>

#include <stddef.h>
#include <stdint.h>
#include <string.h>

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

    /* Do not advertise TS_RAIL_CLIENTSTATUS_ALLOWLOCALMOVESIZE. Guacamole
     * does not manage native local windows and does not send ClientWindowMove
     * when a local move/resize completes. Advertising that capability can
     * cause the server to enter a move/resize path that Guacamole cannot
     * finish, leaving subsequent input ignored after dragging a RemoteApp
     * window. */
    RAIL_CLIENT_STATUS_ORDER client_status = {
        .flags = TS_RAIL_CLIENTSTATUS_APPBAR_REMOTING_SUPPORTED
    };

    /* Send client status */
    guac_client_log(client, GUAC_LOG_TRACE,
            "Sending RAIL client status: flags=0x%08x.",
            (unsigned int) client_status.flags);
    pthread_mutex_lock(&(rdp_client->message_lock));
    status = rail->ClientInformation(rail, &client_status);
    pthread_mutex_unlock(&(rdp_client->message_lock));

    if (status != CHANNEL_RC_OK) {
        guac_client_log(client, GUAC_LOG_DEBUG,
                "RAIL client status send failed: status=%u.",
                (unsigned int) status);
        return status;
    }

    RAIL_SYSPARAM_ORDER sysparam = {

        .dragFullWindows = rdp_client->settings->full_window_drag_enabled,

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
#ifdef SPI_MASK_SET_DRAG_FULL_WINDOWS
            | SPI_MASK_SET_DRAG_FULL_WINDOWS
#endif
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
    guac_client_log(client, GUAC_LOG_TRACE,
            "Executing remote application: program=\"%s\", args=\"%s\", "
            "working-dir=\"%s\", flags=0x%04x.",
            exec.RemoteApplicationProgram != NULL
                ? exec.RemoteApplicationProgram : "",
            exec.RemoteApplicationArguments != NULL
                ? exec.RemoteApplicationArguments : "",
            exec.RemoteApplicationWorkingDir != NULL
                ? exec.RemoteApplicationWorkingDir : "",
            (unsigned int) exec.flags);
    pthread_mutex_lock(&(rdp_client->message_lock));
    status = rail->ClientExecute(rail, &exec);
    pthread_mutex_unlock(&(rdp_client->message_lock));

    guac_client_log(client, GUAC_LOG_TRACE,
            "RAIL client execute send completed: status=%u.",
            (unsigned int) status);

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

    if (execResult->execResult == RAIL_EXEC_E_NOT_IN_ALLOWLIST) {
        guac_client_log(client, GUAC_LOG_WARNING, "RAIL command reported "
                "application not in allowlist.");
    }

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
 *   - UpdateSurfaceArea (fallback for older FreeRDP without the window
 *     callback; called with rect lists for any surface update).
 *
 *   - UpdateWindowFromSurface, when available.
 */

/* 
 * Each guac_rdp_rail_window stores the per-window state needed to combine
 * RAIL window ordering with RDPGFX surface updates and render the result as a
 * Guacamole layer.
 */
typedef struct guac_rdp_rail_window {

    /**
     * The RDP server-side ID of the RAIL window.
     */
    UINT64 window_id;

    /**
     * The RDPGFX surface ID currently mapped to this window, if known.
     */
    UINT16 surface_id;

    /**
     * Whether surface_id contains a valid mapped RDPGFX surface ID.
     */
    int has_surface_id;

    /**
     * The RDP server-side ID of the owning RAIL window, if any.
     */
    UINT64 owner_window_id;

    /**
     * The Guacamole layer that renders the contents of this window.
     */
    guac_display_layer* layer;

    /**
     * Whether a display layer allocation is already in progress for this
     * window.
     */
    int layer_pending;

    /**
     * Optional Guacamole layer used only when this window paints a surface
     * whose pixel format explicitly includes alpha.
     */
    guac_display_layer* alpha_layer;

    /**
     * Whether the currently active backing layer preserves alpha.
     */
    int layer_alpha;

    /**
     * The x coordinate of this window within the remote desktop.
     */
    int x;

    /**
     * The y coordinate of this window within the remote desktop.
     */
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
     * Whether an RDPGFX surface update arrived before the RAIL window order.
     */
    int has_deferred_surface;

    /**
     * Whether a RAIL WindowCreate/Update order has been observed for this
     * window. Layers created speculatively from a surface update (before any
     * RAIL order arrives) are held off-screen until this is set.
     */
    int has_rail_order;

    /**
     * Whether this window was included in the most recent server-provided
     * monitored desktop z-order list. Used to avoid applying the raise to 
     * windows that already have authoritative z-order.
     * We obey the server provided z-order, whenever it is available.
     */
    int in_monitored_desktop_zorder;

    /**
     * Whether this window has already been raised due to receiving its first
     * visible paint after its RAIL order was known. If the window does not yet 
     * have authoritative z-order, we apply this fallback raise only once.
     */
    int painted_raise_done;

    /**
    * The last z-order assigned to the Guacamole layer for this window.
    *
    * Guacamole layer stacking is managed locally, separately from the server's
    * RAIL window IDs. Tracking the assigned z-order lets us find the current
    * topmost layer and raise another window above it without relying on list
    * order or window IDs.
    */
    int z_order;

    /**
     * The current width of the Guacamole layer.
     */
    int layer_width;

    /**
     * The current height of the Guacamole layer.
     */
    int layer_height;

    /**
     * The latest window width reported by RAIL window orders.
     */
    int order_width;

    /**
     * The latest window height reported by RAIL window orders.
     */
    int order_height;

    /**
     * The latest client-area x coordinate reported by RAIL window orders.
     */
    int client_x;

    /**
     * The latest client-area y coordinate reported by RAIL window orders.
     */
    int client_y;

    /**
     * The latest client-area width reported by RAIL window orders.
     */
    int client_width;

    /**
     * The latest client-area height reported by RAIL window orders.
     */
    int client_height;

    /**
     * Whether client-area bounds have been reported by RAIL window orders.
     */
    int has_client_area;

    /**
     * The width of the mapped RDPGFX surface represented by the layer.
     */
    int surface_width;

    /**
     * The height of the mapped RDPGFX surface represented by the layer.
     */
    int surface_height;

    /**
     * Number of RDPGFX paint operations currently using this window's layer.
     */
    int paint_refs;

    /**
     * Whether this window has been removed from the tracked window list and
     * should be freed once all outstanding paints finish.
     */
    int deleted;

} guac_rdp_rail_window;

#ifdef HAVE_RDPGFX_SURFACE_AREA_UPDATE
static UINT guac_rdp_rail_paint_surface(guac_client* client,
        gdiGfxSurface* surface, UINT32 rect_count, const RECTANGLE_16* rects);
#endif

static void guac_rdp_rail_apply_window_layers(
        guac_rdp_client* rdp_client);

/**
 * Returns whether RAIL window surfaces should be rendered through Guacamole
 * layers for the given RDP client.
 *
 * RemoteApp windows are rendered as separate Guacamole layers only when both
 * RemoteApp and RDPGFX are enabled. If GFX is disabled, rendering falls back to
 * the normal GDI/primary-buffer path and the RAIL layer tracking code should
 * stay inactive.
 *
 * @param rdp_client
 *     The RDP client whose settings should be checked.
 *
 * @return
 *     Non-zero if RAIL window surfaces should be rendered through Guacamole layers.
 */
static int guac_rdp_rail_uses_gfx(guac_rdp_client* rdp_client) {
    return rdp_client->settings != NULL
        && rdp_client->settings->remote_app != NULL
        && rdp_client->settings->enable_gfx
        && rdp_client->rdpgfx_interface != NULL;
}

/**
 * Updates the RemoteApp window container layer to cover the default layer.
 *
 * @param rdp_client
 *     The RDP client whose RemoteApp window container should be resized.
 */
static void guac_rdp_rail_update_window_layer_bounds(
        guac_rdp_client* rdp_client) {

    if (rdp_client->rail_window_layer == NULL)
        return;

    guac_display_layer* default_layer =
        guac_display_default_layer(rdp_client->display);

    guac_rect bounds;
    guac_display_layer_get_bounds(default_layer, &bounds);

    int width = guac_rect_width(&bounds);
    int height = guac_rect_height(&bounds);

    if (width > 0 && height > 0)
        guac_display_layer_resize(rdp_client->rail_window_layer,
                width, height);

}

/**
 * Returns the layer used as the common parent for all RemoteApp window layers,
 * creating it if necessary. This additional layer keeps RemoteApp window
 * z-order local to the window group, preventing those windows from stacking
 * above Guacamole's cursor layer.
 *
 * @param client
 *     The Guacamole client associated with the RDP session.
 *
 * @return
 *     The RemoteApp window container layer, or NULL if it could not be
 *     allocated.
 */
static guac_display_layer* guac_rdp_rail_get_window_layer(
        guac_client* client) {

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    if (rdp_client->rail_window_layer != NULL) {
        guac_rdp_rail_update_window_layer_bounds(rdp_client);
        return rdp_client->rail_window_layer;
    }

    guac_display_layer* layer =
        guac_display_alloc_layer(rdp_client->display, 0);

    if (layer == NULL) {
        guac_client_log(client, GUAC_LOG_WARNING, "RAIL window container "
                "could not be allocated. Window contents may not render.");
        return NULL;
    }

    guac_display_layer_set_parent(layer,
            guac_display_default_layer(rdp_client->display));
    guac_display_layer_move(layer, 0, 0);
    guac_display_layer_stack(layer, 0);

    rdp_client->rail_window_layer = layer;
    guac_rdp_rail_update_window_layer_bounds(rdp_client);

    return layer;

}

/**
 * Returns the RAIL window having the given ID, or NULL if no such window is
 * being tracked. The rail_windows list lock must be held.
 *
 * @param rdp_client
 *     The RDP client instance whose tracked RAIL windows should be searched.
 *
 * @param window_id
 *     The server-side ID of the RAIL window to retrieve.
 *
 * @return
 *     The matching RAIL window, or NULL if no such window is being tracked.
 */
static guac_rdp_rail_window* guac_rdp_rail_get_window(
        guac_rdp_client* rdp_client, UINT64 window_id) {

    guac_common_list_element* current = rdp_client->rail_windows->head;

    while (current != NULL) {
        guac_rdp_rail_window* rail_window = current->data;

        if (rail_window != NULL
                && rail_window->window_id == window_id)
            return rail_window;

        current = current->next;
    }

    return NULL;
}

#ifdef HAVE_RDPGFX_SURFACE_AREA_UPDATE
/**
 * Returns the tracked RAIL window currently mapped to the given RDPGFX
 * surface, or NULL if no such mapping is known. The rail_windows list lock
 * must be held.
 *
 * @param rdp_client
 *     The RDP client whose tracked RAIL windows should be searched.
 *
 * @param surface_id
 *     The server-side ID of the RDPGFX surface to look up.
 *
 * @return
 *     The matching tracked RAIL window, or NULL if no such mapping exists.
 */
static guac_rdp_rail_window* guac_rdp_rail_get_window_for_surface(
        guac_rdp_client* rdp_client, UINT16 surface_id) {

    guac_common_list_element* current = rdp_client->rail_windows->head;

    while (current != NULL) {
        guac_rdp_rail_window* rail_window = current->data;

        if (rail_window != NULL
                && rail_window->has_surface_id
                && rail_window->surface_id == surface_id)
            return rail_window;

        current = current->next;
    }

    return NULL;
}
#endif

/**
 * Returns the list element containing the RAIL window having the given ID, or
 * NULL if no such window is being tracked. The rail_windows list lock must be
 * held.
 *
 * @param rdp_client
 *     The RDP client instance whose tracked RAIL windows should be searched.
 *
 * @param window_id
 *     The server-side ID of the RAIL window to retrieve.
 *
 * @return
 *     The list element containing the matching RAIL window, or NULL if no
 *     such window is being tracked.
 */
static guac_common_list_element* guac_rdp_rail_get_window_element(
        guac_rdp_client* rdp_client, UINT64 window_id) {

    guac_common_list_element* current = rdp_client->rail_windows->head;

    while (current != NULL) {
        guac_rdp_rail_window* rail_window = current->data;

        if (rail_window != NULL
                && rail_window->window_id == window_id)
            return current;

        current = current->next;
    }

    return NULL;
}

/**
 * Updates the stacking order tracked for the given RAIL
 * window. The rail_windows list lock must be held.
 *
 * @param rail_window
 *     The RAIL window whose tracked stacking order should be updated.
 *
 * @param z
 *     The new stacking order.
 */
static void guac_rdp_rail_stack_window(guac_rdp_rail_window* rail_window,
        int z) {

    rail_window->z_order = z;

}

/**
 * Snapshot of display-layer state to apply outside the rail_windows lock.
 */
typedef struct guac_rdp_rail_layer_update {

    guac_display_layer* layer;
    guac_display_layer* alpha_layer;

    int x;
    int y;
    int z_order;
    int opacity;
    int layer_alpha;

} guac_rdp_rail_layer_update;

#ifdef HAVE_RDPGFX_SURFACE_AREA_UPDATE
/**
 * Returns the optional alpha-capable layer for the given RAIL window,
 * creating it if necessary. The rail_windows list lock must be held.
 *
 * @param client
 *     The Guacamole client associated with the current RDP session.
 *
 * @param rail_window
 *     The RAIL window whose alpha-capable layer should be retrieved.
 *
 * @return
 *     The alpha-capable layer, or NULL if it could not be allocated.
 */
static guac_display_layer* guac_rdp_rail_get_alpha_layer(
        guac_client* client, guac_rdp_rail_window* rail_window) {

    if (rail_window->alpha_layer != NULL)
        return rail_window->alpha_layer;

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    guac_display_layer* parent_layer =
        guac_rdp_rail_get_window_layer(client);

    if (parent_layer == NULL)
        return NULL;

    guac_display_layer* layer =
        guac_display_alloc_layer(rdp_client->display, 0);

    if (layer == NULL)
        return NULL;

    guac_display_layer_set_parent(layer, parent_layer);
    guac_display_layer_move(layer, rail_window->x, rail_window->y);
    guac_display_layer_stack(layer, rail_window->z_order);
    guac_display_layer_set_opacity(layer, 0);

    if (rail_window->layer_width > 0 && rail_window->layer_height > 0)
        guac_display_layer_resize(layer,
                rail_window->layer_width, rail_window->layer_height);

    rail_window->alpha_layer = layer;
    return layer;

}
#endif

/**
 * Returns a z-order value above all currently tracked RAIL windows. The
 * rail_windows list lock must be held.
 *
 * @param rdp_client
 *     The RDP client whose tracked windows should be counted.
 *
 * @return
 *     A z-order value above all currently tracked RAIL windows.
 */
static int guac_rdp_rail_get_top_z(guac_rdp_client* rdp_client) {

    int z = 1;

    for (guac_common_list_element* current = rdp_client->rail_windows->head;
            current != NULL; current = current->next) {
        guac_rdp_rail_window* rail_window = current->data;

        if (rail_window != NULL
                && rail_window->z_order >= z)
            z = rail_window->z_order + 1;
    }

    return z;

}

/**
 * Raises all tracked windows owned by the given parent window above that
 * parent's layer. The rail_windows list lock must be held.
 * This is needed for showing popup/dialog windows owned by another window,
 * which might otherwise stay hidden behind their owner or other RemoteApp windows.
 *
 * @param rdp_client
 *     The RDP client whose tracked windows should be searched.
 *
 * @param owner_window_id
 *     The server-side ID of the owner window.
 *
 * @param z
 *     The z-order to assign to the first owned window.
 *
 * @return
 *     The next available z-order after all owned windows have been raised.
 */
static int guac_rdp_rail_raise_owned_windows(guac_rdp_client* rdp_client,
        UINT64 owner_window_id, int z) {

    guac_common_list_element* current = rdp_client->rail_windows->head;

    while (current != NULL) {
        guac_rdp_rail_window* rail_window = current->data;

        if (rail_window != NULL
                && rail_window->has_rail_order
                && rail_window->owner_window_id == owner_window_id
                && rail_window->window_id != owner_window_id) {
            guac_rdp_rail_stack_window(rail_window, z++);
            z = guac_rdp_rail_raise_owned_windows(rdp_client,
                    rail_window->window_id, z);
        }

        current = current->next;
    }

    return z;

}

/**
 * Raises the given window above its owner, if that owner is currently being
 * tracked. The rail_windows list lock must be held.
 *
 * @param rdp_client
 *     The RDP client whose tracked windows should be searched.
 *
 * @param rail_window
 *     The window that should be raised above its owner.
 */
static void guac_rdp_rail_raise_window_above_owner(
        guac_rdp_client* rdp_client, guac_rdp_rail_window* rail_window) {

    if (rail_window == NULL || rail_window->owner_window_id == 0)
        return;

    guac_rdp_rail_window* owner = guac_rdp_rail_get_window(rdp_client,
            rail_window->owner_window_id);

    if (owner == NULL)
        return;

    int z = guac_rdp_rail_get_top_z(rdp_client);

    guac_rdp_rail_stack_window(rail_window, z);

}

/**
 * Sends a RAIL Client Activate order for the given window.
 *
 * RAIL servers expect explicit activation notifications when the active
 * RemoteApp window changes.
 *
 * Even if the window_id is 64 bits, MS RAIL protocol activation orders use a 32-bit window ID.
 * Ignore any invalid window IDs that cannot be represented without truncating to 32-bits.
 * *
 * @param rdp_client
 *     The RDP client whose RAIL channel should be used.
 *
 * @param window_id
 *     The server-side ID of the window to activate or deactivate.
 *
 * @param enabled
 *     Non-zero to activate the window, zero to deactivate it.
 *
 * @return
 *     CHANNEL_RC_OK if the activation order was sent successfully, or a
 *     protocol-specific error code otherwise.
 */
static UINT guac_rdp_rail_send_activate(guac_rdp_client* rdp_client,
        UINT64 window_id, BOOL enabled) {

    if (rdp_client->rail_interface == NULL
            || rdp_client->rail_interface->ClientActivate == NULL)
        return CHANNEL_RC_OK;

    if (window_id > UINT32_MAX)
        return CHANNEL_RC_OK;

    RAIL_ACTIVATE_ORDER activate = {
        .windowId = (UINT32) window_id,
        .enabled = enabled
    };

    pthread_mutex_lock(&(rdp_client->message_lock));
    UINT status = rdp_client->rail_interface->ClientActivate(
            rdp_client->rail_interface, &activate);
    pthread_mutex_unlock(&(rdp_client->message_lock));

    return status;

}

/**
 * Sends the RAIL activation to switch focus to the given window.
 *
 * The previously active window is first deactivated, followed by activation
 * of the requested window. The locally tracked active window ID is updated
 * only if the activation request succeeds.
 *
 * @param rdp_client
 *     The RDP client whose RAIL channel should be used.
 *
 * @param window_id
 *     The server-side ID of the RAIL window that should be activated.
 */
static void guac_rdp_rail_activate_window(guac_rdp_client* rdp_client,
        UINT64 window_id) {

    if (rdp_client->rail_active_window_id == window_id)
        return;

    if (guac_rdp_rail_send_activate(rdp_client, window_id, TRUE)
            == CHANNEL_RC_OK)
        rdp_client->rail_active_window_id = window_id;

}

void guac_rdp_rail_raise_window_at(guac_rdp_client* rdp_client,
        int x, int y) {

    if (!guac_rdp_rail_uses_gfx(rdp_client))
        return;

    if (rdp_client->rail_windows == NULL)
        return;

    guac_common_list_lock(rdp_client->rail_windows);

    guac_rdp_rail_window* clicked_window = NULL;
    UINT64 activate_window_id = 0;
    int clicked_z = 0;
    int apply_layer_updates = 0;
    int has_no_surface_window = 0;
    int highest_no_surface_z = 0;
    int suppress_activation = 0;
    int has_adjacent_nosurface_window = 0;

    /* Find the topmost painted window under the click, while separately
     * tracking visible no-surface windows, which may be a menu
     * popups whose drawable surface has not arrived yet. */
    for (guac_common_list_element* current = rdp_client->rail_windows->head;
            current != NULL; current = current->next) {

        guac_rdp_rail_window* rail_window = current->data;
        if (rail_window == NULL || rail_window->layer == NULL)
            continue;

        /* Track the highest visible RAIL window still waiting for its drawable surface, 
           as it may be a transient popup. */
        if (rail_window->visible && rail_window->has_rail_order
                && !rail_window->has_surface) {

            if (!has_no_surface_window
                    || rail_window->z_order > highest_no_surface_z) {
                highest_no_surface_z = rail_window->z_order;
                has_no_surface_window = 1;
            }

            continue;
        }

        /* Verify the window is fully renderable before continuing */
        if (!rail_window->visible || !rail_window->has_surface
                || !rail_window->has_rail_order)
            continue;

        /* Verify the click is within window borders */
        if (x < rail_window->x || y < rail_window->y
                || x >= rail_window->x + rail_window->layer_width
                || y >= rail_window->y + rail_window->layer_height)
            continue;

        /* Use the topmost matching window */
        if (clicked_window == NULL || rail_window->z_order > clicked_z) {
            clicked_window = rail_window;
            clicked_z = rail_window->z_order;
        }

    }

    if (clicked_window != NULL) {
        int z = guac_rdp_rail_get_top_z(rdp_client);

        /* A higher no-surface RAIL window can represent a menu before its
         * drawable popup surface arrives. Avoid reactivating the window below
         * it, or the menu may close before the click is delivered. */
        if (has_no_surface_window
                && highest_no_surface_z > clicked_window->z_order) {
            suppress_activation = 1;
        }

        /* Right-click context menus may not report an owner window, but in
         * that case they're stacked immediately above a visible no-surface
         * RAIL window (surface and window are separate). Treat that pairing
         * as a menu popup and skip pre-click activation so menu-item clicks
         * are not dismissed. */
        if (has_no_surface_window
                && highest_no_surface_z == clicked_window->z_order - 1) {
            suppress_activation = 1;
            has_adjacent_nosurface_window = 1;
        }

        /* Regular menu popups are owned windows. Activating them before the
         * click can dismiss or disturb the menu selection. */
        if (clicked_window->owner_window_id != 0) {
            suppress_activation = 1;
        }

        if (!suppress_activation)
            activate_window_id = clicked_window->window_id;

        /* Locally raise the clicked window unless server z-order or 
           transient menu state should take precedence. */
        if (!has_adjacent_nosurface_window
                && !clicked_window->in_monitored_desktop_zorder
                && clicked_window->z_order < z - 1) {
            guac_rdp_rail_stack_window(clicked_window, z);
            guac_rdp_rail_raise_owned_windows(rdp_client,
                    clicked_window->window_id, z + 1);
            apply_layer_updates = 1;
            rdp_client->gdi_modified = 1;
        }
    }

    guac_common_list_unlock(rdp_client->rail_windows);

    if (apply_layer_updates)
        guac_rdp_rail_apply_window_layers(rdp_client);

    if (activate_window_id != 0)
        guac_rdp_rail_activate_window(rdp_client, activate_window_id);

}

void guac_rdp_rail_track_mouse_event(guac_rdp_client* rdp_client,
        int mask, int x, int y) {

    if (!guac_rdp_rail_uses_gfx(rdp_client))
        return;

    if (rdp_client->rail_windows == NULL)
        return;

    guac_common_list_lock(rdp_client->rail_windows);

    guac_rdp_rail_window* target_window = NULL;

     /* Preserve the original mouse-down target while a drag is in progress. */
    if (rdp_client->mouse_button_mask != 0
            && rdp_client->rail_mouse_window_id != 0)
        target_window = guac_rdp_rail_get_window(rdp_client,
                rdp_client->rail_mouse_window_id);

    if (target_window == NULL) {

        int target_z = 0;

        /* If no drag target is active, find the topmost renderable window
         * currently under the pointer. */
        for (guac_common_list_element* current = rdp_client->rail_windows->head;
                current != NULL; current = current->next) {

            guac_rdp_rail_window* rail_window = current->data;
            if (rail_window == NULL)
                continue;

            if (!rail_window->visible || !rail_window->has_surface
                    || !rail_window->has_rail_order)
                continue;

            if (x < rail_window->x || y < rail_window->y
                    || x >= rail_window->x + rail_window->layer_width
                    || y >= rail_window->y + rail_window->layer_height)
                continue;

            if (target_window == NULL || rail_window->z_order > target_z) {
                target_window = rail_window;
                target_z = rail_window->z_order;
            }

        }

    }

    /* Capture the target on mouse down and release it once all buttons are up. */
    if (target_window != NULL
            && rdp_client->mouse_button_mask == 0 && mask != 0)
        rdp_client->rail_mouse_window_id = target_window->window_id;

    if (mask == 0)
        rdp_client->rail_mouse_window_id = 0;

    guac_common_list_unlock(rdp_client->rail_windows);

}

/**
 * Returns the RAIL window state having the given ID, creating it if necessary.
 * The rail_windows list lock must be held.
 *
 * @param client
 *     The Guacamole client associated with the RDP session.
 *
 * @param window_id
 *     The server-side ID of the RAIL window to retrieve or create.
 *
 * @param new_window
 *     Preallocated window state to use if no matching window exists, or NULL
 *     if allocation failed.
 *
 * @param new_element
 *     Preallocated list element to use if no matching window exists, or NULL
 *     if allocation failed.
 *
 * @return
 *     The matching or newly-created RAIL window state, or NULL if the state
 *     could not be created.
 */
static guac_rdp_rail_window* guac_rdp_rail_get_or_create_window(
        guac_client* client, UINT64 window_id,
        guac_rdp_rail_window* new_window,
        guac_common_list_element* new_element) {

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    guac_rdp_rail_window* rail_window =
        guac_rdp_rail_get_window(rdp_client, window_id);

    if (rail_window != NULL)
        return rail_window;

    if (new_window == NULL || new_element == NULL)
        return NULL;

    new_window->window_id = window_id;
    new_window->visible = 1;

    new_element->data = new_window;
    new_element->next = rdp_client->rail_windows->head;
    new_element->_ptr = &(rdp_client->rail_windows->head);

    if (rdp_client->rail_windows->head != NULL)
        rdp_client->rail_windows->head->_ptr = &(new_element->next);

    rdp_client->rail_windows->head = new_element;

    return new_window;

}

/**
 * Ensures the given RAIL window has a display layer. The rail_windows list lock
 * must NOT be held.
 *
 * @param client
 *     The Guacamole client associated with the RDP session.
 *
 * @param window_id
 *     The server-side ID of the RAIL window whose layer should exist.
 *
 * @return
 *     Non-zero if a layer was allocated and attached, zero otherwise.
 */
static int guac_rdp_rail_ensure_window_layer(
        guac_client* client, UINT64 window_id) {

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    guac_common_list_lock(rdp_client->rail_windows);

    guac_rdp_rail_window* rail_window =
        guac_rdp_rail_get_window(rdp_client, window_id);

    if (rail_window == NULL || rail_window->layer != NULL
            || rail_window->layer_pending || rail_window->deleted) {
        guac_common_list_unlock(rdp_client->rail_windows);
        return 0;
    }

    rail_window->layer_pending = 1;

    guac_common_list_unlock(rdp_client->rail_windows);

    guac_display_layer* parent_layer =
        guac_rdp_rail_get_window_layer(client);

    guac_display_layer* layer = NULL;
    if (parent_layer != NULL) {
        layer = guac_display_alloc_layer(rdp_client->display, 1);

        if (layer != NULL) {
            guac_display_layer_set_parent(layer, parent_layer);
            guac_display_layer_set_opacity(layer, 0);
        }
    }

    guac_common_list_lock(rdp_client->rail_windows);

    rail_window = guac_rdp_rail_get_window(rdp_client, window_id);
    int attached = 0;

    if (rail_window != NULL && !rail_window->deleted
            && rail_window->layer == NULL && layer != NULL) {
        rail_window->layer = layer;
        rail_window->layer_pending = 0;
        layer = NULL;
        attached = 1;
    }
    else if (rail_window != NULL)
        rail_window->layer_pending = 0;

    guac_common_list_unlock(rdp_client->rail_windows);

    if (layer != NULL)
        guac_display_layer_set_opacity(layer, 0);

    return attached;

}

/**
 * Updates the visibility of the Guacamole layer backing the given RAIL window.
 * The rail_windows list lock must be held.
 *
 * @param rail_window
 *     The RAIL window whose backing Guacamole layer should have its opacity updated.
 */
static void guac_rdp_rail_update_window_opacity(
        guac_rdp_client* rdp_client,
        guac_rdp_rail_window* rail_window) {

    (void) rail_window;
    rdp_client->gdi_modified = 1;

}

/**
 * Applies all tracked RAIL window layer state to the corresponding Guacamole
 * display layers. The rail_windows list lock must NOT be held.
 *
 * @param rdp_client
 *     The RDP client whose tracked RAIL window layers should be updated.
 */
static void guac_rdp_rail_apply_window_layers(
        guac_rdp_client* rdp_client) {

    if (rdp_client->rail_windows == NULL)
        return;

    guac_common_list_lock(rdp_client->rail_windows);

    size_t window_count = 0;
    for (guac_common_list_element* current = rdp_client->rail_windows->head;
            current != NULL; current = current->next) {
        if (current->data != NULL)
            window_count++;
    }

    guac_common_list_unlock(rdp_client->rail_windows);

    if (window_count == 0)
        return;

    guac_rdp_rail_layer_update* updates =
        guac_mem_zalloc(sizeof(guac_rdp_rail_layer_update), window_count);

    if (updates == NULL)
        return;

    guac_common_list_lock(rdp_client->rail_windows);

    size_t update_count = 0;
    for (guac_common_list_element* current = rdp_client->rail_windows->head;
            current != NULL && update_count < window_count;
            current = current->next) {

        guac_rdp_rail_window* rail_window = current->data;
        if (rail_window == NULL || rail_window->layer == NULL)
            continue;

        int opacity = (rail_window->visible
                && rail_window->has_surface
                && rail_window->has_rail_order) ? 0xFF : 0;

        updates[update_count++] = (guac_rdp_rail_layer_update) {
            .layer = rail_window->layer,
            .alpha_layer = rail_window->alpha_layer,
            .x = rail_window->x,
            .y = rail_window->y,
            .z_order = rail_window->z_order,
            .opacity = opacity,
            .layer_alpha = rail_window->layer_alpha
        };

    }

    guac_common_list_unlock(rdp_client->rail_windows);

    for (size_t i = 0; i < update_count; i++) {

        guac_rdp_rail_layer_update* update = &updates[i];

        guac_display_layer_move(update->layer, update->x, update->y);
        guac_display_layer_stack(update->layer, update->z_order);
        guac_display_layer_set_opacity(update->layer,
                update->layer_alpha ? 0 : update->opacity);

        if (update->alpha_layer != NULL) {
            guac_display_layer_move(update->alpha_layer,
                    update->x, update->y);
            guac_display_layer_stack(update->alpha_layer,
                    update->z_order);
            guac_display_layer_set_opacity(update->alpha_layer,
                    update->layer_alpha ? update->opacity : 0);
        }

    }

    guac_mem_free(updates);

}

/**
 * Marks the full contents of the given display layer dirty so the next
 * Guacamole frame flush repaints it. This is used when a window becomes
 * visible or receives its first authoritative RAIL order after surface data
 * has already arrived.
 *
 * @param rdp_client
 *     The RDP client whose frame state should be marked modified.
 *
 * @param layer
 *     The display layer that should be repainted.
 */
static void guac_rdp_rail_mark_layer_dirty(guac_rdp_client* rdp_client,
        guac_display_layer* layer) {

    guac_display_layer_raw_context* layer_context =
        guac_display_layer_open_raw(layer);

    if (layer_context == NULL)
        return;

    guac_rect_extend(&layer_context->dirty, &layer_context->bounds);
    guac_display_layer_close_raw(layer, layer_context);

    rdp_client->gdi_modified = 1;

}

#ifdef HAVE_RDPGFX_SURFACE_AREA_UPDATE
/**
 * Repaints the full contents of the given mapped RDPGFX surface through the
 * RAIL paint path. This is used when a surface was known before the
 * corresponding RAIL order made the window visible/renderable.
 *
 * @param client
 *     The Guacamole client associated with the current RDP session.
 *
 * @param surface_id
 *     The server-side ID of the RDPGFX surface that should be repainted.
 *
 * @return
 *     CHANNEL_RC_OK if the repaint was ignored or completed successfully, or
 *     an error from the shared surface-paint path otherwise.
 */
static UINT guac_rdp_rail_repaint_surface(guac_client* client,
        UINT16 surface_id) {

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    RdpgfxClientContext* rdpgfx = rdp_client->rdpgfx_interface;

    if (rdpgfx == NULL || rdpgfx->GetSurfaceData == NULL)
        return CHANNEL_RC_OK;

    gdiGfxSurface* surface =
        (gdiGfxSurface*) rdpgfx->GetSurfaceData(rdpgfx, surface_id);

    if (surface == NULL)
        return CHANNEL_RC_OK;

    UINT32 width = surface->mappedWidth > 0
        ? surface->mappedWidth : surface->width;
    UINT32 height = surface->mappedHeight > 0
        ? surface->mappedHeight : surface->height;

    if (width == 0 || height == 0
            || width > UINT16_MAX || height > UINT16_MAX)
        return CHANNEL_RC_OK;

    RECTANGLE_16 full_rect = {
        .left = 0,
        .top = 0,
        .right = (UINT16) width,
        .bottom = (UINT16) height
    };

    return guac_rdp_rail_paint_surface(client, surface, 1, &full_rect);

}
#endif

/**
 * Paints an opaque black background into the default desktop layer once
 * RemoteApp windows are about to become visible. This prevents transparent or
 * uninitialized pixels from showing through behind per-window RAIL layers.
 *
 * @param rdp_client
 *     The RDP client whose default display layer should be initialized.
 */
static void guac_rdp_rail_paint_background(guac_rdp_client* rdp_client) {

    if (rdp_client->rail_background_painted)
        return;

    guac_display_layer* default_layer =
        guac_display_default_layer(rdp_client->display);

    guac_display_layer_raw_context* bg_ctx =
        guac_display_layer_open_raw(default_layer);

    if (bg_ctx == NULL)
        return;

    guac_rect full_bounds = bg_ctx->bounds;
    int height = guac_rect_height(&full_bounds);
    int width = guac_rect_width(&full_bounds);

    unsigned char* row =
        GUAC_DISPLAY_LAYER_RAW_BUFFER(bg_ctx, full_bounds);

    for (int y = 0; y < height; y++) {
        unsigned char* pixel = row;
        for (int x = 0; x < width; x++) {
            pixel[0] = 0x00;
            pixel[1] = 0x00;
            pixel[2] = 0x00;
            pixel[3] = 0xFF;
            pixel += 4;
        }
        row += bg_ctx->stride;
    }

    guac_rect_extend(&bg_ctx->dirty, &full_bounds);
    guac_display_layer_close_raw(default_layer, bg_ctx);

    rdp_client->rail_background_painted = 1;
    rdp_client->gdi_modified = 1;

}

/**
 * Frees the resources associated with the given RAIL window.
 *
 * @param data
 *     The RAIL window to free.
 */
static void guac_rdp_rail_free_window_data(void* data) {

    guac_rdp_rail_window* rail_window = data;
    if (rail_window == NULL)
        return;

    if (rail_window->layer != NULL)
        guac_display_free_layer(rail_window->layer);

    if (rail_window->alpha_layer != NULL)
        guac_display_free_layer(rail_window->alpha_layer);

    guac_mem_free(rail_window);
}

#ifdef HAVE_RDPGFX_SURFACE_AREA_UPDATE
/**
 * Forces the alpha channel of all pixels within the given rectangle of the
 * given raw display layer context to fully opaque.
 *
 * @param context
 *     The raw display layer context whose pixels should be made opaque.
 *
 * @param rect
 *     The rectangle of pixels that should be made opaque.
 */
static void guac_rdp_rail_force_opaque(
        guac_display_layer_raw_context* context, const guac_rect* rect) {

    int width = guac_rect_width(rect);
    int height = guac_rect_height(rect);

    unsigned char* row =
        GUAC_DISPLAY_LAYER_RAW_BUFFER(context, *rect);

    for (int y = 0; y < height; y++) {

        uint32_t* pixel = (uint32_t*) row;

        for (int x = 0; x < width; x++)
            pixel[x] |= 0xFF000000;

        row += context->stride;

    }

}

/**
 * Releases one active paint reference for the given RAIL window.
 *
 * Paint operations temporarily outlive membership in the rail_windows list, so
 * deleted windows are freed only after their last outstanding paint completes.
 * The rail_windows list lock must be held. If this returns non-zero, the
 * caller must free the window after releasing the list lock.
 *
 * @param rail_window
 *     The tracked RAIL window whose outstanding paint count should be
 *     decremented.
 *
 * @return
 *     Non-zero if the window has been deleted and no more paint references
 *     remain, meaning the caller should free it after unlocking the list.
 */
static int guac_rdp_rail_release_paint_ref(
        guac_rdp_rail_window* rail_window) {

    rail_window->paint_refs--;

    return rail_window->deleted && rail_window->paint_refs == 0;

}
#endif

/**
 * Callback invoked if the RDP server requests client-side local movement or
 * resizing of a RemoteApp window.
 *
 * @param rail
 *     The RailClientContext structure used by FreeRDP to handle the RAIL
 *     channel for the current RDP session.
 *
 * @param localMoveSize
 *     The local move/size request sent by the server.
 *
 * @return
 *     CHANNEL_RC_OK (zero) as the order has been ignored successfully.
 */
static UINT guac_rdp_rail_local_move_size(RailClientContext* rail,
        const RAIL_LOCALMOVESIZE_ORDER* localMoveSize) {

    guac_client* client = (guac_client*) rail->custom;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    if (localMoveSize == NULL) {
        guac_client_log(client, GUAC_LOG_DEBUG,
                "RAIL local move/size ignored: null order.");
        return CHANNEL_RC_OK;
    }

    if (rail->ClientWindowMove == NULL)
        return CHANNEL_RC_OK;

    if (localMoveSize->windowId > UINT32_MAX)
        return CHANNEL_RC_OK;

    guac_common_list_lock(rdp_client->rail_windows);

    guac_rdp_rail_window* rail_window =
        guac_rdp_rail_get_window(rdp_client, localMoveSize->windowId);

    RAIL_WINDOW_MOVE_ORDER windowMove = { 0 };
    int send_move = 0;

    if (rail_window != NULL) {

        int width = rail_window->order_width > 0
            ? rail_window->order_width : rail_window->layer_width;
        int height = rail_window->order_height > 0
            ? rail_window->order_height : rail_window->layer_height;

        int right = rail_window->x + width;
        int bottom = rail_window->y + height;

        if (width > 0 && height > 0
                && rail_window->x >= INT16_MIN && rail_window->x <= INT16_MAX
                && rail_window->y >= INT16_MIN && rail_window->y <= INT16_MAX
                && right >= INT16_MIN && right <= INT16_MAX
                && bottom >= INT16_MIN && bottom <= INT16_MAX) {
            windowMove.windowId = (UINT32) localMoveSize->windowId;
            windowMove.left = (INT16) rail_window->x;
            windowMove.top = (INT16) rail_window->y;
            windowMove.right = (INT16) right;
            windowMove.bottom = (INT16) bottom;
            send_move = 1;
        }

    }

    guac_common_list_unlock(rdp_client->rail_windows);

    if (!send_move) {
        guac_client_log(client, GUAC_LOG_DEBUG,
                "RAIL local move/size completion skipped: "
                "window=0x%08x has no usable bounds.",
                (unsigned int) localMoveSize->windowId);
        return CHANNEL_RC_OK;
    }

    pthread_mutex_lock(&(rdp_client->message_lock));
    UINT status = rail->ClientWindowMove(rail, &windowMove);
    pthread_mutex_unlock(&(rdp_client->message_lock));

    return status;

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

    UINT32 fieldFlags = orderInfo->fieldFlags;

#ifdef HAVE_RDPGFX_SURFACE_AREA_UPDATE
    int repaint_surface = 0;
    UINT16 repaint_surface_id = 0;
#endif
    int paint_background = 0;
    int apply_layer_updates = 0;
    int ensure_window_layer = 0;
    guac_display_layer* dirty_layer = NULL;

    /* Track window location and visibility for RDPGFX RemoteApp rendering. */
    if (guac_rdp_rail_uses_gfx(rdp_client)) {

        guac_rdp_rail_window* new_window =
            guac_mem_zalloc(sizeof(guac_rdp_rail_window));
        guac_common_list_element* new_element =
            guac_mem_alloc(sizeof(guac_common_list_element));

        guac_common_list_lock(rdp_client->rail_windows);

        guac_rdp_rail_window* rail_window =
            guac_rdp_rail_get_or_create_window(client, orderInfo->windowId,
                    new_window, new_element);

        if (rail_window == new_window) {
            new_window = NULL;
            new_element = NULL;
        }

        if (rail_window != NULL) {

            ensure_window_layer = rail_window->layer == NULL;

            int had_rail_order = rail_window->has_rail_order;
#ifdef HAVE_RDPGFX_SURFACE_AREA_UPDATE
            int was_visible = rail_window->visible;
#endif
            int needs_opacity_update = !had_rail_order;
            rail_window->has_rail_order = 1;
            int needs_raise = 0;

            if (fieldFlags & WINDOW_ORDER_FIELD_OWNER) {
                rail_window->owner_window_id = windowState->ownerWindowId;
                needs_raise = 1;
            }

            if (fieldFlags & WINDOW_ORDER_FIELD_WND_OFFSET) {
                rail_window->x = windowState->windowOffsetX;
                rail_window->y = windowState->windowOffsetY;
                apply_layer_updates = 1;
                rdp_client->gdi_modified = 1;
            }

            if (fieldFlags & WINDOW_ORDER_FIELD_WND_SIZE) {
                rail_window->order_width = windowState->windowWidth;
                rail_window->order_height = windowState->windowHeight;
            }

            if (fieldFlags & WINDOW_ORDER_FIELD_CLIENT_AREA_OFFSET) {
                rail_window->client_x = windowState->clientOffsetX;
                rail_window->client_y = windowState->clientOffsetY;
            }

            if (fieldFlags & WINDOW_ORDER_FIELD_CLIENT_AREA_SIZE) {
                rail_window->client_width = windowState->clientAreaWidth;
                rail_window->client_height = windowState->clientAreaHeight;
                rail_window->has_client_area =
                    rail_window->client_width > 0
                    && rail_window->client_height > 0;
            }

            if (fieldFlags & WINDOW_ORDER_FIELD_SHOW) {
                rail_window->visible =
                    windowState->showState != GUAC_RDP_RAIL_WINDOW_STATE_HIDDEN
                    && windowState->showState != GUAC_RDP_RAIL_WINDOW_STATE_MINIMIZED;
                needs_opacity_update = 1;
            }

            if (fieldFlags & WINDOW_ORDER_FIELD_ENFORCE_SERVER_ZORDER)
                needs_raise = 1;

            if (needs_raise) {
                guac_rdp_rail_raise_window_above_owner(rdp_client,
                        rail_window);
                apply_layer_updates = 1;
                rdp_client->gdi_modified = 1;
            }

            if (rail_window->has_rail_order && rail_window->visible)
                paint_background = 1;

            if (rail_window->has_surface && rail_window->has_rail_order
                    && rail_window->visible) {

                if (!had_rail_order || needs_opacity_update)
                    dirty_layer = rail_window->layer_alpha
                            && rail_window->alpha_layer != NULL
                        ? rail_window->alpha_layer
                        : rail_window->layer;

                if (!had_rail_order && rail_window->visible
                        && rail_window->owner_window_id == 0
                        && !rail_window->in_monitored_desktop_zorder) {
                    int z = guac_rdp_rail_get_top_z(rdp_client);
                    guac_rdp_rail_stack_window(rail_window, z);
                    apply_layer_updates = 1;
                    rdp_client->gdi_modified = 1;
                }
            }

            if (needs_opacity_update) {
                guac_rdp_rail_update_window_opacity(rdp_client, rail_window);
                apply_layer_updates = 1;
                rdp_client->gdi_modified = 1;
            }

#ifdef HAVE_RDPGFX_SURFACE_AREA_UPDATE
            if (rail_window->visible && rail_window->has_surface_id
                    && (!rail_window->has_surface || !was_visible)) {
                repaint_surface = 1;
                repaint_surface_id = rail_window->surface_id;
            }
#endif

        }

        guac_common_list_unlock(rdp_client->rail_windows);

        if (ensure_window_layer && guac_rdp_rail_ensure_window_layer(
                    client, orderInfo->windowId))
            apply_layer_updates = 1;

        if (apply_layer_updates)
            guac_rdp_rail_apply_window_layers(rdp_client);

        if (dirty_layer != NULL)
            guac_rdp_rail_mark_layer_dirty(rdp_client, dirty_layer);

#ifdef HAVE_RDPGFX_SURFACE_AREA_UPDATE
        if (repaint_surface)
            guac_rdp_rail_repaint_surface(client, repaint_surface_id);
#endif

        if (paint_background)
            guac_rdp_rail_paint_background(rdp_client);

        guac_mem_free(new_window);
        guac_mem_free(new_element);

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
 *
 * @param context
 *     A pointer to the rdpContext struct associated with the current RDP connection.
 * 
 * @param orderInfo
 *     A pointer to the struct containing information about the window that was deleted.
 *
 * @return
 *     TRUE if the window deletion was handled successfully, otherwise FALSE.
 */
static BOOL guac_rdp_rail_window_delete(rdpContext* context,
        RAIL_CONST WINDOW_ORDER_INFO* orderInfo) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_rdp_rail_window* free_window = NULL;

    if (!guac_rdp_rail_uses_gfx(rdp_client))
        return TRUE;

    guac_common_list_lock(rdp_client->rail_windows);

    guac_common_list_element* element =
        guac_rdp_rail_get_window_element(rdp_client, orderInfo->windowId);

    if (element != NULL) {
        guac_rdp_rail_window* rail_window = element->data;

        if (rdp_client->rail_active_window_id == rail_window->window_id)
            rdp_client->rail_active_window_id = 0;

        guac_common_list_remove(rdp_client->rail_windows, element);
        rail_window->deleted = 1;

        if (rail_window->paint_refs == 0)
            free_window = rail_window;

        rdp_client->gdi_modified = 1;
    }

    guac_common_list_unlock(rdp_client->rail_windows);

    guac_rdp_rail_free_window_data(free_window);

    return TRUE;

}

/**
 * Handler for RAIL monitored desktop orders. These orders include the
 * server-side z-order of RemoteApp windows as well setting active window.
 *
 * @param context
 *     A pointer to the rdpContext struct associated with the current RDP connection.
 *
 * @param orderInfo
 *     A pointer to the struct containing desktop order flags.
 *
 * @param monitoredDesktop
 *     A pointer to the monitored desktop state sent by the server.
 *
 * @return
 *     TRUE if the desktop order was handled successfully, otherwise FALSE.
 */
static BOOL guac_rdp_rail_monitored_desktop(rdpContext* context,
        RAIL_CONST WINDOW_ORDER_INFO* orderInfo,
        RAIL_CONST MONITORED_DESKTOP_ORDER* monitoredDesktop) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    if (!guac_rdp_rail_uses_gfx(rdp_client))
        return TRUE;

    if (!(orderInfo->fieldFlags & (WINDOW_ORDER_FIELD_DESKTOP_ZORDER
                    | WINDOW_ORDER_FIELD_DESKTOP_ACTIVE_WND)))
        return TRUE;

    int paint_background = 0;
    int apply_layer_updates = 0;

    guac_common_list_lock(rdp_client->rail_windows);

    if ((orderInfo->fieldFlags & WINDOW_ORDER_FIELD_DESKTOP_ZORDER)
            && monitoredDesktop->windowIds != NULL) {

        for (guac_common_list_element* current = rdp_client->rail_windows->head;
                current != NULL; current = current->next) {
            guac_rdp_rail_window* rail_window = current->data;
            if (rail_window != NULL)
                rail_window->in_monitored_desktop_zorder = 0;
        }

        /* The server-provided list is authoritative for visual stacking. */
        guac_client_log(client, GUAC_LOG_TRACE,
                "RAIL monitored desktop z-order: count=%u.",
                (unsigned int) monitoredDesktop->numWindowIds);

        for (UINT32 i = 0; i < monitoredDesktop->numWindowIds; i++) {

            guac_rdp_rail_window* rail_window =
                guac_rdp_rail_get_window(rdp_client,
                        monitoredDesktop->windowIds[i]);

            int z = (int) (monitoredDesktop->numWindowIds - i);

            if (rail_window != NULL && rail_window->has_rail_order) {
                rail_window->in_monitored_desktop_zorder = 1;
                guac_rdp_rail_stack_window(rail_window, z);
                guac_rdp_rail_raise_owned_windows(rdp_client,
                        rail_window->window_id, z + 1);
                apply_layer_updates = 1;
                rdp_client->gdi_modified = 1;
            }

        }

    }

    if ((orderInfo->fieldFlags & WINDOW_ORDER_FIELD_DESKTOP_ACTIVE_WND)
            && monitoredDesktop->activeWindowId != 0
            && monitoredDesktop->activeWindowId != 0xFFFFFFFFU) {

        guac_client_log(client, GUAC_LOG_TRACE,
                "RAIL monitored desktop active window: window=0x%08x.",
                (unsigned int) monitoredDesktop->activeWindowId);

        /*
         * Treat the server's active-window update as authoritative even when
         * the matching RAIL order has not arrived yet. Some owned popups are
         * activated before their window order, and re-sending activation on
         * click can dismiss them before the click reaches the intended item.
         * Rendering/stacking still waits for the RAIL order.
         */
        rdp_client->rail_active_window_id = monitoredDesktop->activeWindowId;

        guac_rdp_rail_window* active_window =
            guac_rdp_rail_get_window(rdp_client,
                    monitoredDesktop->activeWindowId);

        if (active_window != NULL) {

            if (active_window->has_rail_order
                    && active_window->has_surface && active_window->visible)
                paint_background = 1;

            if (active_window->has_rail_order
                    && !active_window->in_monitored_desktop_zorder) {
                int z = guac_rdp_rail_get_top_z(rdp_client);
                guac_rdp_rail_stack_window(active_window, z);
                guac_rdp_rail_raise_owned_windows(rdp_client,
                        active_window->window_id, z + 1);
                apply_layer_updates = 1;
                rdp_client->gdi_modified = 1;
            }
        }

    }

    guac_common_list_unlock(rdp_client->rail_windows);

    if (apply_layer_updates)
        guac_rdp_rail_apply_window_layers(rdp_client);

    if (paint_background)
        guac_rdp_rail_paint_background(rdp_client);

    return TRUE;

}

#ifdef HAVE_RDPGFX_SURFACE_AREA_UPDATE
UINT guac_rdp_rail_map_window_for_surface(RdpgfxClientContext* context,
        UINT16 surface_id, UINT64 window_id) {

    if (context == NULL)
        return CHANNEL_RC_OK;

    rdpGdi* gdi = (rdpGdi*) context->custom;
    if (gdi != NULL && gdi->context != NULL) {
        guac_client* client = ((rdp_freerdp_context*) gdi->context)->client;
        guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

        guac_rdp_rail_window* new_window =
            guac_mem_zalloc(sizeof(guac_rdp_rail_window));
        guac_common_list_element* new_element =
            guac_mem_alloc(sizeof(guac_common_list_element));

        guac_common_list_lock(rdp_client->rail_windows);

        guac_rdp_rail_window* old_window =
            guac_rdp_rail_get_window_for_surface(rdp_client, surface_id);

        if (old_window != NULL) {
            old_window->has_surface_id = 0;
        }

        guac_rdp_rail_window* rail_window =
            guac_rdp_rail_get_or_create_window(client, window_id,
                    new_window, new_element);

        if (rail_window == new_window) {
            new_window = NULL;
            new_element = NULL;
        }

        int repaint_surface = 0;
        int ensure_window_layer = 0;

        if (rail_window != NULL) {
            ensure_window_layer = rail_window->layer == NULL;

            /* Surface mappings can precede RAIL window orders. Track the
             * surface now, but keep the layer hidden until the order arrives. */
            rail_window->surface_id = surface_id;
            rail_window->has_surface_id = 1;
            repaint_surface = rail_window->visible
                    && rail_window->has_rail_order
                    && !rail_window->has_surface;
            guac_client_log(client, GUAC_LOG_TRACE,
                    "RAIL GFX surface mapped: surface=%u window=0x%08x "
                    "has-order=%i visible=%i.",
                    (unsigned int) surface_id, (unsigned int) window_id,
                    rail_window->has_rail_order, rail_window->visible);
        }
        else {
            guac_client_log(client, GUAC_LOG_TRACE,
                    "RAIL GFX surface map ignored: surface=%u "
                    "window=0x%08x could not be tracked.",
                    (unsigned int) surface_id, (unsigned int) window_id);
        }

        guac_common_list_unlock(rdp_client->rail_windows);

        if (ensure_window_layer)
            guac_rdp_rail_ensure_window_layer(client, window_id);

#ifdef HAVE_RDPGFX_SURFACE_AREA_UPDATE
        if (repaint_surface)
            guac_rdp_rail_repaint_surface(client, surface_id);
#endif

        guac_mem_free(new_window);
        guac_mem_free(new_element);
    }

    return CHANNEL_RC_OK;

}

UINT guac_rdp_rail_unmap_window_for_surface(RdpgfxClientContext* context,
        UINT64 window_id) {

    if (context == NULL)
        return CHANNEL_RC_OK;

    rdpGdi* gdi = (rdpGdi*) context->custom;
    if (gdi != NULL && gdi->context != NULL) {
        guac_client* client = ((rdp_freerdp_context*) gdi->context)->client;
        guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

        guac_common_list_lock(rdp_client->rail_windows);

        guac_rdp_rail_window* rail_window =
            guac_rdp_rail_get_window(rdp_client, window_id);

        if (rail_window != NULL)
            rail_window->has_surface_id = 0;

        guac_common_list_unlock(rdp_client->rail_windows);
    }

    return CHANNEL_RC_OK;

}

/**
 * Copies updated regions from the given RDPGFX surface to the Guacamole layer
 * associated with that surface's RAIL window. The rail_windows list lock must NOT
 * be held on entry; this function takes it internally.
 *
 * The invalidRegion of the surface is cleared only after a successful paint.
 * FreeRDP's GDI pipeline owns that region and may consume or clear it independently.
 * Deferred surfaces keep their invalid region so FreeRDP retries them after
 * the matching RAIL window order arrives.
 *
 * @param client
 *     The Guacamole client associated with the RDP session.
 *
 * @param surface
 *     The RDPGFX surface containing the updated window contents.
 *
 * @param rect_count
 *     The number of rectangles within the rects array.
 *
 * @param rects
 *     The updated rectangular regions of the surface to copy.
 *
 * @return
 *     CHANNEL_RC_OK if the update was painted, ignored, or deferred
 *     successfully. ERROR_INTERNAL_ERROR if copying or scaling the surface
 *     data failed.
 */
static UINT guac_rdp_rail_paint_surface(guac_client* client,
        gdiGfxSurface* surface, UINT32 rect_count, const RECTANGLE_16* rects) {

    if (surface == NULL)
        return CHANNEL_RC_OK;

    if (surface->windowId == 0) {
        guac_client_log(client, GUAC_LOG_DEBUG,
                "RAIL GFX surface ignored: surface=%u has no window id.",
                (unsigned int) surface->surfaceId);
        return CHANNEL_RC_OK;
    }

    if (rects == NULL || rect_count == 0)
        return CHANNEL_RC_OK;

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    int apply_layer_updates = 0;

    guac_common_list_lock(rdp_client->rail_windows);

    guac_rdp_rail_window* rail_window =
        guac_rdp_rail_get_window(rdp_client, surface->windowId);

    if (rail_window == NULL)
        rail_window =
            guac_rdp_rail_get_window_for_surface(rdp_client,
                    surface->surfaceId);

    if (rail_window == NULL) {
        guac_common_list_unlock(rdp_client->rail_windows);
        return CHANNEL_RC_OK;
    }

    rail_window->surface_id = surface->surfaceId;
    rail_window->has_surface_id = 1;

    /* FreeRDP (gdi/gfx.c, xf_gfx.c) initializes mappedWidth/Height and
     * outputTargetWidth/Height to identical values when a surface is created.
     * They only differ if a MapSurfaceToScaledWindow order explicitly rescales
     * the surface. Use an unscaled copy for the common 1:1 case.
     *
     * Size the layer from outputTargetWidth/Height (client surface), not
     * RAIL order_width/height (we exclude borders/title bar),
     * avoiding unnecessary freerdp_image_scale() calls. */
    int source_width = surface->mappedWidth;
    int source_height = surface->mappedHeight;
    int target_width = surface->outputTargetWidth;
    int target_height = surface->outputTargetHeight;

    if (target_width <= 0)
        target_width = source_width;
    if (target_height <= 0)
        target_height = source_height;

    /* Size the Guacamole layer to mappedWidth/Height, not
     * outputTargetWidth/Height. The latter may include server-side DPI scaling,
     * causing click coordinate drift because input is relative to the source
     * pixel size. Guacamole applies display scaling independently. */
    int layer_width = source_width;
    int layer_height = source_height;

    int preserve_alpha = surface->format == PIXEL_FORMAT_BGRA32;

    if (source_width <= 0 || source_height <= 0
            || target_width <= 0 || target_height <= 0
            || layer_width <= 0 || layer_height <= 0) {
        rail_window->has_surface = 0;
        guac_rdp_rail_update_window_opacity(rdp_client, rail_window);
        guac_common_list_unlock(rdp_client->rail_windows);
        return CHANNEL_RC_OK;
    }

    rail_window->surface_width = source_width;
    rail_window->surface_height = source_height;

    if (!rail_window->visible) {
        rail_window->has_deferred_surface = 1;
        guac_common_list_unlock(rdp_client->rail_windows);
        return CHANNEL_RC_OK;
    }

    int first_surface = !rail_window->has_surface;

    int scaled = (source_width != layer_width
            || source_height != layer_height);

    if (!rail_window->has_rail_order) {
        rail_window->has_deferred_surface = 1;
        guac_common_list_unlock(rdp_client->rail_windows);
        return CHANNEL_RC_OK;
    }

    if (rail_window->layer == NULL) {
        rail_window->has_deferred_surface = 1;
        guac_common_list_unlock(rdp_client->rail_windows);

        if (guac_rdp_rail_ensure_window_layer(client, surface->windowId))
            return guac_rdp_rail_repaint_surface(client, surface->surfaceId);

        return CHANNEL_RC_OK;
    }

    guac_display_layer* layer = rail_window->layer;
    apply_layer_updates = rail_window->layer_alpha != preserve_alpha;
    rail_window->layer_alpha = preserve_alpha;

    if (preserve_alpha) {
        layer = guac_rdp_rail_get_alpha_layer(client, rail_window);
        if (layer == NULL) {
            guac_client_log(client, GUAC_LOG_WARNING, "RAIL window 0x%08x "
                    "could not allocate alpha layer for surface format 0x%08x.",
                    (unsigned int) rail_window->window_id,
                    (unsigned int) surface->format);
            rail_window->layer_alpha = 0;
            layer = rail_window->layer;
            apply_layer_updates = 1;
        }
    }

    int old_layer_width = rail_window->layer_width;
    int old_layer_height = rail_window->layer_height;
    int needs_resize = old_layer_width != layer_width
        || old_layer_height != layer_height;

    rail_window->paint_refs++;

    guac_common_list_unlock(rdp_client->rail_windows);

    if (apply_layer_updates)
        guac_rdp_rail_apply_window_layers(rdp_client);

    if (needs_resize) {
        guac_display_layer_resize(rail_window->layer,
                layer_width, layer_height);

        if (rail_window->alpha_layer != NULL)
            guac_display_layer_resize(rail_window->alpha_layer,
                    layer_width, layer_height);
    }

    guac_display_layer_raw_context* layer_context =
        guac_display_layer_open_raw(layer);

    if (layer_context == NULL) {
        guac_rdp_rail_window* free_window = NULL;

        guac_common_list_lock(rdp_client->rail_windows);
        if (guac_rdp_rail_release_paint_ref(rail_window))
            free_window = rail_window;
        guac_common_list_unlock(rdp_client->rail_windows);

        guac_rdp_rail_free_window_data(free_window);
        return CHANNEL_RC_OK;
    }

    guac_rect source_bounds;
    guac_rect_init(&source_bounds, 0, 0, source_width, source_height);

    RECTANGLE_16 full_rect = {
        .left = 0,
        .top = 0,
        .right = source_width,
        .bottom = source_height
    };

    const RECTANGLE_16* paint_rects = rects;
    UINT32 paint_rect_count = rect_count;

    if (first_surface) {
        paint_rects = &full_rect;
        paint_rect_count = 1;
    }

    /* Preserve alpha only for RDPGFX surfaces whose pixel format explicitly
     * includes alpha. Other formats may leave the high byte undefined. */
    UINT32 dst_format = guac_rdp_get_native_pixel_format(preserve_alpha);
    int copied = 0;

    for (UINT32 i = 0; i < paint_rect_count; i++) {

        const RECTANGLE_16* rect = &paint_rects[i];
        guac_rect src_rect;
        guac_rect_init(&src_rect, rect->left, rect->top,
                rect->right - rect->left, rect->bottom - rect->top);

        guac_rect_constrain(&src_rect, &source_bounds);
        if (guac_rect_is_empty(&src_rect))
            continue;

        if (scaled && !first_surface) {
            src_rect.left -= 2;
            src_rect.top -= 2;
            src_rect.right += 2;
            src_rect.bottom += 2;
            guac_rect_constrain(&src_rect, &source_bounds);
        }

        guac_rect dst_rect;
        if (scaled) {
            dst_rect.left =
                ((UINT64) src_rect.left * layer_width) / source_width;
            dst_rect.top =
                ((UINT64) src_rect.top * layer_height) / source_height;
            dst_rect.right =
                (((UINT64) src_rect.right * layer_width)
                    + source_width - 1) / source_width;
            dst_rect.bottom =
                (((UINT64) src_rect.bottom * layer_height)
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
            success = freerdp_image_copy_no_overlap(
                    GUAC_DISPLAY_LAYER_RAW_BUFFER(layer_context, dst_rect),
                    dst_format,
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
                    dst_format,
                    layer_context->stride,
                    0, 0,
                    guac_rect_width(&dst_rect),
                    guac_rect_height(&dst_rect),
                    surface->data, surface->format, surface->scanline,
                    src_rect.left, src_rect.top,
                    guac_rect_width(&src_rect),
                    guac_rect_height(&src_rect));

        if (!success) {
            guac_rdp_rail_window* free_window = NULL;

            guac_client_log(client, GUAC_LOG_WARNING, "RAIL GFX paint failed");
            guac_display_layer_close_raw(layer, layer_context);

            guac_common_list_lock(rdp_client->rail_windows);
            if (guac_rdp_rail_release_paint_ref(rail_window))
                free_window = rail_window;
            guac_common_list_unlock(rdp_client->rail_windows);

            guac_rdp_rail_free_window_data(free_window);
            return ERROR_INTERNAL_ERROR;
        }

        if (!preserve_alpha)
            guac_rdp_rail_force_opaque(layer_context, &dst_rect);

        guac_rect_extend(&layer_context->dirty, &dst_rect);
        copied = 1;

    }

    guac_display_layer_close_raw(layer, layer_context);

    if (!copied) {
        guac_rdp_rail_window* free_window = NULL;

        guac_common_list_lock(rdp_client->rail_windows);
        if (!rail_window->deleted) {
            rail_window->layer_width = layer_width;
            rail_window->layer_height = layer_height;
        }

        if (guac_rdp_rail_release_paint_ref(rail_window))
            free_window = rail_window;
        guac_common_list_unlock(rdp_client->rail_windows);

        guac_rdp_rail_free_window_data(free_window);
        return CHANNEL_RC_OK;
    }

    region16_clear(&surface->invalidRegion);

    guac_common_list_lock(rdp_client->rail_windows);

    if (rail_window->deleted) {
        guac_rdp_rail_window* free_window = NULL;

        if (guac_rdp_rail_release_paint_ref(rail_window))
            free_window = rail_window;
        guac_common_list_unlock(rdp_client->rail_windows);

        guac_rdp_rail_free_window_data(free_window);
        return CHANNEL_RC_OK;
    }

    rail_window->layer_width = layer_width;
    rail_window->layer_height = layer_height;
    int update_opacity = !rail_window->has_surface;
    apply_layer_updates = apply_layer_updates || update_opacity;
    rail_window->has_surface = 1;
    rail_window->has_deferred_surface = 0;

    int paint_background = rail_window->has_rail_order
        && rail_window->visible;

    /* If a visible top-level window paints before authoritative z-order is
     * available, raise it once so newly launched RemoteApps become visible.
     * Later paints must not keep raising it above other windows. */
    if (!rail_window->painted_raise_done
            && rail_window->has_rail_order && rail_window->visible
            && !rail_window->in_monitored_desktop_zorder
            && rail_window->owner_window_id == 0) {
        int z = guac_rdp_rail_get_top_z(rdp_client);
        guac_rdp_rail_stack_window(rail_window, z);
        guac_rdp_rail_raise_owned_windows(rdp_client,
                rail_window->window_id, z + 1);
        rail_window->painted_raise_done = 1;
        apply_layer_updates = 1;
    }

    if (update_opacity)
        guac_rdp_rail_update_window_opacity(rdp_client, rail_window);
    rdp_client->gdi_modified = 1;

    guac_rdp_rail_window* free_window = NULL;
    if (guac_rdp_rail_release_paint_ref(rail_window))
        free_window = rail_window;

    guac_common_list_unlock(rdp_client->rail_windows);

    if (apply_layer_updates)
        guac_rdp_rail_apply_window_layers(rdp_client);

    if (paint_background)
        guac_rdp_rail_paint_background(rdp_client);

    guac_rdp_rail_free_window_data(free_window);

    return CHANNEL_RC_OK;

}

UINT guac_rdp_rail_update_surface_area(RdpgfxClientContext* context,
        UINT16 surface_id, UINT32 rect_count, const RECTANGLE_16* rects) {

    if (context == NULL)
        return CHANNEL_RC_OK;

    rdpGdi* gdi = (rdpGdi*) context->custom;
    if (gdi == NULL || gdi->context == NULL)
        return CHANNEL_RC_OK;

    if (gdi->suppressOutput)
        return CHANNEL_RC_OK;

    if (context->GetSurfaceData == NULL)
        return CHANNEL_RC_OK;

    gdiGfxSurface* surface =
        (gdiGfxSurface*) context->GetSurfaceData(context, surface_id);

    if (surface == NULL)
        return CHANNEL_RC_OK;

    /* Only window-mapped surfaces are routed through us; non-RAIL surfaces
     * are handled by FreeRDP's default GDI pipeline. We return early without
     * touching invalidRegion so the default path can consume it normally on
     * the next UpdateSurfaces tick. */
    if (surface->windowId == 0)
        return CHANNEL_RC_OK;

    guac_client* client = ((rdp_freerdp_context*) gdi->context)->client;
    return guac_rdp_rail_paint_surface(client, surface, rect_count, rects);

}

#ifdef HAVE_RDPGFX_WINDOW_SURFACE_UPDATE
UINT guac_rdp_rail_update_window_from_surface(RdpgfxClientContext* context,
        gdiGfxSurface* surface) {

    if (context == NULL)
        return CHANNEL_RC_OK;

    rdpGdi* gdi = (rdpGdi*) context->custom;
    if (gdi == NULL || gdi->context == NULL)
        return CHANNEL_RC_OK;

    if (gdi->suppressOutput)
        return CHANNEL_RC_OK;

    if (surface == NULL)
        return CHANNEL_RC_OK;

    if (surface->windowId == 0)
        return CHANNEL_RC_OK;

    /* Snapshot the rect list before painting. region16_rects() returns a
     * pointer into the region's internal storage, which is only valid until
     * the region is next modified. FreeRDP's GDI pipeline clears
     * invalidRegion after this callback returns. */
    UINT32 rect_count = 0;
    const RECTANGLE_16* rects =
        region16_rects(&surface->invalidRegion, &rect_count);

    guac_client* client = ((rdp_freerdp_context*) gdi->context)->client;
    return guac_rdp_rail_paint_surface(client, surface, rect_count, rects);

}
#endif
#endif

void guac_rdp_rail_free_windows(guac_rdp_client* rdp_client) {

    if (rdp_client->rail_windows != NULL) {
        guac_common_list_free(rdp_client->rail_windows,
                guac_rdp_rail_free_window_data);
        rdp_client->rail_windows = NULL;
    }

    if (rdp_client->rail_window_layer != NULL) {
        guac_display_free_layer(rdp_client->rail_window_layer);
        rdp_client->rail_window_layer = NULL;
    }

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
    rail->ServerLocalMoveSize = guac_rdp_rail_local_move_size;
    context->update->window->WindowCreate = guac_rdp_rail_window_update;
    context->update->window->WindowUpdate = guac_rdp_rail_window_update;
    context->update->window->WindowDelete = guac_rdp_rail_window_delete;
    context->update->window->MonitoredDesktop =
        guac_rdp_rail_monitored_desktop;

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
