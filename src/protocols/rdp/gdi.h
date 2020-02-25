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

#ifndef GUAC_RDP_GDI_H
#define GUAC_RDP_GDI_H

#include "config.h"

#include <freerdp/freerdp.h>
#include <guacamole/protocol.h>

/**
 * Translates a standard RDP ROP3 value into a guac_composite_mode. Valid
 * ROP3 operations indexes are listed in the RDP protocol specifications:
 *
 * http://msdn.microsoft.com/en-us/library/cc241583.aspx
 *
 * @param client
 *     The guac_client associated with the current RDP session.
 *
 * @param rop3
 *     The ROP3 operation index to translate.
 *
 * @return
 *     The guac_composite_mode that equates to, or most closely approximates,
 *     the given ROP3 operation.
 */
guac_composite_mode guac_rdp_rop3_transfer_function(guac_client* client,
        int rop3);

/**
 * Handler for the DstBlt Primary Drawing Order. A DstBlt Primary Drawing Order
 * paints a rectangle of image data using a raster operation which considers
 * the destination only. See:
 *
 * https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-rdpegdi/87ea30df-59d6-438e-a735-83f0225fbf91
 *
 * @param context
 *     The rdpContext associated with the current RDP session.
 *
 * @param dstblt
 *     The DSTBLT update to handle.
 *
 * @return
 *     TRUE if successful, FALSE otherwise.
 */
BOOL guac_rdp_gdi_dstblt(rdpContext* context, const DSTBLT_ORDER* dstblt);

/**
 * Handler for the PatBlt Primary Drawing Order. A PatBlt Primary Drawing Order
 * paints a rectangle of image data, a brush pattern, and a three-way raster
 * operation which considers the source data, the destination, AND the brush
 * pattern. See:
 *
 * https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-rdpegdi/bd4bf5e7-b988-45f9-8201-3b22cc9aeeb8
 *
 * @param context
 *     The rdpContext associated with the current RDP session.
 *
 * @param patblt
 *     The PATBLT update to handle.
 *
 * @return
 *     TRUE if successful, FALSE otherwise.
 */
BOOL guac_rdp_gdi_patblt(rdpContext* context, PATBLT_ORDER* patblt);

/**
 * Handler for the ScrBlt Primary Drawing Order. A ScrBlt Primary Drawing Order
 * paints a rectangle of image data using a raster operation which considers
 * the source and destination. See:
 *
 * https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-rdpegdi/a4e322b0-cd64-4dfc-8e1a-f24dc0edc99d
 *
 * @param context
 *     The rdpContext associated with the current RDP session.
 *
 * @param scrblt
 *     The SCRBLT update to handle.
 *
 * @return
 *     TRUE if successful, FALSE otherwise.
 */
BOOL guac_rdp_gdi_scrblt(rdpContext* context, const SCRBLT_ORDER* scrblt);

/**
 * Handler for the MemBlt Primary Drawing Order. A MemBlt Primary Drawing Order
 * paints a rectangle of cached image data from a cached surface to the screen
 * using a raster operation which considers the source and destination. See:
 *
 * https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-rdpegdi/84c2ec2f-f776-405b-9b48-6894a28b1b14
 *
 * @param context
 *     The rdpContext associated with the current RDP session.
 *
 * @param memblt
 *     The MEMBLT update to handle.
 *
 * @return
 *     TRUE if successful, FALSE otherwise.
 */
BOOL guac_rdp_gdi_memblt(rdpContext* context, MEMBLT_ORDER* memblt);

/**
 * Handler for the OpaqueRect Primary Drawing Order. An OpaqueRect Primary
 * Drawing Order draws an opaque rectangle of a single solid color. Note that
 * support for OpaqueRect cannot be claimed without also supporting PatBlt, as
 * both use the same negotiation order number. See:
 *
 * https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-rdpegdi/1eead7aa-ac63-411a-9f8c-b1b227526877
 *
 * @param context
 *     The rdpContext associated with the current RDP session.
 *
 * @param opaque_rect
 *     The OPAQUE RECT update to handle.
 *
 * @return
 *     TRUE if successful, FALSE otherwise.
 */
BOOL guac_rdp_gdi_opaquerect(rdpContext* context,
        const OPAQUE_RECT_ORDER* opaque_rect);

/**
 * Handler called prior to calling the handlers for specific updates when
 * those updates are clipped by a bounding rectangle. This is not a true RDP
 * update, but is called by FreeRDP before and after any update involving
 * clipping.
 *
 * @param context
 *     The rdpContext associated with the current RDP session.
 *
 * @param bounds
 *     The clipping rectangle to set, or NULL to remove any applied clipping
 *     rectangle.
 *
 * @return
 *     TRUE if successful, FALSE otherwise.
 */
BOOL guac_rdp_gdi_set_bounds(rdpContext* context, const rdpBounds* bounds);

/**
 * Handler called when a paint operation is complete. We don't actually
 * use this, but FreeRDP requires it. Calling this function has no effect.
 *
 * @param context
 *     The rdpContext associated with the current RDP session.
 *
 * @return
 *     TRUE if successful, FALSE otherwise.
 */
BOOL guac_rdp_gdi_end_paint(rdpContext* context);

/**
 * Handler called when the desktop dimensions change, either from a
 * true desktop resize event received by the RDP client, or due to
 * a revised size given by the server during initial connection
 * negotiation.
 *
 * The new screen size will be made available within the settings associated
 * with the given context.
 *
 * @param context
 *     The rdpContext associated with the current RDP session.
 *
 * @return
 *     TRUE if successful, FALSE otherwise.
 */
BOOL guac_rdp_gdi_desktop_resize(rdpContext* context);

#endif
