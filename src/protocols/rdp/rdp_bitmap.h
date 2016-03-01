/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#ifndef _GUAC_RDP_RDP_BITMAP_H
#define _GUAC_RDP_RDP_BITMAP_H

#include "config.h"
#include "guac_display.h"

#include <freerdp/freerdp.h>
#include <guacamole/layer.h>

#ifdef ENABLE_WINPR
#include <winpr/wtypes.h>
#else
#include "compat/winpr-wtypes.h"
#endif

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
 * @param context The rdpContext associated with the current RDP session.
 * @param bitmap The bitmap to cache.
 */
void guac_rdp_cache_bitmap(rdpContext* context, rdpBitmap* bitmap);

/**
 * Initializes the given newly-created rdpBitmap.
 *
 * @param context The rdpContext associated with the current RDP session.
 * @param bitmap The bitmap to initialize.
 */
void guac_rdp_bitmap_new(rdpContext* context, rdpBitmap* bitmap);

/**
 * Paints the given rdpBitmap on the primary display surface. Note that this
 * operation does NOT draw to the "current" surface set by calls to
 * guac_rdp_bitmap_setsurface().
 *
 * @param context The rdpContext associated with the current RDP session.
 *
 * @param bitmap
 *     The bitmap to paint. This structure will also contain the specifics of
 *     the paint operation to perform, including the destination X/Y
 *     coordinates.
 */
void guac_rdp_bitmap_paint(rdpContext* context, rdpBitmap* bitmap);

/**
 * Frees any Guacamole-specific data associated with the given rdpBitmap.
 *
 * @param context The rdpContext associated with the current RDP session.
 * @param bitmap The bitmap whose Guacamole-specific data is to be freed.
 */
void guac_rdp_bitmap_free(rdpContext* context, rdpBitmap* bitmap);

/**
 * Sets the given rdpBitmap as the drawing surface for future operations or,
 * if the primary flag is set, resets the current drawing surface to the
 * primary drawing surface of the remote display.
 *
 * @param context The rdpContext associated with the current RDP session.
 *
 * @param bitmap
 *     The rdpBitmap to set as the current drawing surface. This parameter is
 *     only valid if the primary flag is FALSE.
 *
 * @param primary
 *     TRUE if the bitmap parameter should be ignored, and the current drawing
 *     surface should be reset to the primary drawing surface of the remote
 *     display, FALSE otherwise.
 */
void guac_rdp_bitmap_setsurface(rdpContext* context, rdpBitmap* bitmap,
        BOOL primary);

#ifdef LEGACY_RDPBITMAP
/**
 * Decompresses or copies the given image data, storing the result within the
 * given bitmap, depending on the compressed flag. Note that even if the
 * received data is not compressed, it is the duty of this function to also
 * flip received data, if the row order is backwards.
 *
 * @param context The rdpContext associated with the current RDP session.
 *
 * @param bitmap
 *     The bitmap in which the decompressed/copied data should be stored.
 *
 * @param data Possibly-compressed image data.
 * @param width The width of the image data, in pixels.
 * @param height The height of the image data, in pixels.
 * @param bpp The number of bits per pixel in the image data.
 * @param length The length of the image data, in bytes.
 * @param compressed TRUE if the image data is compressed, FALSE otherwise.
 */
void guac_rdp_bitmap_decompress(rdpContext* context, rdpBitmap* bitmap,
        UINT8* data, int width, int height, int bpp, int length,
        BOOL compressed);
#else
/**
 * Decompresses or copies the given image data, storing the result within the
 * given bitmap, depending on the compressed flag. Note that even if the
 * received data is not compressed, it is the duty of this function to also
 * flip received data, if the row order is backwards.
 *
 * @param context The rdpContext associated with the current RDP session.
 *
 * @param bitmap
 *     The bitmap in which the decompressed/copied data should be stored.
 *
 * @param data Possibly-compressed image data.
 * @param width The width of the image data, in pixels.
 * @param height The height of the image data, in pixels.
 * @param bpp The number of bits per pixel in the image data.
 * @param length The length of the image data, in bytes.
 * @param compressed TRUE if the image data is compressed, FALSE otherwise.
 *
 * @param codec_id
 *     The ID of the codec used to compress the image data. This parameter is
 *     currently ignored.
 */
void guac_rdp_bitmap_decompress(rdpContext* context, rdpBitmap* bitmap,
        UINT8* data, int width, int height, int bpp, int length,
        BOOL compressed, int codec_id);
#endif

#endif
