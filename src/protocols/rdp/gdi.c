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

#include "bitmap.h"
#include "color.h"
#include "common/display.h"
#include "common/surface.h"
#include "rdp.h"
#include "settings.h"

#include <cairo/cairo.h>
#include <freerdp/freerdp.h>
#include <freerdp/graphics.h>
#include <freerdp/primary.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <winpr/wtypes.h>

#include <stddef.h>
#include <stddef.h>

guac_transfer_function guac_rdp_rop3_transfer_function(guac_client* client,
        int rop3) {

    /* Translate supported ROP3 opcodes into composite modes */
    switch (rop3) {

        /* "DSon" !(src | dest) */
        case 0x11: return GUAC_TRANSFER_BINARY_NOR;

        /* "DSna" !src & dest */
        case 0x22: return GUAC_TRANSFER_BINARY_NSRC_AND;

        /* "Sn" !src */
        case 0x33: return GUAC_TRANSFER_BINARY_NSRC;

        /* "SDna" (src & !dest) */
        case 0x44: return GUAC_TRANSFER_BINARY_NDEST_AND;

        /* "Dn" !dest */
        case 0x55: return GUAC_TRANSFER_BINARY_NDEST;

        /* "SRCINVERT" (src ^ dest) */
        case 0x66: return GUAC_TRANSFER_BINARY_XOR;

        /* "DSan" !(src & dest) */
        case 0x77: return GUAC_TRANSFER_BINARY_NAND;

        /* "SRCAND" (src & dest) */
        case 0x88: return GUAC_TRANSFER_BINARY_AND;

        /* "DSxn" !(src ^ dest) */
        case 0x99: return GUAC_TRANSFER_BINARY_XNOR;

        /* "MERGEPAINT" (!src | dest)*/
        case 0xBB: return GUAC_TRANSFER_BINARY_NSRC_OR;

        /* "SDno" (src | !dest) */
        case 0xDD: return GUAC_TRANSFER_BINARY_NDEST_OR;

        /* "SRCPAINT" (src | dest) */
        case 0xEE: return GUAC_TRANSFER_BINARY_OR;

        /* 0x00 = "BLACKNESS" (0) */
        /* 0xAA = "NOP" (dest) */
        /* 0xCC = "SRCCOPY" (src) */
        /* 0xFF = "WHITENESS" (1) */

    }

    /* Log warning if ROP3 opcode not supported */
    guac_client_log(client, GUAC_LOG_INFO, "guac_rdp_rop3_transfer_function: "
            "UNSUPPORTED opcode = 0x%02X", rop3);

    /* Default to BINARY_SRC */
    return GUAC_TRANSFER_BINARY_SRC;

}

BOOL guac_rdp_gdi_dstblt(rdpContext* context, const DSTBLT_ORDER* dstblt) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_common_surface* current_surface = ((guac_rdp_client*) client->data)->current_surface;

    int x = dstblt->nLeftRect;
    int y = dstblt->nTopRect;
    int w = dstblt->nWidth;
    int h = dstblt->nHeight;

    switch (dstblt->bRop) {

        /* Blackness */
        case 0:

            /* Send black rectangle */
            guac_common_surface_set(current_surface, x, y, w, h,
                    0x00, 0x00, 0x00, 0xFF);
            break;

        /* DSTINVERT */
        case 0x55:
            guac_common_surface_transfer(current_surface, x, y, w, h,
                                         GUAC_TRANSFER_BINARY_NDEST, current_surface, x, y);
            break;

        /* NOP */
        case 0xAA:
            break;

        /* Whiteness */
        case 0xFF:
            guac_common_surface_set(current_surface, x, y, w, h,
                    0xFF, 0xFF, 0xFF, 0xFF);
            break;

        /* Unsupported ROP3 */
        default:
            guac_client_log(client, GUAC_LOG_INFO,
                    "guac_rdp_gdi_dstblt(rop3=0x%x)", dstblt->bRop);

    }

    return TRUE;

}

BOOL guac_rdp_gdi_patblt(rdpContext* context, PATBLT_ORDER* patblt) {

    /*
     * Note that this is not a full implementation of PATBLT. This is a
     * fallback implementation which only renders a solid block of background
     * color using the specified ROP3 operation, ignoring whatever brush
     * was actually specified.
     *
     * As libguac-client-rdp explicitly tells the server not to send PATBLT,
     * well-behaved RDP servers will not use this operation at all, while
     * others will at least have a fallback.
     */

    /* Get client and current layer */
    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_common_surface* current_surface =
        ((guac_rdp_client*) client->data)->current_surface;

    int x = patblt->nLeftRect;
    int y = patblt->nTopRect;
    int w = patblt->nWidth;
    int h = patblt->nHeight;

    /*
     * Warn that rendering is a fallback, as the server should not be sending
     * this order.
     */
    guac_client_log(client, GUAC_LOG_INFO, "Using fallback PATBLT (server is ignoring "
            "negotiated client capabilities)");

    /* Render rectangle based on ROP */
    switch (patblt->bRop) {

        /* If blackness, send black rectangle */
        case 0x00:
            guac_common_surface_set(current_surface, x, y, w, h,
                    0x00, 0x00, 0x00, 0xFF);
            break;

        /* If NOP, do nothing */
        case 0xAA:
            break;

        /* If operation is just a copy, send foreground only */
        case 0xCC:
        case 0xF0:
            guac_common_surface_set(current_surface, x, y, w, h,
                    (patblt->foreColor >> 16) & 0xFF,
                    (patblt->foreColor >> 8 ) & 0xFF,
                    (patblt->foreColor      ) & 0xFF,
                    0xFF);
            break;

        /* If whiteness, send white rectangle */
        case 0xFF:
            guac_common_surface_set(current_surface, x, y, w, h,
                    0xFF, 0xFF, 0xFF, 0xFF);
            break;

        /* Otherwise, invert entire rect */
        default:
            guac_common_surface_transfer(current_surface, x, y, w, h,
                                         GUAC_TRANSFER_BINARY_NDEST, current_surface, x, y);

    }

    return TRUE;

}

BOOL guac_rdp_gdi_scrblt(rdpContext* context, const SCRBLT_ORDER* scrblt) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_common_surface* current_surface = ((guac_rdp_client*) client->data)->current_surface;
    
    int x = scrblt->nLeftRect;
    int y = scrblt->nTopRect;
    int w = scrblt->nWidth;
    int h = scrblt->nHeight;

    int x_src = scrblt->nXSrc;
    int y_src = scrblt->nYSrc;

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Copy screen rect to current surface */
    guac_common_surface_copy(rdp_client->display->default_surface,
            x_src, y_src, w, h, current_surface, x, y);

    return TRUE;

}

BOOL guac_rdp_gdi_memblt(rdpContext* context, MEMBLT_ORDER* memblt) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_common_surface* current_surface = ((guac_rdp_client*) client->data)->current_surface;
    guac_rdp_bitmap* bitmap = (guac_rdp_bitmap*) memblt->bitmap;

    int x = memblt->nLeftRect;
    int y = memblt->nTopRect;
    int w = memblt->nWidth;
    int h = memblt->nHeight;

    int x_src = memblt->nXSrc;
    int y_src = memblt->nYSrc;

    /* Make sure that the recieved bitmap is not NULL before processing */
    if (bitmap == NULL) {
        guac_client_log(client, GUAC_LOG_INFO, "NULL bitmap found in memblt instruction.");
        return TRUE;
    }

    switch (memblt->bRop) {

        /* If blackness, send black rectangle */
        case 0x00:
            guac_common_surface_set(current_surface, x, y, w, h,
                    0x00, 0x00, 0x00, 0xFF);
            break;

        /* If NOP, do nothing */
        case 0xAA:
            break;

        /* If operation is just SRC, simply copy */
        case 0xCC: 

            /* If not cached, cache if necessary */
            if (bitmap->layer == NULL && bitmap->used >= 1)
                guac_rdp_cache_bitmap(context, memblt->bitmap);

            /* If not cached, send as PNG */
            if (bitmap->layer == NULL) {
                if (memblt->bitmap->data != NULL) {

                    /* Create surface from image data */
                    cairo_surface_t* surface = cairo_image_surface_create_for_data(
                        memblt->bitmap->data + 4*(x_src + y_src*memblt->bitmap->width),
                        CAIRO_FORMAT_RGB24, w, h, 4*memblt->bitmap->width);

                    /* Send surface to buffer */
                    guac_common_surface_draw(current_surface, x, y, surface);

                    /* Free surface */
                    cairo_surface_destroy(surface);

                }
            }

            /* Otherwise, copy */
            else
                guac_common_surface_copy(bitmap->layer->surface,
                        x_src, y_src, w, h, current_surface, x, y);

            /* Increment usage counter */
            ((guac_rdp_bitmap*) bitmap)->used++;

            break;

        /* If whiteness, send white rectangle */
        case 0xFF:
            guac_common_surface_set(current_surface, x, y, w, h,
                    0xFF, 0xFF, 0xFF, 0xFF);
            break;

        /* Otherwise, use transfer */
        default:

            /* If not available as a surface, make available. */
            if (bitmap->layer == NULL)
                guac_rdp_cache_bitmap(context, memblt->bitmap);

            guac_common_surface_transfer(bitmap->layer->surface,
                    x_src, y_src, w, h,
                    guac_rdp_rop3_transfer_function(client, memblt->bRop),
                    current_surface, x, y);

            /* Increment usage counter */
            ((guac_rdp_bitmap*) bitmap)->used++;

    }

    return TRUE;

}

BOOL guac_rdp_gdi_opaquerect(rdpContext* context, const OPAQUE_RECT_ORDER* opaque_rect) {

    /* Get client data */
    guac_client* client = ((rdp_freerdp_context*) context)->client;

    UINT32 color = guac_rdp_convert_color(context, opaque_rect->color);

    guac_common_surface* current_surface = ((guac_rdp_client*) client->data)->current_surface;

    int x = opaque_rect->nLeftRect;
    int y = opaque_rect->nTopRect;
    int w = opaque_rect->nWidth;
    int h = opaque_rect->nHeight;

    guac_common_surface_set(current_surface, x, y, w, h,
            (color >> 16) & 0xFF,
            (color >> 8 ) & 0xFF,
            (color      ) & 0xFF,
            0xFF);

    return TRUE;

}

BOOL guac_rdp_gdi_set_bounds(rdpContext* context, const rdpBounds* bounds) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* If no bounds given, clear bounding rect */
    if (bounds == NULL)
        guac_common_surface_reset_clip(rdp_client->display->default_surface);

    /* Otherwise, set bounding rectangle */
    else
        guac_common_surface_clip(rdp_client->display->default_surface,
                bounds->left, bounds->top,
                bounds->right - bounds->left + 1,
                bounds->bottom - bounds->top + 1);

    return TRUE;

}

BOOL guac_rdp_gdi_end_paint(rdpContext* context) {
    /* IGNORE */
    return TRUE;
}

BOOL guac_rdp_gdi_desktop_resize(rdpContext* context) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    guac_common_surface_resize(rdp_client->display->default_surface,
            guac_rdp_get_width(context->instance),
            guac_rdp_get_height(context->instance));

    guac_common_surface_reset_clip(rdp_client->display->default_surface);

    guac_client_log(client, GUAC_LOG_DEBUG, "Server resized display to %ix%i",
            guac_rdp_get_width(context->instance),
            guac_rdp_get_height(context->instance));

    return TRUE;

}


