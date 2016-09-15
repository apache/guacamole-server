
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
#include "common/display.h"
#include "default_pointer.h"
#include "guac_display.h"
#include "guac_drv.h"
#include "guac_input.h"
#include "guac_user.h"
#include "io.h"

#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/user.h>

void guac_drv_display_sync_user(guac_drv_display* display, guac_user* user) {

    guac_client* client = user->client;
    guac_socket* socket = user->socket;

    /* Synchronize display */
    guac_common_display_dup(display->display, user, user->socket);

    /* Synchronize pointer */
    guac_drv_set_default_pointer(user);

    /* End initial frame */
    guac_protocol_send_sync(socket, client->last_sent_timestamp);
    guac_socket_flush(socket);

}

int guac_drv_user_join_handler(guac_user* user, int argc, char** argv) {

    guac_client* client = user->client;
    guac_drv_display* display = (guac_drv_display*) client->data;

    /* Init data */
    guac_drv_user_data* user_data;
    user_data = user->data = malloc(sizeof(guac_drv_user_data));
    user_data->button_mask = 0;

    /* Init user display state */
    guac_drv_display_sync_user(display, user);

    /* Set user event handlers */
    user->mouse_handler = guac_drv_user_mouse_handler;
    user->leave_handler = guac_drv_user_leave_handler;

    return 0;

}

int guac_drv_user_leave_handler(guac_user* user) {

    /* Free user-specific data */
    free(user->data);

    return 0;

}

int guac_drv_user_mouse_handler(guac_user* user,
        int x, int y, int mask) {

    guac_drv_user_data* user_data = (guac_drv_user_data*) user->data;

    /* If events can be written, send packet */
    if (GUAC_DRV_INPUT_WRITE_FD != -1) {

        /* Calculate button difference */
        int change = mask ^ user_data->button_mask;

        /* Build event packet */
        guac_drv_input_event event;
        event.type = GUAC_DRV_INPUT_EVENT_MOUSE;
        event.data.mouse.mask = mask;
        event.data.mouse.change_mask = change;
        event.data.mouse.x = x;
        event.data.mouse.y = y;

        /* Send packet */
        user_data->button_mask = mask;
        guac_drv_write(GUAC_DRV_INPUT_WRITE_FD, &event, sizeof(event));

    }

    return 0;

}

