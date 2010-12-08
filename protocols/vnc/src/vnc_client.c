
/*
 *  Guacamole - Clientless Remote Desktop
 *  Copyright (C) 2010  Michael Jumper
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <png.h>
#include <time.h>

#include <syslog.h>

#include <rfb/rfbclient.h>

#include <guacamole/guacio.h>
#include <guacamole/protocol.h>
#include <guacamole/client.h>

static char* __GUAC_CLIENT = "GUAC_CLIENT";

typedef struct vnc_guac_client_data {
    
    rfbClient* rfb_client;
    png_byte** png_buffer;
    png_byte** png_buffer_alpha;
    int copy_rect_used;
    char* password;

} vnc_guac_client_data;

void guac_vnc_cursor(rfbClient* client, int x, int y, int w, int h, int bpp) {

    int dx, dy;

    guac_client* gc = rfbClientGetClientData(client, __GUAC_CLIENT);
    GUACIO* io = gc->io;
    png_byte** png_buffer = ((vnc_guac_client_data*) gc->data)->png_buffer_alpha;
    png_byte* row;

    png_byte** png_row_current = png_buffer;

    unsigned int bytesPerRow = bpp * w;
    unsigned char* fb_row_current = client->rcSource;
    unsigned char* fb_mask = client->rcMask;
    unsigned char* fb_row;
    unsigned int v;

    /* Copy image data from VNC client to PNG */
    for (dy = 0; dy<h; dy++) {

        row = *(png_row_current++);

        fb_row = fb_row_current;
        fb_row_current += bytesPerRow;

        for (dx = 0; dx<w; dx++) {

            switch (bpp) {
                case 4:
                    v = *((unsigned int*) fb_row);
                    break;

                case 2:
                    v = *((unsigned short*) fb_row);
                    break;

                default:
                    v = *((unsigned char*) fb_row);
            }

            *(row++) = (v >> client->format.redShift) * 256 / (client->format.redMax+1);
            *(row++) = (v >> client->format.greenShift) * 256 / (client->format.greenMax+1);
            *(row++) = (v >> client->format.blueShift) * 256 / (client->format.blueMax+1);

            /* Handle mask */
            if (*(fb_mask++))
                *(row++) = 255;
            else
                *(row++) = 0;

            fb_row += bpp;

        }
    }

    /* SEND CURSOR */
    guac_send_cursor(io, x, y, png_buffer, w, h);

}


void guac_vnc_update(rfbClient* client, int x, int y, int w, int h) {

    int dx, dy;

    guac_client* gc = rfbClientGetClientData(client, __GUAC_CLIENT);
    GUACIO* io = gc->io;
    png_byte** png_buffer = ((vnc_guac_client_data*) gc->data)->png_buffer;
    png_byte* row;

    png_byte** png_row_current = png_buffer;

    unsigned int bpp = client->format.bitsPerPixel/8;
    unsigned int bytesPerRow = bpp * client->width;
    unsigned char* fb_row_current = client->frameBuffer + (y * bytesPerRow) + (x * bpp);
    unsigned char* fb_row;
    unsigned int v;

    /* Ignore extra update if already handled by copyrect */
    if (((vnc_guac_client_data*) gc->data)->copy_rect_used) {
        ((vnc_guac_client_data*) gc->data)->copy_rect_used = 0;
        return;
    }

    /* Copy image data from VNC client to PNG */
    for (dy = y; dy<y+h; dy++) {

        row = *(png_row_current++);

        fb_row = fb_row_current;
        fb_row_current += bytesPerRow;

        for (dx = x; dx<x+w; dx++) {

            switch (bpp) {
                case 4:
                    v = *((unsigned int*) fb_row);
                    break;

                case 2:
                    v = *((unsigned short*) fb_row);
                    break;

                default:
                    v = *((unsigned char*) fb_row);
            }

            *(row++) = (v >> client->format.redShift) * 256 / (client->format.redMax+1);
            *(row++) = (v >> client->format.greenShift) * 256 / (client->format.greenMax+1);
            *(row++) = (v >> client->format.blueShift) * 256 / (client->format.blueMax+1);

            fb_row += bpp;

        }
    }

    guac_send_png(io, x, y, png_buffer, w, h);

}

void guac_vnc_copyrect(rfbClient* client, int src_x, int src_y, int w, int h, int dest_x, int dest_y) {

    guac_client* gc = rfbClientGetClientData(client, __GUAC_CLIENT);
    GUACIO* io = gc->io;

    guac_send_copy(io, src_x, src_y, w, h, dest_x, dest_y);
    ((vnc_guac_client_data*) gc->data)->copy_rect_used = 1;

}

char* guac_vnc_get_password(rfbClient* client) {
    guac_client* gc = rfbClientGetClientData(client, __GUAC_CLIENT);
    return ((vnc_guac_client_data*) gc->data)->password;
}


void guac_vnc_cut_text(rfbClient* client, const char* text, int textlen) {

    guac_client* gc = rfbClientGetClientData(client, __GUAC_CLIENT);
    GUACIO* io = gc->io;

    guac_send_clipboard(io, text);

}


int vnc_guac_client_handle_messages(guac_client* client) {

    int wait_result;
    rfbClient* rfb_client = ((vnc_guac_client_data*) client->data)->rfb_client;


    wait_result = WaitForMessage(rfb_client, 2000);
    if (wait_result < 0) {
        syslog(LOG_ERR, "Error waiting for VNC server message\n");
        return 1;
    }

    if (wait_result > 0) {

        struct timespec sleep_period;

        if (!HandleRFBServerMessage(rfb_client)) {
            syslog(LOG_ERR, "Error handling VNC server message\n");
            return 1;
        }

        /* Wait before returning ... don't want to handle
         * too many server messages. */

        sleep_period.tv_sec = 0;
        sleep_period.tv_nsec = 50000000L /* 50 ms */;

        nanosleep(&sleep_period, NULL);

    }

    return 0;

}


int vnc_guac_client_mouse_handler(guac_client* client, int x, int y, int mask) {

    rfbClient* rfb_client = ((vnc_guac_client_data*) client->data)->rfb_client;

    SendPointerEvent(rfb_client, x, y, mask);

    return 0;
}

int vnc_guac_client_key_handler(guac_client* client, int keysym, int pressed) {

    rfbClient* rfb_client = ((vnc_guac_client_data*) client->data)->rfb_client;

    SendKeyEvent(rfb_client, keysym, pressed);

    return 0;
}

int vnc_guac_client_clipboard_handler(guac_client* client, char* data) {

    rfbClient* rfb_client = ((vnc_guac_client_data*) client->data)->rfb_client;

    SendClientCutText(rfb_client, data, strlen(data));

    return 0;
}

int vnc_guac_client_free_handler(guac_client* client) {

    rfbClient* rfb_client = ((vnc_guac_client_data*) client->data)->rfb_client;
    png_byte** png_buffer = ((vnc_guac_client_data*) client->data)->png_buffer;
    png_byte** png_buffer_alpha = ((vnc_guac_client_data*) client->data)->png_buffer_alpha;

    /* Free PNG data */
    guac_free_png_buffer(png_buffer, rfb_client->height);
    guac_free_png_buffer(png_buffer_alpha, rfb_client->height);

    /* Clean up VNC client*/
    rfbClientCleanup(rfb_client);

    return 0;
}


int guac_client_init(guac_client* client, int argc, char** argv) {

    rfbClient* rfb_client;

    png_byte** png_buffer;
    png_byte** png_buffer_alpha;

    vnc_guac_client_data* guac_client_data;

    /*** INIT ***/
    rfb_client = rfbGetClient(8, 3, 4); /* 32-bpp client */

    /* Framebuffer update handler */
    rfb_client->GotFrameBufferUpdate = guac_vnc_update;
    rfb_client->GotCopyRect = guac_vnc_copyrect;

    /* Enable client-side cursor */
    rfb_client->GotCursorShape = guac_vnc_cursor;
    rfb_client->appData.useRemoteCursor = TRUE;

    /* Clipboard */
    rfb_client->GotXCutText = guac_vnc_cut_text;

    /* Password */
    rfb_client->GetPassword = guac_vnc_get_password;

    if (argc < 3) {
        guac_send_error(client->io, "VNC client requires hostname and port arguments");
        guac_flush(client->io);
        return 1;
    }

    /* Alloc client data */
    guac_client_data = malloc(sizeof(vnc_guac_client_data));
    client->data = guac_client_data;

    /* Store Guac client in rfb client */
    rfbClientSetClientData(rfb_client, __GUAC_CLIENT, client);

    /* Parse password from args if provided */
    if (argc >= 4) {

        /* Freed after use by libvncclient */
        guac_client_data->password = malloc(64);
        strncpy(guac_client_data->password, argv[3], 63);

    }
    else {

        /* Freed after use by libvncclient */
        guac_client_data->password = malloc(64);
        guac_client_data->password[0] = '\0';

    }

    /* Set hostname and port */
    rfb_client->serverHost = strdup(argv[1]);
    rfb_client->serverPort = atoi(argv[2]);

    /* Connect */
    if (!rfbInitClient(rfb_client, NULL, NULL)) {
        guac_send_error(client->io, "Error initializing VNC client");
        guac_flush(client->io);
        return 1;
    }

    /* Allocate buffers */
    png_buffer = guac_alloc_png_buffer(rfb_client->width, rfb_client->height, 3); /* No-alpha */
    png_buffer_alpha = guac_alloc_png_buffer(rfb_client->width, rfb_client->height, 4); /* With alpha */

    /* Set remaining client data */
    guac_client_data->rfb_client = rfb_client;
    guac_client_data->png_buffer = png_buffer;
    guac_client_data->png_buffer_alpha = png_buffer_alpha;
    guac_client_data->copy_rect_used = 0;

    /* Set handlers */
    client->handle_messages = vnc_guac_client_handle_messages;
    client->mouse_handler = vnc_guac_client_mouse_handler;
    client->key_handler = vnc_guac_client_key_handler;
    client->clipboard_handler = vnc_guac_client_clipboard_handler;

    /* Send name */
    guac_send_name(client->io, rfb_client->desktopName);

    /* Send size */
    guac_send_size(client->io, rfb_client->width, rfb_client->height);

    return 0;

}

