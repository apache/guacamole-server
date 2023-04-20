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

#include "cef_render_handler.h"

#include <stdlib.h>
#include <stdio.h>

int on_paint(struct _cef_render_handler_t *self,
             struct _cef_browser_t *browser,
             cef_paint_element_type_t type,
             size_t dirtyRectsCount,
             const cef_rect_t *dirtyRects,
             const void *buffer,
             int width,
             int height) {

    /**
     * Process the buffer containing the updated screen data here.
     * Integrate with the Guacamole-specific protocol to send the render
     * data to the guacamole-client.
     */
    fprintf(stderr, "Got screen data\n");

    return 0;
}

int get_view_rect(struct _cef_browser_t *browser,
                  struct _cef_render_handler_t *self,
                  cef_rect_t *rect) {
    rect->x = 0;
    rect->y = 0;
    rect->width = 1280;  // Set this to the desired width of your viewport
    rect->height = 720;  // Set this to the desired height of your viewport

    return 1; // Returning 1 indicates the rect was provided successfully
}

custom_render_handler_t *create_render_handler() {
    custom_render_handler_t *handler = (custom_render_handler_t *)calloc(1, sizeof(custom_render_handler_t));

    /* Set the function pointers for the callbacks we are implementing */
    handler->get_view_rect = get_view_rect;
    handler->on_paint = on_paint;

    /* Initialize base structure */
    handler->base.base.size = sizeof(cef_render_handler_t);

    return handler;
}
