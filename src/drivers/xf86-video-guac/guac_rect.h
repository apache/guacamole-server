
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

#ifndef __GUAC_RECT_H
#define __GUAC_RECT_H

#include "config.h"

/**
 * An arbitrary rectangle.
 */
typedef struct guac_drv_rect {

    /**
     * X coordinate of the upper-left corner of the rectangle.
     */
    int x;

    /**
     * Y coordinate of the upper-left corner of the rectangle.
     */
    int y;

    /**
     * The width of the rectangle.
     */
    int width;

    /**
     * The height of the rectangle.
     */
    int height;

} guac_drv_rect;


/**
 * Resets all parameters of the rectangle to 0, a fairly common operation.
 */
void guac_drv_rect_clear(guac_drv_rect* rect);

/**
 * Initialize the given rectangle to the given dimensions.
 */
void guac_drv_rect_init(guac_drv_rect* rect, int x, int y, int w, int h);

/**
 * Extends the given rect such that it contains the other given rect.
 */
void guac_drv_rect_extend(guac_drv_rect* rect, const guac_drv_rect* op);

/**
 * Shrinks the given rect such that it is within the other given rect.
 */
void guac_drv_rect_shrink(guac_drv_rect* rect, const guac_drv_rect* op);

#endif

