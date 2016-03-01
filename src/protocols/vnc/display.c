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
#include "guac_iconv.h"
#include "guac_surface.h"
#include "vnc.h"

#include <cairo/cairo.h>
#include <guacamole/client.h>
#include <guacamole/layer.h>
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
#include <stdlib.h>
#include <syslog.h>

void guac_vnc_update(rfbClient* client, int x, int y, int w, int h) {

    guac_client* gc = rfbClientGetClientData(client, GUAC_VNC_CLIENT_KEY);
    guac_vnc_client* vnc_client = (guac_vnc_client*) gc->data;

    int dx, dy;

    /* Cairo image buffer */
    int stride;
    unsigned char* buffer;
    unsigned char* buffer_row_current;
    cairo_surface_t* surface;

    /* VNC framebuffer */
    unsigned int bpp;
    unsigned int fb_stride;
    unsigned char* fb_row_current;

    /* Ignore extra update if already handled by copyrect */
    if (vnc_client->copy_rect_used) {
        vnc_client->copy_rect_used = 0;
        return;
    }

    /* Init Cairo buffer */
    stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, w);
    buffer = malloc(h*stride);
    buffer_row_current = buffer;

    bpp = client->format.bitsPerPixel/8;
    fb_stride = bpp * client->width;
    fb_row_current = client->frameBuffer + (y * fb_stride) + (x * bpp);

    /* Copy image data from VNC client to PNG */
    for (dy = y; dy<y+h; dy++) {

        unsigned int*  buffer_current;
        unsigned char* fb_current;
        
        /* Get current buffer row, advance to next */
        buffer_current      = (unsigned int*) buffer_row_current;
        buffer_row_current += stride;

        /* Get current framebuffer row, advance to next */
        fb_current      = fb_row_current;
        fb_row_current += fb_stride;

        for (dx = x; dx<x+w; dx++) {

            unsigned char red, green, blue;
            unsigned int v;

            switch (bpp) {
                case 4:
                    v = *((unsigned int*)   fb_current);
                    break;

                case 2:
                    v = *((unsigned short*) fb_current);
                    break;

                default:
                    v = *((unsigned char*)  fb_current);
            }

            /* Translate value to RGB */
            red   = (v >> client->format.redShift)   * 0x100 / (client->format.redMax  + 1);
            green = (v >> client->format.greenShift) * 0x100 / (client->format.greenMax+ 1);
            blue  = (v >> client->format.blueShift)  * 0x100 / (client->format.blueMax + 1);

            /* Output RGB */
            if (vnc_client->settings->swap_red_blue)
                *(buffer_current++) = (blue << 16) | (green << 8) | red;
            else
                *(buffer_current++) = (red  << 16) | (green << 8) | blue;

            fb_current += bpp;

        }
    }

    /* For now, only use default layer */
    surface = cairo_image_surface_create_for_data(buffer, CAIRO_FORMAT_RGB24, w, h, stride);

    guac_common_surface_draw(vnc_client->display->default_surface,
            x, y, surface);

    /* Free surface */
    cairo_surface_destroy(surface);
    free(buffer);

}

void guac_vnc_copyrect(rfbClient* client, int src_x, int src_y, int w, int h, int dest_x, int dest_y) {

    guac_client* gc = rfbClientGetClientData(client, GUAC_VNC_CLIENT_KEY);
    guac_vnc_client* vnc_client = (guac_vnc_client*) gc->data;

    /* For now, only use default layer */
    guac_common_surface_copy(vnc_client->display->default_surface,
            src_x, src_y, w, h,
            vnc_client->display->default_surface, dest_x, dest_y);

    vnc_client->copy_rect_used = 1;

}

void guac_vnc_set_pixel_format(rfbClient* client, int color_depth) {
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
        guac_common_surface_resize(vnc_client->display->default_surface,
                rfb_client->width, rfb_client->height);

    /* Use original, wrapped proc */
    return vnc_client->rfb_MallocFrameBuffer(rfb_client);
}

