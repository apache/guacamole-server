
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

#ifndef __GUAC_DRV_CLIENT_H
#define __GUAC_DRV_CLIENT_H

#include "config.h"
#include "guac_drawable.h"
#include "list.h"

#include <guacamole/client.h>

/**
 * Guacamole client-specific data.
 */
typedef struct guac_drv_client_data {

    /**
     * Input thread handling incoming Guacamole messages.
     */
    pthread_t input_thread;

    /**
     * The old button mask state.
     */
    int button_mask;

    /**
     * The list which contains ALL clients.
     */
    guac_drv_list* clients;

    /**
     * The list element which contains this client.
     */
    guac_drv_list_element* self;

} guac_drv_client_data;

/**
 * Creates the given drawable on the given client.
 */
void guac_drv_client_create_drawable(guac_client* client,
        guac_drv_drawable* drawable);

/**
 * Alters the visibility of the given drawable on the given client.
 */
void guac_drv_client_shade_drawable(guac_client* client,
        guac_drv_drawable* drawable);

/**
 * Destroys the given drawable on the given client.
 */
void guac_drv_client_destroy_drawable(guac_client* client,
        guac_drv_drawable* drawable);

/**
 * Moves the given drawable on the given client.
 */
void guac_drv_client_move_drawable(guac_client* client,
        guac_drv_drawable* drawable);

/**
 * Resizes the given drawable on the given client.
 */
void guac_drv_client_resize_drawable(guac_client* client,
        guac_drv_drawable* drawable);

/**
 * Copies a rectangle of image data between the given drawables on the given
 * client.
 */
void guac_drv_client_copy(guac_client* client,
        guac_drv_drawable* src, int srcx, int srcy, int w, int h,
        guac_drv_drawable* dst, int dstx, int dsty);

/**
 * Sends the contents of the given rectangle of the given drawable to the given
 * client.
 */
void guac_drv_client_draw(guac_client* client,
        guac_drv_drawable* drawable, int x, int y, int w, int h);

/**
 * Sends the the given colored rectangle to the given client.
 */
void guac_drv_client_crect(guac_client* client,
        guac_drv_drawable* drawable, int x, int y, int w, int h,
        int r, int g, int b, int a);

/**
 * Sends the the given drawable-filled rectangle to the given client.
 */
void guac_drv_client_drect(guac_client* client,
        guac_drv_drawable* drawable, int x, int y, int w, int h,
        guac_drv_drawable* fill);

/**
 * Completes the current frame, flushing all buffers and sending syncs.
 */
void guac_drv_client_end_frame(guac_client* client);

/**
 * Thread which handles Guacamole instructions coming from the connected
 * client. The guac_client is passed as the argument.
 */
void* guac_drv_client_input_thread(void* arg);

/**
 * Handler for mouse events.
 */
int guac_drv_client_mouse_handler(guac_client* client, int x, int y, int mask);

/**
 * Handler for client unloading.
 */
int guac_drv_client_free_handler(guac_client* client);

/**
 * Send a debug message using the "log" instruction to the given client.
 */
void vguac_drv_client_debug(guac_client* client, const char* format,
        va_list args);

#endif

