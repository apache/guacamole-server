
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

#include <guacamole/user.h>

#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <stdlib.h>

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

    /* Process events until signalled to stop */
    while (agent->thread_running) {

        /* TODO: guac_drv_agent_free() needs to cause an event to be fired */
        xcb_generic_event_t* event = xcb_wait_for_event(connection);
        if (event == NULL)
            break;

        /* Derive event type from response type */
        uint8_t event_type = event->response_type & 0x7F;

        /* STUB */
        guac_user_log(agent->user, GUAC_LOG_DEBUG, "Event: %i", event_type);

    }

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

