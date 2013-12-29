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


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <cairo/cairo.h>

#include <guacamole/socket.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>

#include <freerdp/freerdp.h>
#include <freerdp/codec/color.h>
#include <freerdp/codec/bitmap.h>

#ifdef ENABLE_WINPR
#include <winpr/wtypes.h>
#else
#include "compat/winpr-wtypes.h"
#endif

#include "client.h"
#include "rdp_bitmap.h"

void guac_rdp_cache_bitmap(rdpContext* context, rdpBitmap* bitmap) {

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

        /* Get client data */
        guac_client* client = ((rdp_freerdp_context*) context)->client;
        rdp_guac_client_data* client_data = (rdp_guac_client_data*) client->data;

        /* Convert image data to 32-bit RGB */
        unsigned char* image_buffer = freerdp_image_convert(bitmap->data, NULL,
                bitmap->width, bitmap->height,
                client_data->settings.color_depth,
                32, ((rdp_freerdp_context*) context)->clrconv);

        /* Free existing image, if any */
        if (image_buffer != bitmap->data)
            free(bitmap->data);

        /* Store converted image in bitmap */
        bitmap->data = image_buffer;

    }

    /* No corresponding layer yet - caching is deferred. */
    ((guac_rdp_bitmap*) bitmap)->layer = NULL;

    /* Start at zero usage */
    ((guac_rdp_bitmap*) bitmap)->used = 0;

}

void guac_rdp_bitmap_paint(rdpContext* context, rdpBitmap* bitmap) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_socket* socket = client->socket;

    int width = bitmap->right - bitmap->left + 1;
    int height = bitmap->bottom - bitmap->top + 1;

    /* If not cached, cache if necessary */
    if (((guac_rdp_bitmap*) bitmap)->layer == NULL
            && ((guac_rdp_bitmap*) bitmap)->used >= 1)
        guac_rdp_cache_bitmap(context, bitmap);

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

    /* Increment usage counter */
    ((guac_rdp_bitmap*) bitmap)->used++;

}

void guac_rdp_bitmap_free(rdpContext* context, rdpBitmap* bitmap) {
    guac_client* client = ((rdp_freerdp_context*) context)->client;

    /* If cached, free buffer */
    if (((guac_rdp_bitmap*) bitmap)->layer != NULL)
        guac_client_free_buffer(client, ((guac_rdp_bitmap*) bitmap)->layer);

}

void guac_rdp_bitmap_setsurface(rdpContext* context, rdpBitmap* bitmap, BOOL primary) {
    guac_client* client = ((rdp_freerdp_context*) context)->client;

    if (primary)
        ((rdp_guac_client_data*) client->data)->current_surface
            = GUAC_DEFAULT_LAYER;

    else {

        /* Make sure that the recieved bitmap is not NULL before processing */
        if (bitmap == NULL) {
            guac_client_log_info(client, "NULL bitmap found in bitmap_setsurface instruction.");
            return;
        }

        /* If not available as a surface, make available. */
        if (((guac_rdp_bitmap*) bitmap)->layer == NULL)
            guac_rdp_cache_bitmap(context, bitmap);

        ((rdp_guac_client_data*) client->data)->current_surface 
            = ((guac_rdp_bitmap*) bitmap)->layer;

    }

}

#ifdef LEGACY_RDPBITMAP
void guac_rdp_bitmap_decompress(rdpContext* context, rdpBitmap* bitmap, UINT8* data,
        int width, int height, int bpp, int length, BOOL compressed) {
#else
void guac_rdp_bitmap_decompress(rdpContext* context, rdpBitmap* bitmap, UINT8* data,
        int width, int height, int bpp, int length, BOOL compressed, int codec_id) {
#endif

    int size = width * height * (bpp + 7) / 8;

    if (bitmap->data == NULL)
        bitmap->data = (UINT8*) malloc(size);
    else
        bitmap->data = (UINT8*) realloc(bitmap->data, size);

    if (compressed)
        bitmap_decompress(data, bitmap->data, width, height, length, bpp, bpp);
    else
        freerdp_image_flip(data, bitmap->data, width, height, bpp);

    bitmap->compressed = FALSE;
    bitmap->length = size;
    bitmap->bpp = bpp;

}

