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


#ifndef _GUAC_RDP_RDP_GLYPH_H
#define _GUAC_RDP_RDP_GLYPH_H

#include "config.h"

#include <cairo/cairo.h>
#include <freerdp/freerdp.h>

#ifdef ENABLE_WINPR
#include <winpr/wtypes.h>
#else
#include "compat/winpr-wtypes.h"
#endif

/**
 * Guacamole-specific rdpGlyph data.
 */
typedef struct guac_rdp_glyph {

    /**
     * FreeRDP glyph data - MUST GO FIRST.
     */
    rdpGlyph glyph;

    /**
     * Cairo surface layer containing cached image data.
     */
    cairo_surface_t* surface;

} guac_rdp_glyph;

/**
 * Caches the given glyph. Note that this caching currently only occurs server-
 * side, as it is more efficient to transmit the text as PNG.
 *
 * @param context The rdpContext associated with the current RDP session.
 * @param glyph The glyph to cache.
 */
void guac_rdp_glyph_new(rdpContext* context, rdpGlyph* glyph);

/**
 * Draws a previously-cached glyph at the given coordinates within the current
 * drawing surface.
 *
 * @param context The rdpContext associated with the current RDP session.
 * @param glyph The cached glyph to draw.
 * @param x The destination X coordinate of the upper-left corner of the glyph.
 * @param y The destination Y coordinate of the upper-left corner of the glyph.
 */
void guac_rdp_glyph_draw(rdpContext* context, rdpGlyph* glyph, int x, int y);

/**
 * Frees any Guacamole-specific data associated with the given glyph, such that
 * it can be safely freed by FreeRDP.
 *
 * @param context The rdpContext associated with the current RDP session.
 * @param glyph The cached glyph to free.
 */
void guac_rdp_glyph_free(rdpContext* context, rdpGlyph* glyph);

/**
 * Called just prior to rendering a series of glyphs. After this function is
 * called, the glyphs will be individually rendered by calls to
 * guac_rdp_glyph_draw().
 *
 * @param context The rdpContext associated with the current RDP session.
 *
 * @param x
 *     The X coordinate of the upper-left corner of the background rectangle of
 *     the drawing operation, or 0 if the background is transparent.
 *
 * @param y
 *     The Y coordinate of the upper-left corner of the background rectangle of
 *     the drawing operation, or 0 if the background is transparent.
 *
 * @param width
 *     The width of the background rectangle of the drawing operation, or 0 if
 *     the background is transparent.
 *
 * @param height 
 *     The height of the background rectangle of the drawing operation, or 0 if
 *     the background is transparent.
 *
 * @param fgcolor
 *     The foreground color of each glyph. This color will be in the colorspace
 *     of the RDP session, and may even be a palette index, and must be
 *     translated via guac_rdp_convert_color().
 *
 * @param bgcolor
 *     The background color of the drawing area. This color will be in the
 *     colorspace of the RDP session, and may even be a palette index, and must
 *     be translated via guac_rdp_convert_color(). If the background is
 *     transparent, this value is undefined.
 */
void guac_rdp_glyph_begindraw(rdpContext* context,
        int x, int y, int width, int height, UINT32 fgcolor, UINT32 bgcolor);

/**
 * Called immediately after rendering a series of glyphs. Unlike
 * guac_rdp_glyph_begindraw(), there is no way to detect through any invocation
 * of this function whether the background color is opaque or transparent. We
 * currently do NOT implement this function.
 *
 * @param context The rdpContext associated with the current RDP session.
 *
 * @param x
 *     The X coordinate of the upper-left corner of the background rectangle of
 *     the drawing operation.
 *
 * @param y
 *     The Y coordinate of the upper-left corner of the background rectangle of
 *     the drawing operation.
 *
 * @param width
 *     The width of the background rectangle of the drawing operation.
 *
 * @param height 
 *     The height of the background rectangle of the drawing operation.
 *
 * @param fgcolor
 *     The foreground color of each glyph. This color will be in the colorspace
 *     of the RDP session, and may even be a palette index, and must be
 *     translated via guac_rdp_convert_color().
 *
 * @param bgcolor
 *     The background color of the drawing area. This color will be in the
 *     colorspace of the RDP session, and may even be a palette index, and must
 *     be translated via guac_rdp_convert_color(). If the background is
 *     transparent, this value is undefined.
 */
void guac_rdp_glyph_enddraw(rdpContext* context,
        int x, int y, int width, int height, UINT32 fgcolor, UINT32 bgcolor);

#endif
