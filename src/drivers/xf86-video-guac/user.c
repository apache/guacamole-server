
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

#include <xcb/xcb.h>
#include <xcb/randr.h>

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

/**
 * Resizes the display to the given width and height, taking into account the
 * user's reported optimal DPI.
 *
 * @param user
 *     The user for whom the display is being resized.
 *
 * @param w
 *     The desired display width, in pixels.
 *
 * @param h
 *     The desired display height, in pixels.
 */
static void guac_drv_user_resize_display(guac_user* user, int w, int h) {

    /* Get X client connection */
    guac_drv_user_data* user_data = (guac_drv_user_data*) user->data;
    xcb_connection_t* connection = user_data->connection;

    /* Ignore if there is no X connection */
    if (connection == NULL)
        return;

    /* Get user's optimal DPI */
    int dpi = user->info.optimal_resolution;

    /* Calculate dimensions in millimeters */
    int width_mm = w * 254 / dpi / 10;
    int height_mm = h * 254 / dpi / 10;

    /* Scale width/height back to 96 DPI */
    w = w * 96 / dpi;
    h = h * 96 / dpi;

    /* Request screen resize */
    xcb_void_cookie_t randr_request = xcb_randr_set_screen_size_checked(
            connection, user_data->dummy, w, h, width_mm, height_mm);
    xcb_flush(connection);

    guac_user_log(user, GUAC_LOG_INFO, "Requested screen resize to %ix%i "
            "pixels (%ix%i mm).", w, h, width_mm, height_mm);

    /* Check for errors */
    xcb_generic_error_t* error = xcb_request_check(connection, randr_request);
    if (error != NULL)
        guac_user_log(user, GUAC_LOG_WARNING, "Screen resize request failed.");

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

    /* Connect to X server if authorization succeeds */
    if (display->auth != NULL) {

        /* Connect to X server as a client */
        xcb_connection_t* connection = guac_drv_get_connection(display->auth);

        /* Warn if X connection fails */
        if (connection == NULL)
            guac_user_log(user, GUAC_LOG_WARNING, "Unable to connect to X.Org "
                    "display as a client. Automatic screen resizing will NOT "
                    "work.");

        /* Otherwise init X client resources */
        else {

            /* Get screen */
            const xcb_setup_t* setup = xcb_get_setup(connection);
            xcb_screen_t* screen = xcb_setup_roots_iterator(setup).data;

            /* Create dummy window for future X requests */
            user_data->dummy = xcb_generate_id(connection);
            xcb_create_window(connection,  0, user_data->dummy, screen->root,
                    0, 0, 1, 1, 0, XCB_WINDOW_CLASS_COPY_FROM_PARENT,
                    XCB_COPY_FROM_PARENT, 0, NULL);

            /* Flush pending requests */
            xcb_flush(connection);

            /* Store successful connection */
            user_data->connection = connection;

        }

    }

    /* Resize screen based on declared optimal settings */
    guac_drv_user_resize_display(user, user->info.optimal_width,
            user->info.optimal_height);

    /* Init user display state */
    guac_drv_display_sync_user(display, user);

    return 0;

}

int guac_drv_user_leave_handler(guac_user* user) {

    guac_drv_user_data* user_data = (guac_drv_user_data*) user->data;

    /* Update shared cursor state */
    guac_common_cursor_remove_user(user_data->display->display->cursor, user);

    /* Disconnect from X.Org (if connected) */
    if (user_data->connection != NULL)
        xcb_disconnect(user_data->connection);

    /* Free user-specific data */
    free(user_data);

    return 0;

}

/**
 * Sends the given event along the file descriptor used by the Guacamole X.Org
 * input driver. If the X.Org server has not yet finished initializing, and the
 * file descriptor is not yet defined, this function has no effect.
 *
 * @param user
 *     The user associated with the event being sent.
 *
 * @param event
 *     The input event to send.
 */
static void guac_drv_user_send_event(guac_user* user,
        guac_drv_input_event* event) {

    /* Do not send packet if input file descriptor not yet ready */
    if (GUAC_DRV_INPUT_WRITE_FD == -1)
        return;

    /* Send packet */
    guac_drv_write(GUAC_DRV_INPUT_WRITE_FD, event,
            sizeof(guac_drv_input_event));

}

int guac_drv_user_size_handler(guac_user* user, int width, int height) {

    /* Resize display resize */
    guac_drv_user_resize_display(user, width, height);

    return 0;

}

int guac_drv_user_key_handler(guac_user* user, int keysym, int pressed) {

    /* Build keyboard event packet */
    guac_drv_input_event event;
    event.type = GUAC_DRV_INPUT_EVENT_KEYBOARD;
    event.data.keyboard.keysym = keysym;
    event.data.keyboard.pressed = pressed;

    /* Send packet */
    guac_drv_user_send_event(user, &event);

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
    guac_drv_user_send_event(user, &event);

    return 0;

}

