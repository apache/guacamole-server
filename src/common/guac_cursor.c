/*
 * Copyright (C) 2014 Glyptodon LLC
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

#include "guac_blank_cursor.h"
#include "guac_dot_cursor.h"
#include "guac_cursor.h"
#include "guac_ibar_cursor.h"
#include "guac_pointer_cursor.h"
#include "guac_surface.h"

#include <cairo/cairo.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/user.h>

#include <stdlib.h>
#include <string.h>

guac_common_cursor* guac_common_cursor_alloc(guac_client* client) {

    guac_common_cursor* cursor = malloc(sizeof(guac_common_cursor));
    if (cursor == NULL)
        return NULL;

    /* Associate cursor with client and allocate cursor layer */
    cursor->client = client;
    cursor->layer= guac_client_alloc_layer(client);

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

    /* Free image buffer and surface */
    free(cursor->image_buffer);
    if (cursor->surface != NULL)
        cairo_surface_destroy(cursor->surface);

    /* Return layer to pool */
    guac_client_free_layer(cursor->client, cursor->layer);

    free(cursor);

}

void guac_common_cursor_dup(guac_common_cursor* cursor, guac_user* user,
        guac_socket* socket) {

    /* Synchronize location */
    guac_protocol_send_move(socket, cursor->layer, GUAC_DEFAULT_LAYER,
            cursor->x - cursor->hotspot_x,
            cursor->y - cursor->hotspot_y,
            0);

    /* Synchronize cursor image */
    if (cursor->surface != NULL) {
        guac_protocol_send_size(socket, cursor->layer,
                cursor->width, cursor->height);

        guac_user_stream_png(user, socket, GUAC_COMP_SRC,
                cursor->layer, 0, 0, cursor->surface);
    }

    guac_socket_flush(socket);

}

void guac_common_cursor_move(guac_common_cursor* cursor, guac_user* user,
        int x, int y) {

    guac_user* last_user = cursor->user;

    /* Update current user of cursor */
    if (last_user != user) {

        cursor->user = user;

        /* Make cursor layer visible to previous user */
        if (last_user != NULL) {
            guac_protocol_send_shade(last_user->socket, cursor->layer, 255);
            guac_socket_flush(last_user->socket);
        }

        /* Show hardware cursor */
        guac_protocol_send_cursor(user->socket,
                cursor->hotspot_x, cursor->hotspot_y,
                cursor->layer, 0, 0, cursor->width, cursor->height);

        /* Hide cursor layer from new user */
        guac_protocol_send_shade(user->socket, cursor->layer, 0);
        guac_socket_flush(user->socket);

    }

    /* Update cursor position */
    cursor->x = x;
    cursor->y = y;

    guac_protocol_send_move(cursor->client->socket, cursor->layer,
            GUAC_DEFAULT_LAYER,
            x - cursor->hotspot_x,
            y - cursor->hotspot_y,
            0);

    guac_socket_flush(cursor->client->socket);

}

/**
 * Ensures the cursor image buffer has enough room to fit an image with the 
 * given characteristics. Existing image buffer data may be destroyed.
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

/**
 * Callback for guac_client_foreach_user() which sends the current cursor image
 * as PNG data to each connected client.
 */
static void* __send_user_cursor_image(guac_user* user, void* data) {

    guac_common_cursor* cursor = (guac_common_cursor*) data;

    guac_user_stream_png(user, user->socket, GUAC_COMP_SRC,
            cursor->layer, 0, 0, cursor->surface);

    return NULL;

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

    /* Update location based on new hotspot */
    guac_protocol_send_move(cursor->client->socket, cursor->layer,
            GUAC_DEFAULT_LAYER,
            cursor->x - hx,
            cursor->y - hy,
            0);

    /* Broadcast new cursor image to all users */
    guac_protocol_send_size(cursor->client->socket, cursor->layer,
            width, height);

    guac_client_foreach_user(cursor->client, __send_user_cursor_image, cursor);

    guac_socket_flush(cursor->client->socket);

    /* Update hardware cursor of current user */
    if (cursor->user != NULL) {
        guac_protocol_send_cursor(cursor->user->socket, hx, hy,
                cursor->layer, 0, 0, width, height);

        guac_socket_flush(cursor->user->socket);
    }

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

