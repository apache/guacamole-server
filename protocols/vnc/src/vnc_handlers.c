
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is libguac-client-vnc.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2010
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <syslog.h>

#include <cairo/cairo.h>

#include <rfb/rfbclient.h>

#include <guacamole/socket.h>
#include <guacamole/protocol.h>
#include <guacamole/client.h>

void guac_vnc_cursor(rfbClient* client, int x, int y, int w, int h, int bpp) {

    guac_client* gc = rfbClientGetClientData(client, __GUAC_CLIENT);
    guac_socket* socket = gc->socket;

    /* Cairo image buffer */
    int stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, w);
    unsigned char* buffer = malloc(h*stride);
    unsigned char* buffer_row_current = buffer;
    cairo_surface_t* surface;

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
                    v = *((unsigned int*)   fb_current);
                    break;

                case 2:
                    v = *((unsigned short*) fb_current);
                    break;

                default:
                    v = *((unsigned char*)  fb_current);
            }

            /* Translate mask to alpha */
            if (*(fb_mask++)) alpha = 0xFF;
            else              alpha = 0x00;

            /* Translate value to RGB */
            red   = (v >> client->format.redShift)   * 0x100 / (client->format.redMax  + 1);
            green = (v >> client->format.greenShift) * 0x100 / (client->format.greenMax+ 1);
            blue  = (v >> client->format.blueShift)  * 0x100 / (client->format.blueMax + 1);

            /* Output ARGB */
            *(buffer_current++) = (alpha << 24) | (red << 16) | (green << 8) | blue;

            /* Next VNC pixel */
            fb_current += bpp;

        }
    }

    /* SEND CURSOR */
    surface = cairo_image_surface_create_for_data(buffer, CAIRO_FORMAT_ARGB32, w, h, stride);
    guac_protocol_send_cursor(socket, x, y, surface);

    /* Free surface */
    cairo_surface_destroy(surface);
    free(buffer);

    /* libvncclient does not free rcMask as it does rcSource */
    free(client->rcMask);
}


void guac_vnc_update(rfbClient* client, int x, int y, int w, int h) {

    guac_client* gc = rfbClientGetClientData(client, __GUAC_CLIENT);
    guac_socket* socket = gc->socket;

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
    if (((vnc_guac_client_data*) gc->data)->copy_rect_used) {
        ((vnc_guac_client_data*) gc->data)->copy_rect_used = 0;
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
            *(buffer_current++) = (red << 16) | (green << 8) | blue;

            fb_current += bpp;

        }
    }

    /* For now, only use default layer */
    surface = cairo_image_surface_create_for_data(buffer, CAIRO_FORMAT_RGB24, w, h, stride);
    guac_protocol_send_png(socket, GUAC_COMP_OVER, GUAC_DEFAULT_LAYER, x, y, surface);

    /* Free surface */
    cairo_surface_destroy(surface);
    free(buffer);

}

void guac_vnc_copyrect(rfbClient* client, int src_x, int src_y, int w, int h, int dest_x, int dest_y) {

    guac_client* gc = rfbClientGetClientData(client, __GUAC_CLIENT);
    guac_socket* socket = gc->socket;

    /* For now, only use default layer */
    guac_protocol_send_copy(socket,
                            GUAC_DEFAULT_LAYER, src_x,  src_y, w, h,
            GUAC_COMP_OVER, GUAC_DEFAULT_LAYER, dest_x, dest_y);

    ((vnc_guac_client_data*) gc->data)->copy_rect_used = 1;

}

char* guac_vnc_get_password(rfbClient* client) {
    guac_client* gc = rfbClientGetClientData(client, __GUAC_CLIENT);
    return ((vnc_guac_client_data*) gc->data)->password;
}

rfbBool guac_vnc_malloc_framebuffer(rfbClient* rfb_client) {

    guac_client* gc = rfbClientGetClientData(rfb_client, __GUAC_CLIENT);
    vnc_guac_client_data* guac_client_data = (vnc_guac_client_data*) gc->data;

    /* Send new size */
    guac_protocol_send_size(gc->socket, rfb_client->width, rfb_client->height);

    /* Use original, wrapped proc */
    return guac_client_data->rfb_MallocFrameBuffer(rfb_client);
}

void guac_vnc_cut_text(rfbClient* client, const char* text, int textlen) {

    guac_client* gc = rfbClientGetClientData(client, __GUAC_CLIENT);
    guac_socket* socket = gc->socket;

    guac_protocol_send_clipboard(socket, text);

}

void guac_vnc_client_log_info(const char* format, ...) {

    va_list args;
    va_start(args, format);

    vsyslog(LOG_INFO, format, args);

    va_end(args);

}

void guac_vnc_client_log_error(const char* format, ...) {

    va_list args;
    va_start(args, format);

    vsyslog(LOG_ERR, format, args);

    va_end(args);

}

