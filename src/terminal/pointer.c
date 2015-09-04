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

#include "cursor.h"

#include <cairo/cairo.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>

/* Macros for prettying up the embedded image. */
#define X 0x00,0x00,0x00,0xFF
#define U 0x80,0x80,0x80,0xFF
#define O 0xFF,0xFF,0xFF,0xFF
#define _ 0x00,0x00,0x00,0x00

/* Dimensions */
const int guac_terminal_pointer_width  = 11;
const int guac_terminal_pointer_height = 16;

/* Format */
const cairo_format_t guac_terminal_pointer_format = CAIRO_FORMAT_ARGB32;
const int guac_terminal_pointer_stride = 44;

/* Embedded pointer graphic */
unsigned char guac_terminal_pointer[] = {

        O,_,_,_,_,_,_,_,_,_,_,
        O,O,_,_,_,_,_,_,_,_,_,
        O,X,O,_,_,_,_,_,_,_,_,
        O,X,X,O,_,_,_,_,_,_,_,
        O,X,X,X,O,_,_,_,_,_,_,
        O,X,X,X,X,O,_,_,_,_,_,
        O,X,X,X,X,X,O,_,_,_,_,
        O,X,X,X,X,X,X,O,_,_,_,
        O,X,X,X,X,X,X,X,O,_,_,
        O,X,X,X,X,X,X,X,X,O,_,
        O,X,X,X,X,X,O,O,O,O,O,
        O,X,X,O,X,X,O,_,_,_,_,
        O,X,O,_,O,X,X,O,_,_,_,
        O,O,_,_,O,X,X,O,_,_,_,
        O,_,_,_,_,O,X,X,O,_,_,
        _,_,_,_,_,O,O,O,O,_,_

};

guac_terminal_cursor* guac_terminal_create_pointer(guac_client* client) {

    guac_socket* socket = client->socket;
    guac_terminal_cursor* cursor = guac_terminal_cursor_alloc(client);

    /* Draw to buffer */
    cairo_surface_t* graphic = cairo_image_surface_create_for_data(
            guac_terminal_pointer,
            guac_terminal_pointer_format,
            guac_terminal_pointer_width,
            guac_terminal_pointer_height,
            guac_terminal_pointer_stride);

    guac_client_stream_png(client, socket, GUAC_COMP_SRC, cursor->buffer,
            0, 0, graphic);
    cairo_surface_destroy(graphic);

    /* Initialize cursor properties */
    cursor->width = guac_terminal_pointer_width;
    cursor->height = guac_terminal_pointer_height;
    cursor->hotspot_x = 0;
    cursor->hotspot_y = 0;

    return cursor;

}

