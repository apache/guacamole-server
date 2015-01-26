/*
 * Copyright (C) 2015 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"
#include "scrollbar.h"

#include <guacamole/client.h>
#include <guacamole/layer.h>
#include <guacamole/socket.h>
#include <guacamole/protocol.h>

#include <stdlib.h>

guac_terminal_scrollbar* guac_terminal_scrollbar_alloc(guac_client* client,
        const guac_layer* parent, int parent_width, int parent_height, int visible_area) {

    /* Allocate scrollbar */
    guac_terminal_scrollbar* scrollbar =
        malloc(sizeof(guac_terminal_scrollbar));

    /* Associate client */
    scrollbar->client = client;

    /* Init default min/max and value */
    scrollbar->min   = 0;
    scrollbar->max   = 0;
    scrollbar->value = 0;

    /* Init parent data */
    scrollbar->parent        = parent;
    scrollbar->parent_width  = 0;
    scrollbar->parent_height = 0;
    scrollbar->visible_area  = 0;

    /* Allocate and init layers */
    scrollbar->container = guac_client_alloc_layer(client);
    scrollbar->box       = guac_client_alloc_layer(client);

    /* Reposition and resize to fit parent */
    guac_terminal_scrollbar_parent_resized(scrollbar,
            parent_width, parent_height, visible_area);

    return scrollbar;

}

void guac_terminal_scrollbar_free(guac_terminal_scrollbar* scrollbar) {

    /* Free layers */
    guac_client_free_layer(scrollbar->client, scrollbar->box);
    guac_client_free_layer(scrollbar->client, scrollbar->container);

    /* Free scrollbar */
    free(scrollbar);

}

/**
 * Updates the position and size of the scrollbar inner box relative to the
 * current scrollbar value, bounds, and parent layer dimensions.
 *
 * @param scrollbar
 *     The scrollbar whose inner box should be updated.
 */
static void __update_box(guac_terminal_scrollbar* scrollbar) {

    guac_socket* socket = scrollbar->client->socket;

    /* Skip if no scrolling region at all */
    if (scrollbar->min >= scrollbar->max)
        return;

    int region_size = scrollbar->max - scrollbar->min;

    /* Calculate box dimensions */
    int box_width  = GUAC_TERMINAL_SCROLLBAR_WIDTH - GUAC_TERMINAL_SCROLLBAR_PADDING*2;
    int box_height = GUAC_TERMINAL_SCROLLBAR_MIN_HEIGHT;

    /* Size box relative to visible area */
    int padded_container_height = (scrollbar->parent_height - GUAC_TERMINAL_SCROLLBAR_PADDING*2);
    int proportional_height = padded_container_height * scrollbar->visible_area / (region_size + scrollbar->visible_area);
    if (proportional_height > box_height)
        box_height = proportional_height;

    /* Calculate box position and dimensions */
    int box_x = GUAC_TERMINAL_SCROLLBAR_PADDING;
    int box_y = GUAC_TERMINAL_SCROLLBAR_PADDING
              + (padded_container_height - box_height) * (scrollbar->value - scrollbar->min) / region_size;

    /* Reposition box relative to container and current value */
    guac_protocol_send_move(socket,
            scrollbar->box, scrollbar->container,
            box_x, box_y, 0);

    /* Resize box relative to scrollable area */
    guac_protocol_send_size(socket, scrollbar->box,
            box_width, box_height);

    /* Fill box with solid color */
    guac_protocol_send_rect(socket, scrollbar->box, 0, 0, box_width, box_height);
    guac_protocol_send_cfill(socket, GUAC_COMP_SRC, scrollbar->box, 0xFF, 0xFF, 0xFF, 0x80);

}

void guac_terminal_scrollbar_set_bounds(guac_terminal_scrollbar* scrollbar,
        int min, int max) {

    /* Fit value within bounds */
    if (scrollbar->value > max)
        scrollbar->value = max;
    else if (scrollbar->value < min)
        scrollbar->value = min;

    /* Update bounds */
    scrollbar->min = min;
    scrollbar->max = max;

    /* Update box position and size */
    __update_box(scrollbar);

}

void guac_terminal_scrollbar_set_value(guac_terminal_scrollbar* scrollbar,
        int value) {

    /* Fit value within bounds */
    if (value > scrollbar->max)
        value = scrollbar->max;
    else if (value < scrollbar->min)
        value = scrollbar->min;

    /* Update value */
    scrollbar->value = value;

    /* Update box position and size */
    __update_box(scrollbar);

}

void guac_terminal_scrollbar_parent_resized(guac_terminal_scrollbar* scrollbar,
        int parent_width, int parent_height, int visible_area) {

    guac_socket* socket = scrollbar->client->socket;

    /* Calculate container position and dimensions */
    int container_x      = parent_width - GUAC_TERMINAL_SCROLLBAR_WIDTH;
    int container_y      = 0;
    int container_width  = GUAC_TERMINAL_SCROLLBAR_WIDTH;
    int container_height = parent_height;

    /* Reposition container relative to parent dimensions */
    guac_protocol_send_move(socket,
            scrollbar->container, scrollbar->parent,
            container_x, container_y, 0);

    /* Resize to fit within parent */
    guac_protocol_send_size(socket, scrollbar->container,
            container_width, container_height);

    /* Assign new dimensions */
    scrollbar->parent_width  = parent_width;
    scrollbar->parent_height = parent_height;
    scrollbar->visible_area  = visible_area;

    /* Update box position and size */
    __update_box(scrollbar);

}

