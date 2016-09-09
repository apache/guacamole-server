
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
#include "guac_rect.h"

void guac_drv_rect_clear(guac_drv_rect* rect) {
    guac_drv_rect_init(rect, 0, 0, 0, 0);
}

void guac_drv_rect_init(guac_drv_rect* rect, int x, int y, int w, int h) {
    rect->x      = x;
    rect->y      = y; 
    rect->width  = w;
    rect->height = h;
}

void guac_drv_rect_extend(guac_drv_rect* rect, const guac_drv_rect* op) {

    /* Extents of the rectangle being modified */
    int rect_x1 = rect->x;
    int rect_x2 = rect_x1 + rect->width;
    int rect_y1 = rect->y;
    int rect_y2 = rect_y1 + rect->height;

    /* Extents of operand */
    int op_x1 = op->x;
    int op_x2 = op_x1 + op->width;
    int op_y1 = op->y;
    int op_y2 = op_y1 + op->height;

    /* Update minimums */
    if (op_x1 < rect_x1) rect_x1 = op_x1;
    if (op_y1 < rect_y1) rect_y1 = op_y1;

    /* Update maximums */
    if (op_x2 > rect_x2) rect_x2 = op_x2;
    if (op_y2 > rect_y2) rect_y2 = op_y2;

    /* Update rect */
    rect->x      = rect_x1;
    rect->y      = rect_y1;
    rect->width  = rect_x2 - rect_x1;
    rect->height = rect_y2 - rect_y1;

}

void guac_drv_rect_shrink(guac_drv_rect* rect, const guac_drv_rect* op) {

    /* Extents of the rectangle being modified */
    int rect_x1 = rect->x;
    int rect_x2 = rect_x1 + rect->width;
    int rect_y1 = rect->y;
    int rect_y2 = rect_y1 + rect->height;

    /* Extents of operand */
    int op_x1 = op->x;
    int op_x2 = op_x1 + op->width;
    int op_y1 = op->y;
    int op_y2 = op_y1 + op->height;

    /* Contain left/top */
    if (op_x1 > rect_x1) rect_x1 = op_x1;
    if (op_y1 > rect_y1) rect_y1 = op_y1;

    /* Contain bottom/right */
    if (op_x2 < rect_x2) rect_x2 = op_x2;
    if (op_y2 < rect_y2) rect_y2 = op_y2;

    /* Update location */
    rect->x = rect_x1;
    rect->y = rect_y1;

    /* Update dimensions if positive */
    if (rect_x2 > rect_x1 && rect_y2 > rect_y1) {
        rect->width  = rect_x2 - rect_x1;
        rect->height = rect_y2 - rect_y1;
    }
    else {
        rect->width  = 0;
        rect->height = 0;
    }

}

