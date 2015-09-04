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

#include "client.h"
#include "rdp_pointer.h"

#include <cairo/cairo.h>
#include <freerdp/freerdp.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>

#include <stdlib.h>

void guac_rdp_pointer_new(rdpContext* context, rdpPointer* pointer) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_socket* socket = client->socket;

    /* Allocate data for image */
    unsigned char* data =
        (unsigned char*) malloc(pointer->width * pointer->height * 4);

    /* Allocate layer */
    guac_layer* buffer = guac_client_alloc_buffer(client);

    cairo_surface_t* surface;

    /* Convert to alpha cursor if mask data present */
    if (pointer->andMaskData && pointer->xorMaskData)
        freerdp_alpha_cursor_convert(data,
                pointer->xorMaskData, pointer->andMaskData,
                pointer->width, pointer->height, pointer->xorBpp,
                ((rdp_freerdp_context*) context)->clrconv);

    /* Create surface from image data */
    surface = cairo_image_surface_create_for_data(
        data, CAIRO_FORMAT_ARGB32,
        pointer->width, pointer->height, 4*pointer->width);

    /* Send surface to buffer */
    guac_client_stream_png(client, socket, GUAC_COMP_SRC, buffer,
            0, 0, surface);

    /* Free surface */
    cairo_surface_destroy(surface);
    free(data);

    /* Remember buffer */
    ((guac_rdp_pointer*) pointer)->layer = buffer;

}

void guac_rdp_pointer_set(rdpContext* context, rdpPointer* pointer) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_socket* socket = client->socket;

    /* Set cursor */
    guac_protocol_send_cursor(socket, pointer->xPos, pointer->yPos,
            ((guac_rdp_pointer*) pointer)->layer,
            0, 0, pointer->width, pointer->height);

}

void guac_rdp_pointer_free(rdpContext* context, rdpPointer* pointer) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_client_free_buffer(client, ((guac_rdp_pointer*) pointer)->layer);

}

void guac_rdp_pointer_set_null(rdpContext* context) {
    /* STUB */
}

void guac_rdp_pointer_set_default(rdpContext* context) {
    /* STUB */
}

