
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is libguac-client-rdp.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2011
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include <stdio.h>
#include <stdlib.h>

#include <cairo/cairo.h>

#include <guacamole/socket.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>

#include <freerdp/freerdp.h>
#include <freerdp/codec/color.h>

#include "client.h"
#include "rdp_bitmap.h"

static CLRCONV _guac_rdp_clrconv = {
    .alpha  = 1,
    .invert = 0,
    .rgb555 = 0,
    .palette = NULL
};

void guac_rdp_bitmap_new(rdpContext* context, rdpBitmap* bitmap) {

    /* Allocate buffer */
    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_socket* socket = client->socket; 
    guac_layer* buffer = guac_client_alloc_buffer(client);

    /* Convert image data if present */
    if (bitmap->data != NULL) {

        /* Convert image data to 32-bit RGB */
        unsigned char* image_buffer = freerdp_image_convert(bitmap->data, NULL,
                bitmap->width, bitmap->height,
                context->instance->settings->color_depth,
                32, (HCLRCONV) &_guac_rdp_clrconv);

        /* Create surface from image data */
        cairo_surface_t* surface = cairo_image_surface_create_for_data(
            bitmap->data, CAIRO_FORMAT_RGB24,
            bitmap->width, bitmap->height, 4*bitmap->width);

        /* Send surface to buffer */
        guac_protocol_send_png(socket, GUAC_COMP_SRC, buffer, 0, 0, surface);

        /* Free surface */
        cairo_surface_destroy(surface);

        /* Free image data if actually alloated */
        if (image_buffer != bitmap->data)
            free(image_buffer);

    }

    /* Store buffer reference in bitmap */
    ((guac_rdp_bitmap*) bitmap)->layer = buffer;

}

void guac_rdp_bitmap_paint(rdpContext* context, rdpBitmap* bitmap) {
    /* STUB */
}

void guac_rdp_bitmap_free(rdpContext* context, rdpBitmap* bitmap) {
    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_client_free_buffer(client, ((guac_rdp_bitmap*) bitmap)->layer);
}

