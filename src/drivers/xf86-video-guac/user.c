
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
#include "common/cursor.h"
#include "common/display.h"
#include "display.h"
#include "drv.h"
#include "input.h"
#include "io.h"
#include "user.h"
#include "xclient.h"

#include <xf86.h>
#include <xf86str.h>

#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/user.h>

void guac_drv_display_sync_user(guac_drv_display* display, guac_user* user) {

    guac_client* client = user->client;
    guac_socket* socket = user->socket;

    /* Synchronize display */
    guac_common_display_dup(display->display, user, user->socket);

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
    user_data->display = display;
    user_data->button_mask = 0;

    /* Set user event handlers */
    user->size_handler  = guac_drv_user_size_handler;
    user->key_handler   = guac_drv_user_key_handler;
    user->mouse_handler = guac_drv_user_mouse_handler;
    user->leave_handler = guac_drv_user_leave_handler;

    /* Connect agent X client if authorization is available */
    if (display->auth != NULL)
        user_data->agent = guac_drv_agent_alloc(user, display->auth);

    /* Resize screen based on declared optimal settings */
    if (user_data->agent != NULL)
        guac_drv_agent_resize_display(user_data->agent,
                user->info.optimal_width,
                user->info.optimal_height);

    /* Init user display state */
    guac_drv_display_sync_user(display, user);

    return 0;

}

int guac_drv_user_leave_handler(guac_user* user) {

    guac_drv_user_data* user_data = (guac_drv_user_data*) user->data;

    /* Free connected agent */
    if (user_data->agent != NULL)
        guac_drv_agent_free(user_data->agent);

    /* Update shared cursor state */
    guac_common_cursor_remove_user(user_data->display->display->cursor, user);

    /* Free user-specific data */
    free(user_data);

    return 0;

}

int guac_drv_user_size_handler(guac_user* user, int width, int height) {

    guac_drv_user_data* user_data = (guac_drv_user_data*) user->data;

    /* Resize display resize */
    if (user_data->agent != NULL)
        guac_drv_agent_resize_display(user_data->agent, width, height);

    return 0;

}

int guac_drv_user_key_handler(guac_user* user, int keysym, int pressed) {

    /* Build keyboard event packet */
    guac_drv_input_event event;
    event.type = GUAC_DRV_INPUT_EVENT_KEYBOARD;
    event.data.keyboard.keysym = keysym;
    event.data.keyboard.pressed = pressed;

    /* Send packet */
    guac_drv_input_send_event(&event);

    return 0;

}

int guac_drv_user_mouse_handler(guac_user* user,
        int x, int y, int mask) {

    guac_drv_user_data* user_data = (guac_drv_user_data*) user->data;

    /* Store current mouse location */
    guac_common_cursor_move(user_data->display->display->cursor, user, x, y);

    /* Calculate button difference */
    int change = mask ^ user_data->button_mask;

    /* Build mouse event packet */
    guac_drv_input_event event;
    event.type = GUAC_DRV_INPUT_EVENT_MOUSE;
    event.data.mouse.mask = mask;
    event.data.mouse.change_mask = change;
    event.data.mouse.x = x;
    event.data.mouse.y = y;

    /* Send packet */
    user_data->button_mask = mask;
    guac_drv_input_send_event(&event);

    return 0;

}

