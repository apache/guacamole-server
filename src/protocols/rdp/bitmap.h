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

#ifndef GUAC_RDP_BITMAP_H
#define GUAC_RDP_BITMAP_H

#include "config.h"
#include "common/display.h"

#include <freerdp/freerdp.h>
#include <freerdp/graphics.h>
#include <guacamole/layer.h>
#include <winpr/wtypes.h>

/**
 * Guacamole-specific rdpBitmap data.
 */
typedef struct guac_rdp_bitmap {

    /**
     * FreeRDP bitmap data - MUST GO FIRST.
     */
    rdpBitmap bitmap;

    /**
     * Layer containing cached image data.
     */
    guac_common_display_layer* layer;

    /**
     * The number of times a bitmap has been used.
     */
    int used;

} guac_rdp_bitmap;

/**
 * Caches the given bitmap immediately, storing its data in a remote Guacamole
 * buffer. As RDP bitmaps are frequently created, used once, and immediately
 * destroyed, we defer actual remote-side caching of RDP bitmaps until they are
 * used at least once.
 *
 * @param context
 *     The rdpContext associated with the current RDP session.
 *
 * @param bitmap
 *     The bitmap to cache.
 */
void guac_rdp_cache_bitmap(rdpContext* context, rdpBitmap* bitmap);

/**
 * Initializes the given newly-created rdpBitmap.
 *
 * @param context
 *     The rdpContext associated with the current RDP session.
 *
 * @param bitmap
 *     The bitmap to initialize.
 *
 * @return
 *     TRUE if successful, FALSE otherwise.
 */
BOOL guac_rdp_bitmap_new(rdpContext* context, rdpBitmap* bitmap);

/**
 * Paints the given rdpBitmap on the primary display surface. Note that this
 * operation does NOT draw to the "current" surface set by calls to
 * guac_rdp_bitmap_setsurface().
 *
 * @param context
 *     The rdpContext associated with the current RDP session.
 *
 * @param bitmap
 *     The bitmap to paint. This structure will also contain the specifics of
 *     the paint operation to perform, including the destination X/Y
 *     coordinates.
 *
 * @return
 *     TRUE if successful, FALSE otherwise.
 */
BOOL guac_rdp_bitmap_paint(rdpContext* context, rdpBitmap* bitmap);

/**
 * Frees any Guacamole-specific data associated with the given rdpBitmap.
 *
 * @param context
 *     The rdpContext associated with the current RDP session.
 *
 * @param bitmap
 *     The bitmap whose Guacamole-specific data is to be freed.
 */
void guac_rdp_bitmap_free(rdpContext* context, rdpBitmap* bitmap);

/**
 * Sets the given rdpBitmap as the drawing surface for future operations or,
 * if the primary flag is set, resets the current drawing surface to the
 * primary drawing surface of the remote display.
 *
 * @param context
 *     The rdpContext associated with the current RDP session.
 *
 * @param bitmap
 *     The rdpBitmap to set as the current drawing surface. This parameter is
 *     only valid if the primary flag is FALSE.
 *
 * @param primary
 *     TRUE if the bitmap parameter should be ignored, and the current drawing
 *     surface should be reset to the primary drawing surface of the remote
 *     display, FALSE otherwise.
 *
 * @return
 *     TRUE if successful, FALSE otherwise.
 */
BOOL guac_rdp_bitmap_setsurface(rdpContext* context, rdpBitmap* bitmap,
        BOOL primary);

#endif
