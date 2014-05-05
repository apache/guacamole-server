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

#include "config.h"

#include "cursor.h"

#include <stdlib.h>

#include <guacamole/client.h>
#include <guacamole/protocol.h>

guac_terminal_cursor* guac_terminal_cursor_alloc(guac_client* client) {

    /* Alloc new cursor, initialize buffer */
    guac_terminal_cursor* cursor = malloc(sizeof(guac_terminal_cursor));
    cursor->buffer = guac_client_alloc_buffer(client);

    return cursor;

}

void guac_terminal_cursor_free(guac_client* client, guac_terminal_cursor* cursor) {

    /* Free buffer */
    guac_client_free_buffer(client, cursor->buffer);

    /* Free cursor */
    free(cursor);

}

void guac_terminal_set_cursor(guac_client* client, guac_terminal_cursor* cursor) {

    /* Set cursor */
    guac_protocol_send_cursor(client->socket,
            cursor->hotspot_x, cursor->hotspot_y,
            cursor->buffer,
            0, 0, cursor->width, cursor->height);

}

