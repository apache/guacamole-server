
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


#include <freerdp/freerdp.h>

#include <guacamole/client.h>

#include "client.h"
#include "rdp_pointer.h"
#include "default_pointer.h"

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
    guac_protocol_send_png(socket, GUAC_COMP_SRC, buffer, 0, 0, surface);

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

