/*
 * Copyright (C) 2013 Glyptodon LLC
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


#ifndef _GUAC_TERMINAL_CURSOR_H
#define _GUAC_TERMINAL_CURSOR_H

#include "config.h"

#include <guacamole/client.h>
#include <guacamole/layer.h>

typedef struct guac_terminal_cursor {

    /**
     * A buffer allocated with guac_client_alloc_buffer() that contains the
     * cursor image.
     */
    guac_layer* buffer;

    /**
     * The width of the cursor in pixels.
     */
    int width;

    /**
     * The height of the cursor in pixels.
     */
    int height;

    /**
     * The X coordinate of the cursor hotspot.
     */
    int hotspot_x;

    /**
     * The Y coordinate of the cursor hotspot.
     */
    int hotspot_y;

} guac_terminal_cursor;

/**
 * Allocates a new cursor, pre-populating the cursor with a newly-allocated
 * buffer.
 */
guac_terminal_cursor* guac_terminal_cursor_alloc(guac_client* client);

/**
 * Frees the buffer associated with this cursor as well as the cursor itself.
 */
void guac_terminal_cursor_free(guac_client* client, guac_terminal_cursor* cursor);

/**
 * Set the remote cursor.
 */
void guac_terminal_set_cursor(guac_client* client, guac_terminal_cursor* cursor);

#endif
