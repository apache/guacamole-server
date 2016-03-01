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

#include <cairo/cairo.h>
#include <guacamole/client.h>
#include <guacamole/layer.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/user.h>

/* Macros for prettying up the embedded image. */
#define X 0x00,0x00,0x00,0xFF
#define U 0x80,0x80,0x80,0xFF
#define O 0xFF,0xFF,0xFF,0xFF
#define _ 0x00,0x00,0x00,0x00

/* Dimensions */
const int guac_common_ibar_cursor_width  = 7;
const int guac_common_ibar_cursor_height = 16;

/* Format */
const cairo_format_t guac_common_ibar_cursor_format = CAIRO_FORMAT_ARGB32;
const int guac_common_ibar_cursor_stride = 28;

/* Embedded I-bar graphic */
unsigned char guac_common_ibar_cursor[] = {

        X,X,X,X,X,X,X,
        X,O,O,U,O,O,X,
        X,X,X,O,X,X,X,
        _,_,X,O,X,_,_,
        _,_,X,O,X,_,_,
        _,_,X,O,X,_,_,
        _,_,X,O,X,_,_,
        _,_,X,O,X,_,_,
        _,_,X,O,X,_,_,
        _,_,X,O,X,_,_,
        _,_,X,O,X,_,_,
        _,_,X,O,X,_,_,
        _,_,X,O,X,_,_,
        X,X,X,O,X,X,X,
        X,O,O,U,O,O,X,
        X,X,X,X,X,X,X

};

void guac_common_set_ibar_cursor(guac_user* user) {

    guac_client* client = user->client;
    guac_socket* socket = user->socket;

    /* Draw to buffer */
    guac_layer* cursor = guac_client_alloc_buffer(client);

    cairo_surface_t* graphic = cairo_image_surface_create_for_data(
            guac_common_ibar_cursor,
            guac_common_ibar_cursor_format,
            guac_common_ibar_cursor_width,
            guac_common_ibar_cursor_height,
            guac_common_ibar_cursor_stride);

    guac_user_stream_png(user, socket, GUAC_COMP_SRC, cursor,
            0, 0, graphic);
    cairo_surface_destroy(graphic);

    /* Set cursor */
    guac_protocol_send_cursor(socket, 0, 0, cursor,
            guac_common_ibar_cursor_width / 2,
            guac_common_ibar_cursor_height / 2,
            guac_common_ibar_cursor_width,
            guac_common_ibar_cursor_height);

    /* Free buffer */
    guac_client_free_buffer(client, cursor);

    guac_client_log(client, GUAC_LOG_DEBUG,
            "Client cursor image set to generic built-in I-bar.");

}

