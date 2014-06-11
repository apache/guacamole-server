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
#include "guac_surface.h"

#include <freerdp/freerdp.h>
#include <guacamole/layer.h>

#ifdef ENABLE_WINPR
#include <winpr/wtypes.h>
#else
#include "compat/winpr-wtypes.h"
#endif

typedef struct guac_rdp_bitmap {

    /**
     * FreeRDP bitmap data - MUST GO FIRST.
     */
    rdpBitmap bitmap;

    /**
     * The allocated buffer which backs this bitmap.
     */
    guac_layer* buffer;

    /**
     * Surface containing cached image data.
     */
    guac_common_surface* surface;

    /**
     * The number of times a bitmap has been used.
     */
    int used;

} guac_rdp_bitmap;

void guac_rdp_cache_bitmap(rdpContext* context, rdpBitmap* bitmap);
void guac_rdp_bitmap_new(rdpContext* context, rdpBitmap* bitmap);
void guac_rdp_bitmap_paint(rdpContext* context, rdpBitmap* bitmap);
void guac_rdp_bitmap_free(rdpContext* context, rdpBitmap* bitmap);
void guac_rdp_bitmap_setsurface(rdpContext* context, rdpBitmap* bitmap, BOOL primary);

#ifdef LEGACY_RDPBITMAP
void guac_rdp_bitmap_decompress(rdpContext* context, rdpBitmap* bitmap, UINT8* data,
        int width, int height, int bpp, int length, BOOL compressed);
#else
void guac_rdp_bitmap_decompress(rdpContext* context, rdpBitmap* bitmap, UINT8* data,
        int width, int height, int bpp, int length, BOOL compressed, int codec_id);
#endif

#endif
