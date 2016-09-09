
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

#ifndef __GUAC_MULTICAST_H
#define __GUAC_MULTICAST_H

#include "config.h"
#include "list.h"

#include <xf86.h>
#include <xf86str.h>

/**
 * Creates the given drawable on all clients.
 */
void guac_drv_multicast_create_drawable(guac_drv_list* clients,
        guac_drv_drawable* drawable);

/**
 * Alters the visibility of the given drawable on all clients.
 */
void guac_drv_multicast_shade_drawable(guac_drv_list* clients,
        guac_drv_drawable* drawable);

/**
 * Destroys the given drawable on all clients.
 */
void guac_drv_multicast_destroy_drawable(guac_drv_list* clients,
        guac_drv_drawable* drawable);

/**
 * Moves the given drawable on all clients.
 */
void guac_drv_multicast_move_drawable(guac_drv_list* clients,
        guac_drv_drawable* drawable);

/**
 * Resizes the given drawable on all clients.
 */
void guac_drv_multicast_resize_drawable(guac_drv_list* clients,
        guac_drv_drawable* drawable);

/**
 * Copies a rectangle of image data between the given drawables on the given
 * client.
 */
void guac_drv_multicast_copy(guac_drv_list* clients,
        guac_drv_drawable* src, int srcx, int srcy, int w, int h,
        guac_drv_drawable* dst, int dstx, int dsty);

/**
 * Sends the contents of the given rectangle of the given drawable to the given
 * client.
 */
void guac_drv_multicast_draw(guac_drv_list* clients,
        guac_drv_drawable* drawable, int x, int y, int w, int h);

/**
 * Sends the the given colored rectangle to all clients.
 */
void guac_drv_multicast_crect(guac_drv_list* clients,
        guac_drv_drawable* drawable, int x, int y, int w, int h,
        int r, int g, int b, int a);

/**
 * Sends the the given drawable-filled rectangle to all clients.
 */
void guac_drv_multicast_drect(guac_drv_list* clients,
        guac_drv_drawable* drawable, int x, int y, int w, int h,
        guac_drv_drawable* fill);

/**
 * Completes the current frame, flushing all buffers and sending syncs.
 */
void guac_drv_multicast_end_frame(guac_drv_list* clients);

#endif

