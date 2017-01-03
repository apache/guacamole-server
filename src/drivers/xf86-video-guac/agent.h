
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

#ifndef GUAC_DRV_AGENT_H
#define GUAC_DRV_AGENT_H

#include <xcb/xcb.h>

#include <guacamole/user.h>

#include <pthread.h>

/**
 * The X client which acts as an agent on behalf of a particular connected
 * user, sending requests which would otherwise not be possible within scope of
 * a display driver.
 */
typedef struct guac_drv_agent {

    /**
     * The connected Guacamole user for whom this agent was created.
     */
    guac_user* user;

    /**
     * Client connection to the X server.
     */
    xcb_connection_t* connection;

    /**
     * Dummy window to associate with X client requests.
     */
    xcb_window_t dummy;

    /**
     * Flag indicating whether the event loop thread should continue running.
     * When the event loop thread needs to die, this is set to 0.
     */
    int thread_running;

    /**
     * The X client's event loop thread.
     */
    pthread_t thread;

} guac_drv_agent;

/**
 * Creates a new agent X client connected to the current display.
 *
 * @param user
 *     The connected Guacamole user for whom the agent is being created.
 *
 * @param auth
 *     The X authorization to use to connect to current display.
 *
 * @return
 *     A new agent X client which can be used to issue requests, or NULL if
 *     the agent X client could not be connected.
 */
guac_drv_agent* guac_drv_agent_alloc(guac_user* user, xcb_auth_info_t* auth);

/**
 * Disconnects and frees the given agent X client.
 *
 * @param agent
 *     The agent x client to free.
 */
void guac_drv_agent_free(guac_drv_agent* agent);

/**
 * Uses the agent X client to signal the display to resize to the given width
 * and height. The request is made on behalf of the agent's associated
 * Guacamole user.
 *
 * @param agent
 *     The agent X client to use to signal the display to resize. This agent
 *     MUST be the agent associated with the user making the resize request.
 *
 * @param w
 *     The desired display width, in pixels.
 *
 * @param h
 *     The desired display height, in pixels.
 */
int guac_drv_agent_resize_display(guac_drv_agent* agent, int w, int h);

#endif

