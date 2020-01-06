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

#include "color.h"
#include "common/cursor.h"
#include "common/display.h"
#include "common/surface.h"
#include "pointer.h"
#include "rdp.h"

#include <cairo/cairo.h>
#include <freerdp/codec/color.h>
#include <freerdp/freerdp.h>
#include <freerdp/gdi/gdi.h>
#include <guacamole/client.h>
#include <winpr/crt.h>

BOOL guac_rdp_pointer_new(rdpContext* context, rdpPointer* pointer) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Allocate buffer */
    guac_common_display_layer* buffer = guac_common_display_alloc_buffer(
            rdp_client->display, pointer->width, pointer->height);

    /* Allocate data for image */
    unsigned char* data = _aligned_malloc(pointer->width * pointer->height * 4, 16);

    cairo_surface_t* surface;

    /* Convert to alpha cursor if mask data present */
    if (pointer->andMaskData && pointer->xorMaskData)
        freerdp_image_copy_from_pointer_data(data,
                guac_rdp_get_native_pixel_format(TRUE), 0, 0, 0,
                pointer->width, pointer->height, pointer->xorMaskData,
                pointer->lengthXorMask, pointer->andMaskData,
                pointer->lengthAndMask, pointer->xorBpp,
                &context->gdi->palette);

    /* Create surface from image data */
    surface = cairo_image_surface_create_for_data(
        data, CAIRO_FORMAT_ARGB32,
        pointer->width, pointer->height, 4*pointer->width);

    /* Send surface to buffer */
    guac_common_surface_draw(buffer->surface, 0, 0, surface);

    /* Free surface */
    cairo_surface_destroy(surface);
    _aligned_free(data);

    /* Remember buffer */
    ((guac_rdp_pointer*) pointer)->layer = buffer;

    return TRUE;

}

BOOL guac_rdp_pointer_set(rdpContext* context, const rdpPointer* pointer) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Set cursor */
    guac_common_cursor_set_surface(rdp_client->display->cursor,
            pointer->xPos, pointer->yPos,
            ((guac_rdp_pointer*) pointer)->layer->surface);

    return TRUE;

}

void guac_rdp_pointer_free(rdpContext* context, rdpPointer* pointer) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_common_display_layer* buffer = ((guac_rdp_pointer*) pointer)->layer;

    /* Free buffer */
    guac_common_display_free_buffer(rdp_client->display, buffer);

    /* NOTE: FreeRDP-allocated memory for the rdpPointer will be automatically
     * released after this free handler is invoked */

}

BOOL guac_rdp_pointer_set_null(rdpContext* context) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Set cursor to empty/blank graphic */
    guac_common_cursor_set_blank(rdp_client->display->cursor);

    return TRUE;

}

BOOL guac_rdp_pointer_set_default(rdpContext* context) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Set cursor to embedded pointer */
    guac_common_cursor_set_pointer(rdp_client->display->cursor);

    return TRUE;
}

