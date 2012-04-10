
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
 * The Original Code is libguac-client-rdp.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * Matt Hortman
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

#include <stdio.h>
#include <stdlib.h>

#include <cairo/cairo.h>

#include <guacamole/socket.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>

#include <freerdp/freerdp.h>
#include <freerdp/utils/memory.h>
#include <freerdp/codec/color.h>
#include <freerdp/codec/bitmap.h>

#include "client.h"
#include "rdp_bitmap.h"

void __guac_rdp_cache_bitmap(rdpContext* context, rdpBitmap* bitmap) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_socket* socket = client->socket; 

    /* Allocate buffer */
    guac_layer* buffer = guac_client_alloc_buffer(client);

    /* Cache image data if present */
    if (bitmap->data != NULL) {

        /* Create surface from image data */
        cairo_surface_t* surface = cairo_image_surface_create_for_data(
            bitmap->data, CAIRO_FORMAT_RGB24,
            bitmap->width, bitmap->height, 4*bitmap->width);

        /* Send surface to buffer */
        guac_protocol_send_png(socket,
                GUAC_COMP_SRC, buffer, 0, 0, surface);

        /* Free surface */
        cairo_surface_destroy(surface);

    }

    /* Store buffer reference in bitmap */
    ((guac_rdp_bitmap*) bitmap)->layer = buffer;

}


void guac_rdp_bitmap_new(rdpContext* context, rdpBitmap* bitmap) {

    /* Convert image data if present */
    if (bitmap->data != NULL) {

        /* Convert image data to 32-bit RGB */
        unsigned char* image_buffer = freerdp_image_convert(bitmap->data, NULL,
                bitmap->width, bitmap->height,
                context->instance->settings->color_depth,
                32, ((rdp_freerdp_context*) context)->clrconv);

        /* Free existing image, if any */
        if (image_buffer != bitmap->data)
            free(bitmap->data);

        /* Store converted image in bitmap */
        bitmap->data = image_buffer;

        /* If not ephemeral, store cached image */
        if (!bitmap->ephemeral)
            __guac_rdp_cache_bitmap(context, bitmap);

        else
            /* No corresponding layer */
            ((guac_rdp_bitmap*) bitmap)->layer = NULL;

    }

    else
        /* No corresponding layer */
        ((guac_rdp_bitmap*) bitmap)->layer = NULL;

}

void guac_rdp_bitmap_paint(rdpContext* context, rdpBitmap* bitmap) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_socket* socket = client->socket;

    int width = bitmap->right - bitmap->left + 1;
    int height = bitmap->bottom - bitmap->top + 1;

    /* If cached, retrieve from cache */
    if (((guac_rdp_bitmap*) bitmap)->layer != NULL)
        guac_protocol_send_copy(socket,
                ((guac_rdp_bitmap*) bitmap)->layer,
                0, 0, width, height,
                GUAC_COMP_OVER,
                GUAC_DEFAULT_LAYER, bitmap->left, bitmap->top);

    /* Otherwise, draw with stored image data */
    else if (bitmap->data != NULL) {

        /* Create surface from image data */
        cairo_surface_t* surface = cairo_image_surface_create_for_data(
            bitmap->data, CAIRO_FORMAT_RGB24,
            width, height, 4*bitmap->width);

        /* Send surface to buffer */
        guac_protocol_send_png(socket,
                GUAC_COMP_OVER, GUAC_DEFAULT_LAYER,
                bitmap->left, bitmap->top, surface);

        /* Free surface */
        cairo_surface_destroy(surface);

    }

}

void guac_rdp_bitmap_free(rdpContext* context, rdpBitmap* bitmap) {
    guac_client* client = ((rdp_freerdp_context*) context)->client;

    /* Free layer, if any */
    if (((guac_rdp_bitmap*) bitmap)->layer != NULL)
        guac_client_free_buffer(client, ((guac_rdp_bitmap*) bitmap)->layer);
}

void guac_rdp_bitmap_setsurface(rdpContext* context, rdpBitmap* bitmap, boolean primary) {
    guac_client* client = ((rdp_freerdp_context*) context)->client;

    if (primary)
        ((rdp_guac_client_data*) client->data)->current_surface
            = GUAC_DEFAULT_LAYER;

    else {

        /* If not available as a surface, make available. */
        if (((guac_rdp_bitmap*) bitmap)->layer == NULL)
            __guac_rdp_cache_bitmap(context, bitmap);

        ((rdp_guac_client_data*) client->data)->current_surface 
            = ((guac_rdp_bitmap*) bitmap)->layer;

    }

}

void guac_rdp_bitmap_decompress(rdpContext* context, rdpBitmap* bitmap, uint8* data, int width, int height, int bpp, int length, boolean compressed) {

    int size = width * height * (bpp + 7) / 8;

    if (bitmap->data == NULL)
        bitmap->data = (uint8*) xmalloc(size);
    else
        bitmap->data = (uint8*) xrealloc(bitmap->data, size);

    if (compressed)
        bitmap_decompress(data, bitmap->data, width, height, length, bpp, bpp);
    else
        freerdp_image_flip(data, bitmap->data, width, height, bpp);

    bitmap->compressed = false;
    bitmap->length = size;
    bitmap->bpp = bpp;

}

