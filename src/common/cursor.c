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

#include "common/blank_cursor.h"
#include "common/dot_cursor.h"
#include "common/cursor.h"
#include "common/ibar_cursor.h"
#include "common/pointer_cursor.h"
#include "common/surface.h"

#include <cairo/cairo.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/user.h>

#include <limits.h>
#include <stdlib.h>
#include <string.h>


/**
 * Allocates a cursor as well as an image buffer where the cursor is rendered.
 *
 * @param client
 *     The client owning the cursor.
 *
 * @return
 *     The newly-allocated cursor or NULL if cursor cannot be allocated.
 */
guac_common_cursor* guac_common_cursor_alloc(guac_client* client) {

    guac_common_cursor* cursor = malloc(sizeof(guac_common_cursor));
    if (cursor == NULL)
        return NULL;

    /* Associate cursor with client and allocate cursor buffer */
    cursor->client = client;
    cursor->buffer = guac_client_alloc_buffer(client);

    /* Allocate initial image buffer */
    cursor->image_buffer_size = GUAC_COMMON_CURSOR_DEFAULT_SIZE;
    cursor->image_buffer = malloc(cursor->image_buffer_size);

    /* No cursor image yet */
    cursor->width = 0;
    cursor->height = 0;
    cursor->surface = NULL;
    cursor->hotspot_x = 0;
    cursor->hotspot_y = 0;

    /* No user has moved the mouse yet */
    cursor->user = NULL;

    /* Start cursor in upper-left */
    cursor->x = 0;
    cursor->y = 0;

    return cursor;

}

void guac_common_cursor_free(guac_common_cursor* cursor) {

    guac_client* client = cursor->client;
    guac_layer* buffer = cursor->buffer;
    cairo_surface_t* surface = cursor->surface;

    /* Free image buffer and surface */
    free(cursor->image_buffer);
    if (surface != NULL)
        cairo_surface_destroy(surface);

    /* Destroy buffer within remotely-connected client */
    guac_protocol_send_dispose(client->socket, buffer);

    /* Return buffer to pool */
    guac_client_free_buffer(client, buffer);

    free(cursor);

}

void guac_common_cursor_dup(guac_common_cursor* cursor, guac_user* user,
        guac_socket* socket) {

    /* Synchronize location */
    guac_protocol_send_mouse(socket, cursor->x, cursor->y);

    /* Synchronize cursor image */
    if (cursor->surface != NULL) {
        guac_protocol_send_size(socket, cursor->buffer,
                cursor->width, cursor->height);

        guac_user_stream_png(user, socket, GUAC_COMP_SRC,
                cursor->buffer, 0, 0, cursor->surface);

        guac_protocol_send_cursor(socket,
                cursor->hotspot_x, cursor->hotspot_y,
                cursor->buffer, 0, 0, cursor->width, cursor->height);
    }

    guac_socket_flush(socket);

}

/**
 * Callback for guac_client_foreach_user() which sends the current cursor
 * position to any given user except the user that moved the cursor last.
 *
 * @param data
 *     A pointer to the guac_common_cursor whose position should be broadcast
 *     to all users except the user that moved the cursor last.
 *
 * @return
 *     Always NULL.
 */
static void* guac_common_cursor_broadcast_position(guac_user* user,
        void* data) {

    guac_common_cursor* cursor = (guac_common_cursor*) data;

    /* Send cursor position only if the user is not moving the cursor */
    if (user != cursor->user) {
        guac_protocol_send_mouse(user->socket, cursor->x, cursor->y);
        guac_socket_flush(user->socket);
    }

    return NULL;

}

void guac_common_cursor_move(guac_common_cursor* cursor, guac_user* user,
        int x, int y) {

    /* Update current user of cursor */
    cursor->user = user;

    /* Update cursor position */
    cursor->x = x;
    cursor->y = y;

    /* Notify all other users of change in cursor position */
    guac_client_foreach_user(cursor->client,
            guac_common_cursor_broadcast_position, cursor);

}

/**
 * Ensures the cursor image buffer has enough room to fit an image with the 
 * given characteristics. Existing image buffer data may be destroyed.
 *
 * @param cursor
 *     The cursor whose buffer size should be checked. If this cursor lacks
 *     sufficient space to contain a cursor image of the specified width,
 *     height, and stride, the current contents of this cursor will be
 *     destroyed and replaced with an new buffer having sufficient space.
 *
 * @param width
 *     The required cursor width, in pixels.
 *
 * @param height
 *     The required cursor height, in pixels.
 *
 * @param stride
 *     The number of bytes in each row of image data.
 */
static void guac_common_cursor_resize(guac_common_cursor* cursor,
        int width, int height, int stride) {

    int minimum_size = height * stride;

    /* Grow image buffer if necessary */
    if (cursor->image_buffer_size < minimum_size) {

        /* Calculate new size */
        cursor->image_buffer_size = minimum_size*2;

        /* Destructively reallocate image buffer */
        free(cursor->image_buffer);
        cursor->image_buffer = malloc(cursor->image_buffer_size);

    }

}

void guac_common_cursor_set_argb(guac_common_cursor* cursor, int hx, int hy,
    unsigned const char* data, int width, int height, int stride) {

    /* Copy image data */
    guac_common_cursor_resize(cursor, width, height, stride);
    memcpy(cursor->image_buffer, data, height * stride);

    if (cursor->surface != NULL)
        cairo_surface_destroy(cursor->surface);

    cursor->surface = cairo_image_surface_create_for_data(cursor->image_buffer,
            CAIRO_FORMAT_ARGB32, width, height, stride);

    /* Set new cursor parameters */
    cursor->width = width;
    cursor->height = height;
    cursor->hotspot_x = hx;
    cursor->hotspot_y = hy;

    /* Broadcast new cursor image to all users */
    guac_protocol_send_size(cursor->client->socket, cursor->buffer,
            width, height);

    guac_client_stream_png(cursor->client, cursor->client->socket,
            GUAC_COMP_SRC, cursor->buffer, 0, 0, cursor->surface);

    /* Update cursor image */
    guac_protocol_send_cursor(cursor->client->socket,
            cursor->hotspot_x, cursor->hotspot_y,
            cursor->buffer, 0, 0, cursor->width, cursor->height);

    guac_socket_flush(cursor->client->socket);

}

void guac_common_cursor_set_surface(guac_common_cursor* cursor, int hx, int hy,
    guac_common_surface* surface) {

    /* Set cursor to surface contents */
    guac_common_cursor_set_argb(cursor, hx, hy, surface->buffer,
            surface->width, surface->height, surface->stride);

}

void guac_common_cursor_set_pointer(guac_common_cursor* cursor) {

    guac_common_cursor_set_argb(cursor, 0, 0,
            guac_common_pointer_cursor,
            guac_common_pointer_cursor_width,
            guac_common_pointer_cursor_height,
            guac_common_pointer_cursor_stride);

}

void guac_common_cursor_set_dot(guac_common_cursor* cursor) {

     guac_common_cursor_set_argb(cursor, 2, 2,
            guac_common_dot_cursor,
            guac_common_dot_cursor_width,
            guac_common_dot_cursor_height,
            guac_common_dot_cursor_stride);

}

void guac_common_cursor_set_ibar(guac_common_cursor* cursor) {

     guac_common_cursor_set_argb(cursor,
            guac_common_ibar_cursor_width / 2,
            guac_common_ibar_cursor_height / 2,
            guac_common_ibar_cursor,
            guac_common_ibar_cursor_width,
            guac_common_ibar_cursor_height,
            guac_common_ibar_cursor_stride);

}

void guac_common_cursor_set_blank(guac_common_cursor* cursor) {

     guac_common_cursor_set_argb(cursor, 0, 0,
            guac_common_blank_cursor,
            guac_common_blank_cursor_width,
            guac_common_blank_cursor_height,
            guac_common_blank_cursor_stride);

}

void guac_common_cursor_remove_user(guac_common_cursor* cursor,
        guac_user* user) {

    /* Disassociate from given user */
    if (cursor->user == user)
        cursor->user = NULL;

}

