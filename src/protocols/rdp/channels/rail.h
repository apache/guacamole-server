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

#ifndef GUAC_RDP_CHANNELS_RAIL_H
#define GUAC_RDP_CHANNELS_RAIL_H

#include <freerdp/freerdp.h>
#include <freerdp/window.h>

struct guac_rdp_client;

#ifdef HAVE_RDPGFX_SURFACE_AREA_UPDATE
#include <freerdp/client/rdpgfx.h>
#include <freerdp/gdi/gfx.h>
#endif

#ifdef FREERDP_RAIL_CALLBACKS_REQUIRE_CONST
/**
 * FreeRDP 2.0.0-rc4 and newer requires the final arguments for RAIL
 * callbacks to be const.
 */
#define RAIL_CONST const
#else
/**
 * FreeRDP 2.0.0-rc3 and older requires the final arguments for RAIL
 * callbacks to NOT be const.
 */
#define RAIL_CONST
#endif

/**
 * The RAIL window state that indicates a hidden window.
 */
#define GUAC_RDP_RAIL_WINDOW_STATE_HIDDEN 0x00

/**
 * The RAIL window state that indicates a visible but minimized window.
 */
#define GUAC_RDP_RAIL_WINDOW_STATE_MINIMIZED 0x02

/**
 * Initializes RemoteApp support for RDP and handling of the RAIL channel. If
 * failures occur, messages noting the specifics of those failures will be
 * logged, and RemoteApp support will not be functional.
 *
 * This MUST be called within the PreConnect callback of the freerdp instance
 * for RAIL support to be loaded.
 *
 * @param context
 *     The rdpContext associated with the FreeRDP side of the RDP connection.
 */
void guac_rdp_rail_load_plugin(rdpContext* context);

/**
 * Raises and activates the topmost tracked RAIL window containing the given
 * display coordinates, if any.
 *
 * This is used as a local fallback for RemoteApp+GFX window ordering. Windows
 * may accept a click on a RemoteApp window without sending a monitored desktop
 * z-order or DESKTOP_ACTIVE_WND update. In that case, FreeRDP receives the
 * input normally, but Guacamole's separate per-window layers would remain in
 * the old visual order unless the clicked layer is raised locally. Sending
 * ClientActivate mirrors native FreeRDP clients and makes sure the RDP server
 * targets the clicked RemoteApp window before handling the mouse click.
 *
 * @param rdp_client
 *     The RDP client whose tracked RAIL windows should be searched.
 *
 * @param x
 *     The X coordinate of the mouse event.
 *
 * @param y
 *     The Y coordinate of the mouse event.
 */
void guac_rdp_rail_raise_window_at(struct guac_rdp_client* rdp_client,
        int x, int y);

/**
 * Tracks which RAIL window owns the current mouse drag.
 *
 * FreeRDP sends RDP mouse input in desktop coordinates even for RemoteApp. This
 * function records the clicked window so later drag/release events stay associated with it.
 *
 * @param rdp_client
 *     The RDP client whose tracked RAIL windows should be searched.
 *
 * @param mask
 *     The current Guacamole mouse button mask.
 *
 * @param x
 *     The X coordinate of the mouse event.
 *
 * @param y
 *     The Y coordinate of the mouse event.
 */
void guac_rdp_rail_track_mouse_event(struct guac_rdp_client* rdp_client,
        int mask, int x, int y);

#ifdef HAVE_RDPGFX_SURFACE_AREA_UPDATE
/**
 * Handles notification that the given RDPGFX surface has been mapped to the
 * given RAIL window.
 *
 * @param context
 *     The RDPGFX client context associated with the surface mapping.
 *
 * @param surface_id
 *     The server-side ID of the mapped RDPGFX surface.
 *
 * @param window_id
 *     The server-side ID of the RAIL window to which the surface was mapped.
 *
 * @return
 *     CHANNEL_RC_OK. Surface mapping failures are treated as non-fatal
 *     tracking failures. Window contents may not render (event gets logged), but the
 *     RDPGFX channel remains usable.
 */
UINT guac_rdp_rail_map_window_for_surface(RdpgfxClientContext* context,
        UINT16 surface_id, UINT64 window_id);

/**
 * Handles notification that an RDPGFX surface is no longer mapped to the given
 * RAIL window.
 *
 * @param context
 *     The RDPGFX client context associated with the surface unmapping.
 *
 * @param window_id
 *     The server-side ID of the RAIL window that was unmapped.
 *
 * @return
 *     CHANNEL_RC_OK is always returned. Surface unmapping failures are ignored
 *     as the window might not be tracked.
 */
UINT guac_rdp_rail_unmap_window_for_surface(RdpgfxClientContext* context,
        UINT64 window_id);

/**
 * Updates the Guacamole display layer for a RAIL window from modified areas of
 * the given RDPGFX surface. Non-RAIL (windowId == 0) surfaces are ignored and
 * left for FreeRDP's default GDI pipeline.
 *
 * Used only as a fallback when UpdateWindowFromSurface is unavailable in the
 * installed FreeRDP. Successful paints clear the invalidRegion of the surface,
 * matching the shared paint path used by window-mapped RDPGFX updates.
 *
 * @param context
 *     The RDPGFX client context associated with the surface update.
 *
 * @param surface_id
 *     The server-side ID of the updated RDPGFX surface.
 *
 * @param rect_count
 *     The number of rectangles within the rects array.
 *
 * @param rects
 *     The updated rectangular regions of the surface.
 *
 * @return
 *     CHANNEL_RC_OK if the surface was ignored or painted successfully.
 *     ERROR_INTERNAL_ERROR if painting the surface failed.
 */
UINT guac_rdp_rail_update_surface_area(RdpgfxClientContext* context,
        UINT16 surface_id, UINT32 rect_count, const RECTANGLE_16* rects);

#ifdef HAVE_RDPGFX_WINDOW_SURFACE_UPDATE
/**
 * Updates the Guacamole display layer for a RAIL window from the given
 * window-mapped RDPGFX surface. FreeRDP's GDI pipeline calls this once per
 * surface per frame, with the surface's accumulated invalidRegion, and
 * subsequently clears that region itself.
 *
 * @param context
 *     The RDPGFX client context associated with the surface update.
 *
 * @param surface
 *     The RDPGFX surface containing the updated window contents.
 *
 * @return
 *     CHANNEL_RC_OK if the update was ignored or painted successfully.
 *     ERROR_INTERNAL_ERROR if painting the surface failed.
 */
UINT guac_rdp_rail_update_window_from_surface(RdpgfxClientContext* context,
        gdiGfxSurface* surface);
#endif
#endif

/**
 * Frees all RAIL window state tracked for RemoteApp rendering. Safe to call
 * even when no RAIL windows are tracked.
 *
 * @param rdp_client
 *     The RDP client whose tracked RAIL windows should be freed.
 */
void guac_rdp_rail_free_windows(struct guac_rdp_client* rdp_client);

#endif
