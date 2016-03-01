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
#include "guac_cursor.h"
#include "guac_display.h"
#include "rdp.h"
#include "rdp_pointer.h"

#include <cairo/cairo.h>
#include <freerdp/freerdp.h>
#include <guacamole/client.h>

#include <stdlib.h>

void guac_rdp_pointer_new(rdpContext* context, rdpPointer* pointer) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Allocate buffer */
    guac_common_display_layer* buffer = guac_common_display_alloc_buffer(
            rdp_client->display, pointer->width, pointer->height);

    /* Allocate data for image */
    unsigned char* data =
        (unsigned char*) malloc(pointer->width * pointer->height * 4);

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
    guac_common_surface_draw(buffer->surface, 0, 0, surface);

    /* Free surface */
    cairo_surface_destroy(surface);
    free(data);

    /* Remember buffer */
    ((guac_rdp_pointer*) pointer)->layer = buffer;

}

void guac_rdp_pointer_set(rdpContext* context, rdpPointer* pointer) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Set cursor */
    guac_common_cursor_set_surface(rdp_client->display->cursor,
            pointer->xPos, pointer->yPos,
            ((guac_rdp_pointer*) pointer)->layer->surface);

}

void guac_rdp_pointer_free(rdpContext* context, rdpPointer* pointer) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_common_display_layer* buffer = ((guac_rdp_pointer*) pointer)->layer;

    /* Free buffer */
    guac_common_display_free_buffer(rdp_client->display, buffer);

}

void guac_rdp_pointer_set_null(rdpContext* context) {
    /* STUB */
}

void guac_rdp_pointer_set_default(rdpContext* context) {
    /* STUB */
}

