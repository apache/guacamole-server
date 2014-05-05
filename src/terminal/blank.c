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

#include <cairo/cairo.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>

guac_terminal_cursor* guac_terminal_create_blank(guac_client* client) {

    guac_socket* socket = client->socket;
    guac_terminal_cursor* cursor = guac_terminal_cursor_alloc(client);

    /* Set buffer to a single 1x1 transparent rectangle */
    guac_protocol_send_rect(socket, cursor->buffer, 0, 0, 1, 1);
    guac_protocol_send_cfill(socket, GUAC_COMP_SRC, cursor->buffer,
            0x00, 0x00, 0x00, 0x00);

    /* Initialize cursor properties */
    cursor->width  = 1;
    cursor->height = 1;
    cursor->hotspot_x = 0;
    cursor->hotspot_y = 0;

    return cursor;

}

