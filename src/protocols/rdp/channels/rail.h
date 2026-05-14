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

#ifdef HAVE_RDPGFX_SURFACE_AREA_UPDATE
/**
 * Updates the Guacamole display layer for a RAIL window from modified areas of
 * the given RDPGFX surface. Non-RAIL (windowId == 0) surfaces are ignored and
 * left for FreeRDP's default GDI pipeline.
 *
 * Used only as a fallback when UpdateWindowFromSurface is unavailable in the
 * installed FreeRDP. The invalidRegion of the surface is NOT cleared.
 */
UINT guac_rdp_rail_update_surface_area(RdpgfxClientContext* context,
        UINT16 surface_id, UINT32 rect_count, const RECTANGLE_16* rects);

#ifdef HAVE_RDPGFX_WINDOW_SURFACE_UPDATE
/**
 * Updates the Guacamole display layer for a RAIL window from the given
 * window-mapped RDPGFX surface. FreeRDP's GDI pipeline calls this once per
 * surface per frame, with the surface's accumulated invalidRegion, and
 * subsequently clears that region itself.
 */
UINT guac_rdp_rail_update_window_from_surface(RdpgfxClientContext* context,
        gdiGfxSurface* surface);
#endif
#endif

/**
 * Frees all RAIL window state tracked for RemoteApp rendering. Safe to call
 * even when no RAIL windows are tracked.
 */
void guac_rdp_rail_free_windows(struct guac_rdp_client* rdp_client);

#endif

