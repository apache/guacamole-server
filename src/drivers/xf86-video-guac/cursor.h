
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

#ifndef __GUAC_CURSOR_H
#define __GUAC_CURSOR_H

#include "config.h"

#include <xorg-server.h>
#include <xf86.h>

/**
 * The maximum width of a cursor supported by this driver, in pixels.
 */
#define GUAC_DRV_CURSOR_MAX_WIDTH 64

/**
 * The maximum height of a cursor supported by this driver, in pixels.
 */
#define GUAC_DRV_CURSOR_MAX_HEIGHT 64

/**
 * The number of bytes in each row of ARGB image data stored within a
 * guac_drv_cursor.
 */
#define GUAC_DRV_CURSOR_STRIDE (GUAC_DRV_CURSOR_MAX_HEIGHT * 4)

/**
 * A single ARGB mouse cursor and corresponding metadata.
 */
typedef struct guac_drv_cursor {

    /**
     * The raw ARGB image data of this cursor. Each row of image data is
     * GUAC_DRV_CURSOR_STRIDE bytes long, made up of 32-bit ARGB pixels. All
     * pixels are set to transparent black by default.
     */
    uint32_t image[GUAC_DRV_CURSOR_MAX_WIDTH * GUAC_DRV_CURSOR_MAX_HEIGHT];

    /**
     * The X coordinate of the mouse cursor's hotspot.
     */
    int hotspot_x;

    /**
     * The Y coordinate of the mouse cursor's hotspot.
     */
    int hotspot_y;

    /**
     * The width of the mouse cursor, in pixels.
     */
    int width;

    /**
     * The height of the mouse cursor, in pixels.
     */
    int height;

} guac_drv_cursor;

/**
 * Initialize hardware cursor rendering.
 */
Bool guac_drv_init_cursor(ScreenPtr screen);

#endif

