
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
#include "guac_drawable.h"
#include "guac_drv.h"
#include "guac_input.h"
#include "guac_protocol.h"
#include "io.h"
#include "log.h"

#include <guacamole/error.h>
#include <guacamole/client.h>
#include <guacamole/layer.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/timestamp.h>
#include <guacamole/user.h>

void guac_drv_send_create_drawable(guac_socket* socket,
        guac_drv_drawable* drawable) {

    /* Initialize drawable */
    guac_drv_send_move_drawable(socket, drawable);
    guac_drv_send_shade_drawable(socket, drawable);
    guac_drv_send_resize_drawable(socket, drawable);

}

void guac_drv_send_shade_drawable(guac_socket* socket,
        guac_drv_drawable* drawable) {

    /* Only applies to non-default layers */
    if (drawable->index > 0) {

        /* Create layer representation of drawable */
        guac_layer layer;
        layer.index = drawable->index;

        guac_protocol_send_shade(socket, &layer, drawable->pending.opacity);

    }

}

void guac_drv_send_destroy_drawable(guac_socket* socket,
        guac_drv_drawable* drawable) {

    /* Create layer representation of drawable */
    guac_layer layer;
    layer.index = drawable->index;

    /* Dispose if layer */
    if (drawable->index > 0)
        guac_protocol_send_dispose(socket, &layer);

    /* Clear data if buffer */
    else if (drawable->index < 0) {

        guac_protocol_send_rect(socket, &layer, 0, 0,
                drawable->pending.rect.width,
                drawable->pending.rect.height);

        guac_protocol_send_cfill(socket, GUAC_COMP_SRC, &layer, 0, 0, 0, 0);

    }

}

void guac_drv_send_move_drawable(guac_socket* socket,
        guac_drv_drawable* drawable) {

    /* Only applies to non-default layers */
    if (drawable->index > 0) {

        /* Create layer representation of drawable */
        guac_layer layer;
        layer.index = drawable->index;

        /* Get parent layer */
        guac_layer parent_layer;
        if (drawable->pending.parent != NULL)
            parent_layer.index = drawable->pending.parent->index;
        else
            parent_layer.index = 0;

        /* Set position */
        guac_protocol_send_move(socket, &layer, &parent_layer,
                drawable->pending.rect.x, drawable->pending.rect.y,
                drawable->pending.z);

    }

}

void guac_drv_send_resize_drawable(guac_socket* socket,
        guac_drv_drawable* drawable) {

    /* Create layer representation of window */
    guac_layer layer;
    layer.index = drawable->index;

    guac_protocol_send_size(socket, &layer,
            drawable->pending.rect.width,
            drawable->pending.rect.height);

}

void guac_drv_send_copy(guac_socket* socket,
        guac_drv_drawable* src, int srcx, int srcy, int w, int h,
        guac_drv_drawable* dst, int dstx, int dsty) {

    /* Create layer representations of src/dst drawables */
    guac_layer src_layer, dst_layer;
    src_layer.index = src->index;
    dst_layer.index = dst->index;

    guac_protocol_send_copy(socket,
                            &src_layer, srcx, srcy, w, h,
            GUAC_COMP_OVER, &dst_layer, dstx, dsty);

}

void guac_drv_send_crect(guac_socket* socket,
        guac_drv_drawable* drawable, int x, int y, int w, int h,
        int r, int g, int b, int a) {

    /* Create layer representation of drawable */
    guac_layer layer;
    layer.index = drawable->index;

    /* Send rectangle */
    guac_protocol_send_rect(socket, &layer, x, y, w, h);
    guac_protocol_send_cfill(socket, GUAC_COMP_OVER, &layer, r, g, b, a);

}

void guac_drv_send_drect(guac_socket* socket,
        guac_drv_drawable* drawable, int x, int y, int w, int h,
        guac_drv_drawable* fill) {

    /* Create layer representations of drawables */
    guac_layer layer, fill_layer;
    layer.index = drawable->index;
    fill_layer.index = fill->index;

    /* Send rectangle */
    guac_protocol_send_rect(socket, &layer, x, y, w, h);
    guac_protocol_send_lfill(socket, GUAC_COMP_OVER, &layer, &fill_layer);

}

void guac_drv_client_end_frame(guac_client* client) {

    /* Send sync */
    guac_client_end_frame(client);

    /* Flush buffer */
    guac_socket_flush(client->socket);

}

void guac_drv_client_draw(guac_client* client,
        guac_drv_drawable* drawable, int x, int y, int w, int h) {

    unsigned char* data;
    cairo_surface_t* surface;
    guac_socket* socket = client->socket;

    /* Create layer representation of drawable */
    guac_layer layer;
    layer.index = drawable->index;

    /* Don't bother if image has no dimension */
    if (w == 0 || h == 0)
        return;

    /* Create temporary surface */
    data = drawable->image_data + (y*drawable->image_stride) + x*4;
    surface = cairo_image_surface_create_for_data(data, CAIRO_FORMAT_RGB24,
            w, h, drawable->image_stride);

    /* Send rectangle */
    guac_client_stream_png(client, socket, GUAC_COMP_OVER, &layer, x, y, surface);

    /* Done */
    cairo_surface_destroy(surface);

}
void guac_drv_user_draw(guac_user* user,
        guac_drv_drawable* drawable, int x, int y, int w, int h) {

    unsigned char* data;
    cairo_surface_t* surface;
    guac_socket* socket = user->socket;

    /* Create layer representation of drawable */
    guac_layer layer;
    layer.index = drawable->index;

    /* Don't bother if image has no dimension */
    if (w == 0 || h == 0)
        return;

    /* Create temporary surface */
    data = drawable->image_data + (y*drawable->image_stride) + x*4;
    surface = cairo_image_surface_create_for_data(data, CAIRO_FORMAT_RGB24,
            w, h, drawable->image_stride);

    /* Send rectangle */
    guac_user_stream_png(user, socket, GUAC_COMP_OVER, &layer, x, y, surface);

    /* Done */
    cairo_surface_destroy(surface);

}

