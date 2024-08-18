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
#include "config.h"
#include "rdp.h"

#include <freerdp/freerdp.h>
#include <guacamole/display.h>
#include <winpr/crt.h>
#include <winpr/wtypes.h>

#include <stdio.h>
#include <stdlib.h>

void guac_rdp_cache_bitmap(rdpContext* context, rdpBitmap* bitmap) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Allocate buffer */
    guac_display_layer* buffer = guac_display_alloc_buffer(rdp_client->display, 1);
    guac_display_layer_resize(buffer, bitmap->width, bitmap->height);

    /* Cache image data if present */
    if (bitmap->data != NULL) {

        guac_display_layer_raw_context* dst_context = guac_display_layer_open_raw(buffer);

        guac_rect dst_rect = {
            .left   = 0,
            .top    = 0,
            .right  = bitmap->width,
            .bottom = bitmap->height
        };

        guac_rect_constrain(&dst_rect, &dst_context->bounds);

        guac_display_layer_raw_context_put(dst_context, &dst_rect, bitmap->data, 4 * bitmap->width);

        guac_rect_extend(&dst_context->dirty, &dst_rect);

        guac_display_layer_close_raw(buffer, dst_context);

    }

    /* Store buffer reference in bitmap */
    ((guac_rdp_bitmap*) bitmap)->layer = buffer;

}

BOOL guac_rdp_bitmap_new(rdpContext* context, rdpBitmap* bitmap) {

    /* No corresponding surface yet - caching is deferred. */
    ((guac_rdp_bitmap*) bitmap)->layer = NULL;

    /* Start at zero usage */
    ((guac_rdp_bitmap*) bitmap)->used = 0;

    return TRUE;

}

BOOL guac_rdp_bitmap_paint(rdpContext* context, rdpBitmap* bitmap) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    guac_display_layer* buffer = ((guac_rdp_bitmap*) bitmap)->layer;

    /* If not cached, cache if necessary */
    if (buffer == NULL && ((guac_rdp_bitmap*) bitmap)->used >= 1)
        guac_rdp_cache_bitmap(context, bitmap);

    guac_display_layer* default_layer = guac_display_default_layer(rdp_client->display);
    guac_display_layer_raw_context* dst_context = guac_display_layer_open_raw(default_layer);

    guac_rect dst_rect = {
        .left   = bitmap->left,
        .top    = bitmap->top,
        .right  = bitmap->right,
        .bottom = bitmap->bottom
    };

    guac_rect_constrain(&dst_rect, &dst_context->bounds);

    /* If cached, retrieve from cache */
    if (buffer != NULL) {
        guac_display_layer_raw_context* src_context = guac_display_layer_open_raw(buffer);
        guac_display_layer_raw_context_put(dst_context, &dst_rect, src_context->buffer, src_context->stride);
    }

    /* Otherwise, draw with stored image data */
    else if (bitmap->data != NULL) {
        guac_display_layer_raw_context_put(dst_context, &dst_rect, bitmap->data, 4 * bitmap->width);
    }

    /* Increment usage counter */
    ((guac_rdp_bitmap*) bitmap)->used++;

    guac_rect_extend(&dst_context->dirty, &dst_rect);
    dst_context->hint_from = buffer;

    guac_display_layer_close_raw(default_layer, dst_context);
    return TRUE;

}

void guac_rdp_bitmap_free(rdpContext* context, rdpBitmap* bitmap) {

    guac_display_layer* buffer = ((guac_rdp_bitmap*) bitmap)->layer;

    /* If cached, free buffer */
    if (buffer != NULL)
        guac_display_free_layer(buffer);

#ifndef FREERDP_BITMAP_FREE_FREES_BITMAP
    /* NOTE: Except in FreeRDP 2.0.0-rc0 and earlier, FreeRDP-allocated memory
     * for the rdpBitmap will NOT be automatically released after this free
     * handler is invoked, thus we must do so manually here */
    GUAC_ALIGNED_FREE(bitmap->data);
    free(bitmap);
#endif

}

BOOL guac_rdp_bitmap_setsurface(rdpContext* context, rdpBitmap* bitmap, BOOL primary) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    if (primary)
        rdp_client->current_surface = guac_display_default_layer(rdp_client->display);

    else {

        /* Make sure that the received bitmap is not NULL before processing */
        if (bitmap == NULL) {
            guac_client_log(client, GUAC_LOG_INFO, "NULL bitmap found in bitmap_setsurface instruction.");
            return TRUE;
        }

        /* If not available as a surface, make available. */
        if (((guac_rdp_bitmap*) bitmap)->layer == NULL)
            guac_rdp_cache_bitmap(context, bitmap);

        rdp_client->current_surface = ((guac_rdp_bitmap*) bitmap)->layer;

    }

    return TRUE;

}
