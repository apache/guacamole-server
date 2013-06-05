
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
 * The Original Code is libguac-client-ssh.
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

#include "cursor.h"

/* Macros for prettying up the embedded image. */
#define X 0x00,0x00,0x00,0xFF
#define U 0x80,0x80,0x80,0xFF
#define O 0xFF,0xFF,0xFF,0xFF
#define _ 0x00,0x00,0x00,0x00

/* Dimensions */
const int guac_ssh_ibar_width  = 7;
const int guac_ssh_ibar_height = 16;

/* Format */
const cairo_format_t guac_ssh_ibar_format = CAIRO_FORMAT_ARGB32;
const int guac_ssh_ibar_stride = 28;

/* Embedded pointer graphic */
unsigned char guac_ssh_ibar[] = {

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


guac_ssh_cursor* guac_ssh_create_ibar(guac_client* client) {

    guac_socket* socket = client->socket;
    guac_ssh_cursor* cursor = guac_ssh_cursor_alloc(client);

    /* Draw to buffer */
    cairo_surface_t* graphic = cairo_image_surface_create_for_data(
            guac_ssh_ibar,
            guac_ssh_ibar_format,
            guac_ssh_ibar_width,
            guac_ssh_ibar_height,
            guac_ssh_ibar_stride);

    guac_protocol_send_png(socket, GUAC_COMP_SRC, cursor->buffer,
            0, 0, graphic);
    cairo_surface_destroy(graphic);

    /* Initialize cursor properties */
    cursor->width = guac_ssh_ibar_width;
    cursor->height = guac_ssh_ibar_height;
    cursor->hotspot_x = guac_ssh_ibar_width / 2;
    cursor->hotspot_y = guac_ssh_ibar_height / 2;

    return cursor;

}

