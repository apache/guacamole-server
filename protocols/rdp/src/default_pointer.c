
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

#include <cairo/cairo.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>

/* Macros for prettying up the embedded image. */
#define X 0x00,0x00,0x00,0xFF
#define O 0xFF,0xFF,0xFF,0xFF
#define _ 0x00,0x00,0x00,0x00

/* Dimensions */
const int guac_rdp_default_pointer_width  = 11;
const int guac_rdp_default_pointer_height = 16;

/* Format */
const cairo_format_t guac_rdp_default_pointer_format = CAIRO_FORMAT_ARGB32;
const int guac_rdp_default_pointer_stride = 44;

/* Embedded pointer graphic */
unsigned char guac_rdp_default_pointer[] = {

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


void guac_rdp_pointer_set_default(guac_client* client) {

    guac_socket* socket = client->socket;

    /* Draw to buffer */
    guac_layer* cursor = guac_client_alloc_buffer(client);

    cairo_surface_t* graphic = cairo_image_surface_create_for_data(
            guac_rdp_default_pointer,
            guac_rdp_default_pointer_format,
            guac_rdp_default_pointer_width,
            guac_rdp_default_pointer_height,
            guac_rdp_default_pointer_stride);

    guac_protocol_send_png(socket, GUAC_COMP_SRC, cursor, 0, 0, graphic);
    cairo_surface_destroy(graphic);

    /* Set cursor */
    guac_protocol_send_cursor(socket, 0, 0, cursor,
            0, 0,
            guac_rdp_default_pointer_width,
            guac_rdp_default_pointer_height);

    /* Free buffer */
    guac_client_free_buffer(client, cursor);

}

