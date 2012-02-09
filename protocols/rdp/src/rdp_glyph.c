
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
#include "rdp_glyph.h"

void guac_rdp_glyph_new(rdpContext* context, rdpGlyph* glyph) {

    /* Allocate buffer */
    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_socket* socket = client->socket;
    guac_layer* layer = guac_client_alloc_buffer(client);

    int x, y, i;
    int stride;
    unsigned char* image_buffer;
    unsigned char* image_buffer_row;

    unsigned char* data = glyph->aj;
    int width  = glyph->cx;
    int height = glyph->cy;

    cairo_surface_t* surface;

    /* Init Cairo buffer */
    stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
    image_buffer = malloc(height*stride);
    image_buffer_row = image_buffer;

    /* Copy image data from image data to buffer */
    for (y = 0; y<height; y++) {

        unsigned int*  image_buffer_current;
        
        /* Get current buffer row, advance to next */
        image_buffer_current  = (unsigned int*) image_buffer_row;
        image_buffer_row     += stride;

        for (x = 0; x<width;) {

            /* Get byte from image data */
            unsigned int v = *(data++);

            /* Read bits, write pixels */
            for (i = 0; i<8 && x<width; i++, x++) {

                /* Output RGB */
                if (v & 0x80)
                    *(image_buffer_current++) = 0xFF000000;
                else
                    *(image_buffer_current++) = 0x00000000;

                /* Next bit */
                v <<= 1;

            }

        }
    }

    surface = cairo_image_surface_create_for_data(image_buffer, CAIRO_FORMAT_ARGB32, width, height, stride);
    guac_protocol_send_png(socket, GUAC_COMP_SRC, layer, 0, 0, surface);
    guac_socket_flush(socket);

    /* Free surface */
    cairo_surface_destroy(surface);
    free(image_buffer);

    /* Store layer */
    ((guac_rdp_glyph*) glyph)->layer = layer;

}

void guac_rdp_glyph_draw(rdpContext* context, rdpGlyph* glyph, int x, int y) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    const guac_layer* current_layer = ((rdp_guac_client_data*) client->data)->current_surface;
   
    /* Draw glyph */
    guac_protocol_send_copy(client->socket,
           ((guac_rdp_glyph*) glyph)->layer, 0, 0, glyph->cx, glyph->cy,
           GUAC_COMP_OVER, current_layer, x, y); 

}

void guac_rdp_glyph_free(rdpContext* context, rdpGlyph* glyph) {
    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_client_free_buffer(client, ((guac_rdp_glyph*) glyph)->layer);
}

void guac_rdp_glyph_begindraw(rdpContext* context,
        int x, int y, int width, int height, uint32 bgcolor, uint32 fgcolor) {
    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_client_log_info(client, "guac_rdp_glyph_begindraw()");
}

void guac_rdp_glyph_enddraw(rdpContext* context,
        int x, int y, int width, int height, uint32 bgcolor, uint32 fgcolor) {
    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_client_log_info(client, "guac_rdp_glyph_enddraw()");
}

