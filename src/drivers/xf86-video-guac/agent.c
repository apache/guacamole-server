
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
#include "agent.h"
#include "xclient.h"

#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/stream.h>
#include <guacamole/user.h>

#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xfixes.h>
#include <stdlib.h>

/**
 * Sends the contents of a window property to the given user over the Guacamole
 * connection as a text clipboard stream.
 *
 * @param user
 *     The user to send the window property to.
 *
 * @param connection
 *     The X connection associated with the window from which the property is
 *     being read.
 *
 * @param window
 *     The window from which the property is being read.
 *
 * @param property
 *     The property being read.
 *
 * @param type
 *     The type of the property being read.
 */
static void guac_drv_send_property_value_as_clipboard(guac_user* user,
        xcb_connection_t* connection, xcb_window_t window,
        xcb_atom_t property, xcb_atom_t type) {

    xcb_generic_error_t* error;

    /* Request property contents */
    xcb_get_property_cookie_t property_cookie = xcb_get_property(connection, 1,
            window, property, type, 0, 1024 /* = 4096 bytes */);

    /* Wait for response */
    xcb_get_property_reply_t* property_reply = xcb_get_property_reply(
            connection, property_cookie, &error);

    /* Bail out if request fails */
    if (error != NULL || property_reply->format != 8)
        return;

    /* Begin clipboard stream */
    guac_stream* stream = guac_user_alloc_stream(user);
    guac_protocol_send_clipboard(user->socket, stream, "text/plain");

    /* STUB: Need to repeat request if not all data is available */
    guac_protocol_send_blob(user->socket, stream,
            xcb_get_property_value(property_reply),
            xcb_get_property_value_length(property_reply));

    /* Clipboard stream ended */
    guac_protocol_send_end(user->socket, stream);
    guac_user_free_stream(user, stream);

    guac_socket_flush(user->socket);

}

/**
 * The event loop thread of the agent X client. This thread listens for X
 * events, such as changes to the clipboard, translating what it receives to
 * Guacamole protocol.
 *
 * @param data
 *     The guac_drv_agent instance controlling this thread.
 *
 * @return
 *     Always NULL.
 */
static void* guac_drv_agent_thread(void* data) {

    guac_drv_agent* agent = (guac_drv_agent*) data;
    xcb_connection_t* connection = agent->connection;

    /* Determine value of UTF8_STRING atom */
    xcb_atom_t utf8_string = guac_drv_get_atom(connection, "UTF8_STRING");
    if (utf8_string == XCB_ATOM_NONE) {
        guac_user_log(agent->user, GUAC_LOG_WARNING, "X server does not "
                "support the UTF8_STRING atom. Clipboard will not work.");
        return NULL;
    }

    /* Determine value of XSEL_DATA atom */
    xcb_atom_t xsel_data = guac_drv_get_atom(connection, "XSEL_DATA");
    if (xsel_data == XCB_ATOM_NONE) {
        guac_user_log(agent->user, GUAC_LOG_WARNING, "X server does not "
                "support the XSEL_DATA atom. Clipboard will not work.");
        return NULL;
    }

    /* Init XFixes extension */
    const xcb_query_extension_reply_t* xfixes =
        guac_drv_init_xfixes(connection);

    /* Agent thread is useless if XFixes is absent */
    if (xfixes == NULL) {
        guac_user_log(agent->user, GUAC_LOG_WARNING, "X server does not "
                "have the XFixes extension. Clipboard will not work.");
        return NULL;
    }

    /* Request XFixes to inform us of selection changes */
    xcb_xfixes_select_selection_input_checked(connection, agent->dummy,
            XCB_ATOM_PRIMARY,
            XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_CLIENT_CLOSE
            | XCB_XFIXES_SELECTION_EVENT_MASK_SELECTION_WINDOW_DESTROY
            | XCB_XFIXES_SELECTION_EVENT_MASK_SET_SELECTION_OWNER);

    /* Process events until signalled to stop */
    while (agent->thread_running) {

        /* TODO: guac_drv_agent_free() needs to cause an event to be fired */
        xcb_generic_event_t* event = xcb_wait_for_event(connection);
        if (event == NULL)
            break;

        /* Derive event type from response type */
        uint8_t event_type = event->response_type & 0x7F;

        /* If notified of a selection change, request conversion to UTF8 */
        if (event_type == xfixes->first_event + XCB_XFIXES_SELECTION_NOTIFY) {
            xcb_convert_selection(connection, agent->dummy, XCB_ATOM_PRIMARY,
                    utf8_string, xsel_data, XCB_CURRENT_TIME);
            xcb_flush(connection);
        }

        /* If we've received the converted UTF8 data, resend as clipboard */
        else if (event_type == XCB_SELECTION_NOTIFY) {

            xcb_selection_notify_event_t* selection_notify =
                (xcb_selection_notify_event_t*) event;

            guac_drv_send_property_value_as_clipboard(agent->user, connection,
                    selection_notify->requestor, selection_notify->property,
                    utf8_string);

        }

    } /* end event loop */

    guac_user_log(agent->user, GUAC_LOG_INFO, "End of agent thread.");

    return NULL;

}

guac_drv_agent* guac_drv_agent_alloc(guac_user* user, xcb_auth_info_t* auth) {

    /* Connect to X server as a client */
    xcb_connection_t* connection = guac_drv_get_connection(auth);
    if (connection == NULL)
        return NULL;

    guac_drv_agent* agent = malloc(sizeof(guac_drv_agent));
    agent->user = user;

    /* Get screen */
    const xcb_setup_t* setup = xcb_get_setup(connection);
    xcb_screen_t* screen = xcb_setup_roots_iterator(setup).data;

    /* New windows need to listen for property change events */
    uint32_t values[1] = { XCB_EVENT_MASK_PROPERTY_CHANGE };

    /* Create dummy window for future X requests */
    agent->dummy = xcb_generate_id(connection);
    xcb_create_window(connection,  0, agent->dummy, screen->root,
            0, 0, 1, 1, 0, XCB_WINDOW_CLASS_COPY_FROM_PARENT,
            XCB_COPY_FROM_PARENT, XCB_CW_EVENT_MASK, values);

    /* Flush pending requests */
    xcb_flush(connection);

    /* Store successful connection */
    agent->connection = connection;

    /* Start thread */
    if (!pthread_create(&agent->thread, NULL, guac_drv_agent_thread, agent))
        agent->thread_running = 1;

    /* Do not mark thread as running if it could not start */
    else {
        guac_user_log(user, GUAC_LOG_WARNING, "Unable to start agent thread. "
                "Clipboard access will not work.");
        agent->thread_running = 0;
    }

    /* Agent created */
    return agent;

}

void guac_drv_agent_free(guac_drv_agent* agent) {

    /* Wait for agent thread, if running */
    if (agent->thread_running) {
        agent->thread_running = 0;
        pthread_join(agent->thread, NULL);
    }

    /* Disconnect and free */
    xcb_disconnect(agent->connection);
    free(agent);

}

int guac_drv_agent_resize_display(guac_drv_agent* agent, int w, int h) {

    /* Get user and X client connection */
    guac_user* user = agent->user;
    xcb_connection_t* connection = agent->connection;

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
            connection, agent->dummy, w, h, width_mm, height_mm);
    xcb_flush(connection);

    guac_user_log(user, GUAC_LOG_INFO, "Requested screen resize to %ix%i "
            "pixels (%ix%i mm).", w, h, width_mm, height_mm);

    /* Check for errors */
    xcb_generic_error_t* error = xcb_request_check(connection, randr_request);
    if (error != NULL)
        return 1;

    /* Resize succeeded */
    return 0;

}

