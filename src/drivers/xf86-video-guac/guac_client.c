
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
#include "guac_client.h"
#include "guac_drawable.h"
#include "guac_drv.h"
#include "guac_input.h"
#include "io.h"
#include "log.h"

#include <guacamole/error.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/timestamp.h>

void guac_drv_client_create_drawable(guac_client* client,
        guac_drv_drawable* drawable) {

    /* Initialize drawable */
    guac_drv_client_move_drawable(client, drawable);
    guac_drv_client_shade_drawable(client, drawable);
    guac_drv_client_resize_drawable(client, drawable);

}

void guac_drv_client_shade_drawable(guac_client* client,
        guac_drv_drawable* drawable) {

    /* Only applies to non-default layers */
    if (drawable->index > 0) {

        guac_socket* socket = client->socket;

        /* Create layer representation of drawable */
        guac_layer layer;
        layer.index = drawable->index;

        guac_protocol_send_shade(socket, &layer, drawable->pending.opacity);

    }

}

void guac_drv_client_destroy_drawable(guac_client* client,
        guac_drv_drawable* drawable) {

    guac_socket* socket = client->socket;

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

void guac_drv_client_move_drawable(guac_client* client,
        guac_drv_drawable* drawable) {

    /* Only applies to non-default layers */
    if (drawable->index > 0) {

        guac_socket* socket = client->socket;

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

void guac_drv_client_resize_drawable(guac_client* client,
        guac_drv_drawable* drawable) {

    guac_socket* socket = client->socket;

    /* Create layer representation of window */
    guac_layer layer;
    layer.index = drawable->index;

    guac_protocol_send_size(socket, &layer,
            drawable->pending.rect.width,
            drawable->pending.rect.height);

}

void guac_drv_client_copy(guac_client* client,
        guac_drv_drawable* src, int srcx, int srcy, int w, int h,
        guac_drv_drawable* dst, int dstx, int dsty) {

    guac_socket* socket = client->socket;

    /* Create layer representations of src/dst drawables */
    guac_layer src_layer, dst_layer;
    src_layer.index = src->index;
    dst_layer.index = dst->index;

    guac_protocol_send_copy(socket,
                            &src_layer, srcx, srcy, w, h,
            GUAC_COMP_OVER, &dst_layer, dstx, dsty);

}

void guac_drv_client_crect(guac_client* client,
        guac_drv_drawable* drawable, int x, int y, int w, int h,
        int r, int g, int b, int a) {

    guac_socket* socket = client->socket;

    /* Create layer representation of drawable */
    guac_layer layer;
    layer.index = drawable->index;

    /* Send rectangle */
    guac_protocol_send_rect(socket, &layer, x, y, w, h);
    guac_protocol_send_cfill(socket, GUAC_COMP_OVER, &layer, r, g, b, a);

}

void guac_drv_client_drect(guac_client* client,
        guac_drv_drawable* drawable, int x, int y, int w, int h,
        guac_drv_drawable* fill) {

    guac_socket* socket = client->socket;

    /* Create layer representations of drawables */
    guac_layer layer, fill_layer;
    layer.index = drawable->index;
    fill_layer.index = fill->index;

    /* Send rectangle */
    guac_protocol_send_rect(socket, &layer, x, y, w, h);
    guac_protocol_send_lfill(socket, GUAC_COMP_OVER, &layer, &fill_layer);

}

void guac_drv_client_end_frame(guac_client* client) {

    guac_socket* socket = client->socket;
    guac_timestamp current = guac_timestamp_current();

    /* Send sync */
    guac_protocol_send_sync(socket, current);
    client->last_sent_timestamp = current;

    /* Flush buffer */
    guac_socket_flush(socket);

}

void* guac_drv_client_input_thread(void* arg) {

    guac_client* client = (guac_client*) arg;
    guac_socket* socket = client->socket;

    /* Guacamole client input loop */
    while (client->state == GUAC_CLIENT_RUNNING) {

        /* Read instruction */
        guac_instruction* instruction =
            guac_instruction_read(socket, GUAC_DRV_USEC_TIMEOUT);

        /* Stop on error */
        if (instruction == NULL) {
            guac_drv_client_log_guac_error(client, GUAC_LOG_ERROR,
                    "Error reading instruction");
            guac_client_stop(client);
            break;
        }

        /* Reset guac_error and guac_error_message (client handlers are not
         * guaranteed to set these) */
        guac_error = GUAC_STATUS_SUCCESS;
        guac_error_message = NULL;

        /* Call handler, stop on error */
        if (guac_client_handle_instruction(client, instruction) < 0) {

            /* Log error */
            guac_drv_client_log_guac_error(client, GUAC_LOG_ERROR,
                    "Client instruction handler error");

            /* Log handler details */
            guac_client_log(client, GUAC_LOG_INFO,
                    "Failing instruction handler in client was \"%s\"",
                    instruction->opcode);

            guac_instruction_free(instruction);
            guac_client_stop(client);
            break;
        }

        /* Free allocated instruction */
        guac_instruction_free(instruction);

    }

    guac_client_free(client);
    return NULL;

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
    guac_protocol_send_png(socket, GUAC_COMP_OVER, &layer, x, y, surface);

    /* Done */
    cairo_surface_destroy(surface);

}

int guac_drv_client_mouse_handler(guac_client* client,
        int x, int y, int mask) {

    guac_drv_client_data* client_data = (guac_drv_client_data*) client->data;

    /* If events can be written, send packet */
    if (GUAC_DRV_INPUT_WRITE_FD != -1) {

        /* Calculate button difference */
        int change = mask ^ client_data->button_mask;

        /* Build event packet */
        guac_drv_input_event event;
        event.type = GUAC_DRV_INPUT_EVENT_MOUSE;
        event.data.mouse.mask = mask;
        event.data.mouse.change_mask = change;
        event.data.mouse.x = x;
        event.data.mouse.y = y;

        /* Send packet */
        client_data->button_mask = mask;
        guac_drv_write(GUAC_DRV_INPUT_WRITE_FD, &event, sizeof(event));

    }

    return 0;

}

int guac_drv_client_free_handler(guac_client* client) {

    /* Get client data */
    guac_drv_client_data* client_data = (guac_drv_client_data*) client->data;

    /* Remove client from list */
    guac_drv_list_lock(client_data->clients);
    guac_drv_list_remove(client_data->clients, client_data->self);
    guac_drv_list_unlock(client_data->clients);

    return 0;

}

void vguac_drv_client_debug(guac_client* client, const char* format,
        va_list args) {

    guac_socket* socket = client->socket;
    vguac_protocol_send_log(socket, format, args);

}


