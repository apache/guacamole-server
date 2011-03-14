
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
#include <png.h>
#include <time.h>

#include <guacamole/log.h>

#include <rfb/rfbclient.h>

#include <guacamole/guacio.h>
#include <guacamole/protocol.h>
#include <guacamole/client.h>

/* Client plugin arguments */
const char* GUAC_CLIENT_ARGS[] = {
    "hostname",
    "port",
    "read-only",
    "encodings",
    "password",
    NULL
};

static char* __GUAC_CLIENT = "GUAC_CLIENT";

typedef struct vnc_guac_client_data {
    
    rfbClient* rfb_client;
    MallocFrameBufferProc rfb_MallocFrameBuffer;

    png_byte** png_buffer;
    png_byte** png_buffer_alpha;
    int buffer_height;

    int copy_rect_used;
    char* password;
    char* encodings;

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

    /* For now, only use layer 0 */
    guac_send_png(io, 0, x, y, png_buffer, w, h);

}

void guac_vnc_copyrect(rfbClient* client, int src_x, int src_y, int w, int h, int dest_x, int dest_y) {

    guac_client* gc = rfbClientGetClientData(client, __GUAC_CLIENT);
    GUACIO* io = gc->io;

    /* For now, only use layer 0 */
    guac_send_copy(io, 0, src_x, src_y, w, h, 0, dest_x, dest_y);
    ((vnc_guac_client_data*) gc->data)->copy_rect_used = 1;

}

char* guac_vnc_get_password(rfbClient* client) {
    guac_client* gc = rfbClientGetClientData(client, __GUAC_CLIENT);
    return ((vnc_guac_client_data*) gc->data)->password;
}

rfbBool guac_vnc_malloc_framebuffer(rfbClient* rfb_client) {

    guac_client* gc = rfbClientGetClientData(rfb_client, __GUAC_CLIENT);
    vnc_guac_client_data* guac_client_data = (vnc_guac_client_data*) gc->data;

    /* Free old buffers */
    if (guac_client_data->png_buffer != NULL)
        guac_free_png_buffer(guac_client_data->png_buffer, guac_client_data->buffer_height);
    if (guac_client_data->png_buffer_alpha != NULL)
        guac_free_png_buffer(guac_client_data->png_buffer_alpha, guac_client_data->buffer_height);

    /* Allocate new buffers */
    guac_client_data->png_buffer = guac_alloc_png_buffer(rfb_client->width, rfb_client->height, 3); /* No-alpha */
    guac_client_data->png_buffer_alpha = guac_alloc_png_buffer(rfb_client->width, rfb_client->height, 4); /* With alpha */
    guac_client_data->buffer_height = rfb_client->height;

    /* Send new size */
    guac_send_size(gc->io, rfb_client->width, rfb_client->height);

    /* Use original, wrapped proc */
    return guac_client_data->rfb_MallocFrameBuffer(rfb_client);
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
        GUAC_LOG_ERROR("Error waiting for VNC server message\n");
        return 1;
    }

    if (wait_result > 0) {

        if (!HandleRFBServerMessage(rfb_client)) {
            GUAC_LOG_ERROR("Error handling VNC server message\n");
            return 1;
        }

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

    vnc_guac_client_data* guac_client_data = (vnc_guac_client_data*) client->data;

    rfbClient* rfb_client = guac_client_data->rfb_client;
    png_byte** png_buffer = guac_client_data->png_buffer;
    png_byte** png_buffer_alpha = guac_client_data->png_buffer_alpha;

    /* Free PNG data */
    guac_free_png_buffer(png_buffer, guac_client_data->buffer_height);
    guac_free_png_buffer(png_buffer_alpha, guac_client_data->buffer_height);

    /* Free encodings string, if used */
    if (guac_client_data->encodings != NULL)
        free(guac_client_data->encodings);

    /* Free generic data struct */
    free(client->data);

    /* Clean up VNC client*/
    rfbClientCleanup(rfb_client);

    return 0;
}


int guac_client_init(guac_client* client, int argc, char** argv) {

    rfbClient* rfb_client;

    png_byte** png_buffer;
    png_byte** png_buffer_alpha;

    vnc_guac_client_data* guac_client_data;

    int read_only = 0;

    /*** PARSE ARGUMENTS ***/

    if (argc < 5) {
        guac_send_error(client->io, "Wrong argument count received.");
        guac_flush(client->io);
        return 1;
    }

    /* Alloc client data */
    guac_client_data = malloc(sizeof(vnc_guac_client_data));
    client->data = guac_client_data;

    /* If read-only specified, set flag */
    if (strcmp(argv[2], "true") == 0)
        read_only = 1;

    /* Freed after use by libvncclient */
    guac_client_data->password = strdup(argv[4]);

    /*** INIT RFB CLIENT ***/

    rfb_client = rfbGetClient(8, 3, 4); /* 32-bpp client */

    /* Store Guac client in rfb client */
    rfbClientSetClientData(rfb_client, __GUAC_CLIENT, client);

    /* Framebuffer update handler */
    rfb_client->GotFrameBufferUpdate = guac_vnc_update;
    rfb_client->GotCopyRect = guac_vnc_copyrect;

    /* Do not handle clipboard and local cursor if read-only */
    if (read_only == 0) {
        /* Enable client-side cursor */
        rfb_client->GotCursorShape = guac_vnc_cursor;
        rfb_client->appData.useRemoteCursor = TRUE;

        /* Clipboard */
        rfb_client->GotXCutText = guac_vnc_cut_text;
    }

    /* Password */
    rfb_client->GetPassword = guac_vnc_get_password;
    
    /* Hook into allocation so we can handle resize. */
    guac_client_data->png_buffer = NULL;
    guac_client_data->png_buffer_alpha = NULL;
    guac_client_data->rfb_MallocFrameBuffer = rfb_client->MallocFrameBuffer;
    rfb_client->MallocFrameBuffer = guac_vnc_malloc_framebuffer;
    rfb_client->canHandleNewFBSize = 1;

    /* Set hostname and port */
    rfb_client->serverHost = strdup(argv[0]);
    rfb_client->serverPort = atoi(argv[1]);

    /* Set encodings if specified */
    if (argv[3][0] != '\0')
        rfb_client->appData.encodingsString = guac_client_data->encodings = strdup(argv[3]);
    else
        guac_client_data->encodings = NULL;

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
    guac_client_data->buffer_height = rfb_client->height;
    guac_client_data->copy_rect_used = 0;

    /* Set handlers */
    client->handle_messages = vnc_guac_client_handle_messages;
    if (read_only == 0) {
        /* Do not handle mouse/keyboard/clipboard if read-only */
        client->mouse_handler = vnc_guac_client_mouse_handler;
        client->key_handler = vnc_guac_client_key_handler;
        client->clipboard_handler = vnc_guac_client_clipboard_handler;
    }

    /* Send name */
    guac_send_name(client->io, rfb_client->desktopName);

    /* Send size */
    guac_send_size(client->io, rfb_client->width, rfb_client->height);

    return 0;

}

