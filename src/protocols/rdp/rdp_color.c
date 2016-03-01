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

#include "config.h"

#include "client.h"
#include "rdp.h"
#include "rdp_settings.h"

#include <freerdp/codec/color.h>
#include <freerdp/freerdp.h>

#ifdef ENABLE_WINPR
#include <winpr/wtypes.h>
#else
#include "compat/winpr-wtypes.h"
#endif

UINT32 guac_rdp_convert_color(rdpContext* context, UINT32 color) {

#ifdef HAVE_FREERDP_CONVERT_GDI_ORDER_COLOR
    UINT32* palette = ((rdp_freerdp_context*) context)->palette;

    /* Convert given color to ARGB32 */
    return freerdp_convert_gdi_order_color(color,
            guac_rdp_get_depth(context->instance), PIXEL_FORMAT_ARGB32,
            (BYTE*) palette);

#elif defined(HAVE_FREERDP_COLOR_CONVERT_DRAWING_ORDER_COLOR_TO_GDI_COLOR)
    CLRCONV* clrconv = ((rdp_freerdp_context*) context)->clrconv;

    /* Convert given color to ARGB32 */
    return freerdp_color_convert_drawing_order_color_to_gdi_color(color,
            guac_rdp_get_depth(context->instance), clrconv);

#else
    CLRCONV* clrconv = ((rdp_freerdp_context*) context)->clrconv;

    /* Convert given color to ARGB32 */
    return freerdp_color_convert_var(color,
            guac_rdp_get_depth(context->instance), 32,
            clrconv);
#endif

}

