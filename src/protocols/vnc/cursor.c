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
#include "common/cursor.h"
#include "common/display.h"
#include "common/surface.h"
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
#include <stdint.h>
#include <stdlib.h>
#include <syslog.h>

void guac_vnc_cursor(rfbClient* client, int x, int y, int w, int h, int bpp) {

    guac_client* gc = rfbClientGetClientData(client, GUAC_VNC_CLIENT_KEY);
    guac_vnc_client* vnc_client = (guac_vnc_client*) gc->data;

    /* Cairo image buffer */
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, w);
    unsigned char* buffer = malloc(h*stride);
    unsigned char* buffer_row_current = buffer;

    /* VNC image buffer */
    unsigned int fb_stride = bpp * w;
    unsigned char* fb_row_current = client->rcSource;
    unsigned char* fb_mask = client->rcMask;

    int dx, dy;

    /* Copy image data from VNC client to RGBA buffer */
    for (dy = 0; dy<h; dy++) {

        unsigned int*  buffer_current;
        unsigned char* fb_current;
        
        /* Get current buffer row, advance to next */
        buffer_current      = (unsigned int*) buffer_row_current;
        buffer_row_current += stride;

        /* Get current framebuffer row, advance to next */
        fb_current      = fb_row_current;
        fb_row_current += fb_stride;

        for (dx = 0; dx<w; dx++) {

            unsigned char alpha, red, green, blue;
            unsigned int v;

            /* Read current pixel value */
            switch (bpp) {
                case 4:
                    v = *((uint32_t*) fb_current);
                    break;

                case 2:
                    v = *((uint16_t*) fb_current);
                    break;

                default:
                    v = *((uint8_t*)  fb_current);
            }

            /* Translate mask to alpha */
            if (*(fb_mask++)) alpha = 0xFF;
            else              alpha = 0x00;

            /* Translate value to RGB */
            red   = (v >> client->format.redShift)   * 0x100 / (client->format.redMax  + 1);
            green = (v >> client->format.greenShift) * 0x100 / (client->format.greenMax+ 1);
            blue  = (v >> client->format.blueShift)  * 0x100 / (client->format.blueMax + 1);

            /* Output ARGB */
            if (vnc_client->settings->swap_red_blue)
                *(buffer_current++) = (alpha << 24) | (blue << 16) | (green << 8) | red;
            else
                *(buffer_current++) = (alpha << 24) | (red  << 16) | (green << 8) | blue;

            /* Next VNC pixel */
            fb_current += bpp;

        }
    }

    /* Update stored cursor information */
    guac_common_cursor_set_argb(vnc_client->display->cursor, x, y,
            buffer, w, h, stride);

    /* Free surface */
    free(buffer);

    /* libvncclient does not free rcMask as it does rcSource */
    free(client->rcMask);
}

