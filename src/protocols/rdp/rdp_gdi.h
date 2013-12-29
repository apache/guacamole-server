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


#ifndef _GUAC_RDP_RDP_GDI_H
#define _GUAC_RDP_RDP_GDI_H

#include <guacamole/protocol.h>
#include <freerdp/freerdp.h>

guac_composite_mode guac_rdp_rop3_transfer_function(guac_client* client,
        int rop3);

void guac_rdp_gdi_dstblt(rdpContext* context, DSTBLT_ORDER* dstblt);
void guac_rdp_gdi_patblt(rdpContext* context, PATBLT_ORDER* patblt);
void guac_rdp_gdi_scrblt(rdpContext* context, SCRBLT_ORDER* scrblt);
void guac_rdp_gdi_memblt(rdpContext* context, MEMBLT_ORDER* memblt);
void guac_rdp_gdi_opaquerect(rdpContext* context, OPAQUE_RECT_ORDER* opaque_rect);
void guac_rdp_gdi_palette_update(rdpContext* context, PALETTE_UPDATE* palette);
void guac_rdp_gdi_set_bounds(rdpContext* context, rdpBounds* bounds);
void guac_rdp_gdi_end_paint(rdpContext* context);

#endif
