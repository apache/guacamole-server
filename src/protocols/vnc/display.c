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

#include "config.h"

#include "client.h"
#include "common/iconv.h"
#include "vnc.h"

#include <cairo/cairo.h>
#include <guacamole/client.h>
#include <guacamole/layer.h>
#include <guacamole/mem.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <rfb/rfbclient.h>
#include <rfb/rfbproto.h>

/* Define cairo_format_stride_for_width() if missing */
#ifndef HAVE_CAIRO_FORMAT_STRIDE_FOR_WIDTH
#define cairo_format_stride_for_width(format, width) (width*4)
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <syslog.h>

void guac_vnc_update(rfbClient* client, int x, int y, int w, int h) {

    guac_client* gc = rfbClientGetClientData(client, GUAC_VNC_CLIENT_KEY);
    guac_vnc_client* vnc_client = (guac_vnc_client*) gc->data;

    /* Begin drawing operation directly to default layer */
    guac_display_layer* default_layer = guac_display_default_layer(vnc_client->display);
    guac_display_layer_raw_context* context = guac_display_layer_open_raw(default_layer);

    /* Convert operation coordinates to guac_rect for easier manipulation */
    guac_rect op_bounds;
    guac_rect_init(&op_bounds, x, y, w, h);

    /* Ensure draw is within current bounds of the pending frame */
    guac_rect_constrain(&op_bounds, &context->bounds);

    /* VNC framebuffer */
    unsigned int   vnc_bpp               = client->format.bitsPerPixel / 8;
    size_t         vnc_stride            = guac_mem_ckd_mul_or_die(vnc_bpp, client->width);
    const unsigned char* vnc_current_row = GUAC_RECT_CONST_BUFFER(op_bounds, client->frameBuffer, vnc_stride, vnc_bpp);

    unsigned char* layer_current_row = GUAC_RECT_MUTABLE_BUFFER(op_bounds, context->buffer, context->stride, GUAC_DISPLAY_LAYER_RAW_BPP);
    for (int dy = op_bounds.top; dy < op_bounds.bottom; dy++) {

        /* Get current Guacamole buffer row, advance to next */
        uint32_t* layer_current_pixel = (uint32_t*) layer_current_row;
        layer_current_row += context->stride;

        /* Get current VNC framebuffer row, advance to next */
        const unsigned char* vnc_current_pixel = vnc_current_row;
        vnc_current_row += vnc_stride;

        for (int dx = op_bounds.left; dx < op_bounds.right; dx++) {

            /* Read current VNC pixel value */
            uint32_t v;
            switch (vnc_bpp) {
                case 4:
                    v = *((uint32_t*) vnc_current_pixel);
                    break;

                case 2:
                    v = *((uint16_t*) vnc_current_pixel);
                    break;

                default:
                    v = *((uint8_t*) vnc_current_pixel);
            }

            /* Translate value to 32-bit RGB */
            uint8_t red   = (v >> client->format.redShift)   * 0x100 / (client->format.redMax   + 1);
            uint8_t green = (v >> client->format.greenShift) * 0x100 / (client->format.greenMax + 1);
            uint8_t blue  = (v >> client->format.blueShift)  * 0x100 / (client->format.blueMax  + 1);

            /* Output RGB */
            if (vnc_client->settings->swap_red_blue)
                *(layer_current_pixel++) = 0xFF000000 | (blue << 16) | (green << 8) | red;
            else
                *(layer_current_pixel++) = 0xFF000000 | (red  << 16) | (green << 8) | blue;

            /* Advance to next pixel in VNC framebuffer */
            vnc_current_pixel += vnc_bpp;

        }
    }

    /* Mark modified region as dirty */
    guac_rect_extend(&context->dirty, &op_bounds);

    /* Hint at source of copied data if this update involved CopyRect */
    if (vnc_client->copy_rect_used) {
        context->hint_from = default_layer;
        vnc_client->copy_rect_used = 0;
    }

    /* Draw operation is now complete */
    guac_display_layer_close_raw(default_layer, context);

}

void guac_vnc_update_finished(rfbClient* client) {

    guac_client* gc = rfbClientGetClientData(client, GUAC_VNC_CLIENT_KEY);
    guac_vnc_client* vnc_client = (guac_vnc_client*) gc->data;

    guac_display_end_multiple_frames(vnc_client->display, 1);

}

void guac_vnc_copyrect(rfbClient* client, int src_x, int src_y, int w, int h, int dest_x, int dest_y) {

    guac_client* gc = rfbClientGetClientData(client, GUAC_VNC_CLIENT_KEY);
    guac_vnc_client* vnc_client = (guac_vnc_client*) gc->data;

    vnc_client->copy_rect_used = 1;

}

void guac_vnc_set_pixel_format(rfbClient* client, int color_depth) {
    client->format.trueColour = 1;
    switch(color_depth) {
        case 8:
            client->format.depth        = 8;
            client->format.bitsPerPixel = 8;
            client->format.blueShift    = 6;
            client->format.redShift     = 0;
            client->format.greenShift   = 3;
            client->format.blueMax      = 3;
            client->format.redMax       = 7;
            client->format.greenMax     = 7;
            break;

        case 16:
            client->format.depth        = 16;
            client->format.bitsPerPixel = 16;
            client->format.blueShift    = 0;
            client->format.redShift     = 11;
            client->format.greenShift   = 5;
            client->format.blueMax      = 0x1f;
            client->format.redMax       = 0x1f;
            client->format.greenMax     = 0x3f;
            break;

        case 24:
        case 32:
        default:
            client->format.depth        = 24;
            client->format.bitsPerPixel = 32;
            client->format.blueShift    = 0;
            client->format.redShift     = 16;
            client->format.greenShift   = 8;
            client->format.blueMax      = 0xff;
            client->format.redMax       = 0xff;
            client->format.greenMax     = 0xff;
    }
}

rfbBool guac_vnc_malloc_framebuffer(rfbClient* rfb_client) {

    guac_client* gc = rfbClientGetClientData(rfb_client, GUAC_VNC_CLIENT_KEY);
    guac_vnc_client* vnc_client = (guac_vnc_client*) gc->data;

    /* Resize surface */
    if (vnc_client->display != NULL)
        guac_display_layer_resize(guac_display_default_layer(vnc_client->display),
                rfb_client->width, rfb_client->height);

    /* Use original, wrapped proc */
    return vnc_client->rfb_MallocFrameBuffer(rfb_client);
}

