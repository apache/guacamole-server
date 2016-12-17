
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

#ifndef __GUAC_DISPLAY_H
#define __GUAC_DISPLAY_H

#include "config.h"
#include "common/display.h"
#include "drawable.h"

#include <errno.h>
#include <pthread.h>

#include <xorg-server.h>
#include <xf86.h>
#include <xf86str.h>

#include <guacamole/client.h>
#include <guacamole/pool.h>

/**
 * The amount of time to wait for display changes before beginning a new frame,
 * in milliseconds. This value must be kept reasonably small such that a
 * infrequent updates will not prevent external events from being handled (such
 * as the stop signal from guac_client_stop()), but large enough that the
 * render loop does not eat up CPU spinning.
 */
#define GUAC_DRV_FRAME_START_TIMEOUT 1000

/**
 * Maximum frame duration, in milliseconds.
 */
#define GUAC_DRV_FRAME_MAX_DURATION 40

/**
 * Maximum amount of time to wait between render operations before considering
 * the frame complete, in milliseconds.
 */
#define GUAC_DRV_FRAME_TIMEOUT 0

/**
 * Private data for each screen, containing handlers for wrapped functions
 * and structures required for Guacamole protocol communication.
 */
typedef struct guac_drv_display {

    /**
     * The host or address that the instance of guacd built into the Guacamole
     * X.Org driver should listen on.
     */
    const char* listen_address;

    /**
     * The port that the instance of guacd built into the Guacamole X.Org
     * driver should listen on.
     */
    const char* listen_port;

    /**
     * The thread which listens for incoming Guacamole connections.
     */
    pthread_t listen_thread;

    /**
     * Watchdog thread which waits for drawing operations to stop for some
     * arbitrary timeout period, or for a maximum frame duration to be reached,
     * before automatically flushing buffers and sending syncs to connected
     * users.
     */
    pthread_t render_thread;

    /**
     * Flag set whenever an operation has affected the display in a way that
     * will require a frame flush. When this flag is set, the modified_cond
     * condition will be signalled. The modified_lock will always be
     * acquired before this flag is altered.
     */
    int modified;

    /**
     * Condition which is signalled when the modified flag has been set
     */
    pthread_cond_t modified_cond;

    /**
     * The mutex associated with the modified condition and flag, locked
     * whenever a thread is waiting on the modified condition, the modified
     * condition is being signalled, or the modified flag is being changed.
     */
    pthread_mutex_t modified_lock;

    /**
     * The guac_client representing the pseudo-connection to the local X11
     * display.
     */
    guac_client* client;

    /**
     * The internal display state which should be replicated across all
     * connected users.
     */
    guac_common_display* display;

    /**
     * The X.Org screen with which the Guacamole X.Org driver is associated.
     */
    ScreenPtr screen;

} guac_drv_display;

/**
 * Allocates a new multicast display.
 */
guac_drv_display* guac_drv_display_alloc(ScreenPtr screen,
        const char* address, const char* port);

/**
 * Immediately resizes the Guacamole display to the given width and height.
 * This operation is performed independently of the X.Org server, and will NOT
 * update X.Org resources.
 */
void guac_drv_display_resize(guac_drv_display* display, int w, int h);

/**
 * Creates a new layer, returning the new drawable representing that layer.
 */
guac_drv_drawable* guac_drv_display_create_layer(guac_drv_display* display,
        guac_drv_drawable* parent, int x, int y, int z, 
        int width, int height, int opacity);

/**
 * Destroys and frees the layer represented by the given drawable.
 */
void guac_drv_display_destroy_layer(guac_drv_display* display,
        guac_drv_drawable* drawable);

/**
 * Destroys the given drawable, but does not free any server-side resources.
 */
void guac_drv_display_destroy_drawable(guac_drv_drawable* drawable);

/**
 * Signal modification of the display.
 */
void guac_drv_display_touch(guac_drv_display* display);

/**
 * Ends the current frame, flushing pending display state to all users.
 */
void guac_drv_display_flush(guac_drv_display* display);

#endif

