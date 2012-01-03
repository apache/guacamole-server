
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
#include "rdp_handlers.h"


static CLRCONV _guac_rdp_clrconv = {
    .alpha  = 1,
    .invert = 0,
    .rgb555 = 0,
    .palette = NULL
};


void guac_rdp_convert_color(int depth, int color, guac_rdp_color* comp) {

    switch (depth) {

        case 24:
            comp->red   = (color >> 16) & 0xFF;
            comp->green = (color >>  8) & 0xFF;
            comp->blue  = (color      ) & 0xFF;
            break;

        case 16:
            comp->red   = ((color >> 8) & 0xF8) | ((color >> 13) & 0x07);
            comp->green = ((color >> 3) & 0xFC) | ((color >>  9) & 0x03);
            comp->blue  = ((color << 3) & 0xF8) | ((color >>  2) & 0x07);
            break;

        default: /* The Magenta of Failure */
            comp->red   = 0xFF;
            comp->green = 0x00;
            comp->blue  = 0xFF;
    }

}

void guac_rdp_ui_error(freerdp* inst, const char* text) {

    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    guac_socket* socket = client->socket;

    guac_protocol_send_error(socket, text);
    guac_socket_flush(socket);

}

void guac_rdp_ui_warning(freerdp* inst, const char* text) {
    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    guac_client_log_info(client, "guac_rdp_ui_warning: %s\n", text);
}

void guac_rdp_ui_unimpl(freerdp* inst, const char* text) {
    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    guac_client_log_info(client, "guac_rdp_ui_unimpl: %s\n", text);
}

void guac_rdp_ui_begin_update(freerdp* inst) {
    /* UNUSED */
}

void guac_rdp_ui_end_update(freerdp* inst) {
    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    guac_socket* socket = client->socket;
    guac_socket_flush(socket);
}

void guac_rdp_ui_paint_bitmap(freerdp* inst, int x, int y, int cx, int cy, int width, int height, uint8* data) {

    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;
    const guac_layer* current_surface = guac_client_data->current_surface; 
    guac_socket* socket = client->socket; 

    int dx, dy;
    int stride;
    int bpp = (inst->settings->server_depth + 7) / 8;
    unsigned char* image_buffer;
    unsigned char* image_buffer_row;

    int data_stride = width * bpp;
    unsigned char* data_row = data;

    cairo_surface_t* surface;

    /* Init Cairo buffer */
    stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, cx);
    image_buffer = malloc(height*stride);
    image_buffer_row = image_buffer;

    /* Copy image data from image data to buffer */
    for (dy = 0; dy<cy; dy++) {

        unsigned int*  image_buffer_current;
        unsigned char* data_current = data_row;
        
        /* Get current buffer row, advance to next */
        image_buffer_current  = (unsigned int*) image_buffer_row;
        image_buffer_row     += stride;

        /* Get current data row, advance to next */
        data_current = data_row;
        data_row += data_stride;

        for (dx = 0; dx<cx; dx++) {

            unsigned char red, green, blue;
            unsigned int v;

            switch (bpp) {
                case 3:
                    blue  = *((unsigned char*) data_current++);
                    green = *((unsigned char*) data_current++);
                    red   = *((unsigned char*) data_current++);
                    break;

                case 2:
                    v  = *((unsigned char*) data_current++);
                    v |= *((unsigned char*) data_current++) << 8;

                    red   = ((v >> 8) & 0xF8) | ((v >> 13) & 0x07);
                    green = ((v >> 3) & 0xFC) | ((v >>  9) & 0x03);
                    blue  = ((v << 3) & 0xF8) | ((v >>  2) & 0x07);
                    break;

                default: /* The Magenta of Failure */
                    red   = 0xFF;
                    green = 0x00;
                    blue  = 0xFF;
            }

            /* Output RGB */
            *(image_buffer_current++) = (red << 16) | (green << 8) | blue;

        }
    }

    surface = cairo_image_surface_create_for_data(image_buffer, CAIRO_FORMAT_RGB24, cx, cy, stride);
    guac_protocol_send_png(socket, GUAC_COMP_OVER, current_surface, x, y, surface);
    guac_socket_flush(socket);

    /* Free surface */
    cairo_surface_destroy(surface);
    free(image_buffer);

}

void guac_rdp_ui_destroy_bitmap(freerdp* inst, rdpBitmap* bmp) {

    /* Free buffer */
    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    guac_client_free_buffer(client, (guac_layer*) bmp);

}

void guac_rdp_ui_rect(freerdp* inst, int x, int y, int cx, int cy, uint32 color) {

    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;
    const guac_layer* current_surface = guac_client_data->current_surface; 
    guac_socket* socket = client->socket;

    unsigned char red, green, blue;

    switch (inst->settings->server_depth) {
        case 24:
            red   = (color >> 16) & 0xFF;
            green = (color >>  8) & 0xFF;
            blue  = (color      ) & 0xFF;
            break;

        case 16:
            red   = ((color >> 8) & 0xF8) | ((color >> 13) & 0x07);
            green = ((color >> 3) & 0xFC) | ((color >>  9) & 0x03);
            blue  = ((color << 3) & 0xF8) | ((color >>  2) & 0x07);
            break;

        default: /* The Magenta of Failure */
            red   = 0xFF;
            green = 0x00;
            blue  = 0xFF;
    }

    /* Send rectangle */
    guac_protocol_send_rect(socket, GUAC_COMP_OVER, current_surface,
            x, y, cx, cy,
            red, green, blue, 0xFF);

}

void guac_rdp_ui_start_draw_glyphs(freerdp* inst, uint32 bgcolor, uint32 fgcolor) {

    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;

    guac_rdp_convert_color(
            inst->settings->server_depth,
            bgcolor,
            &(guac_client_data->background));

    guac_rdp_convert_color(
            inst->settings->server_depth,
            fgcolor,
            &(guac_client_data->foreground));

}

void guac_rdp_ui_draw_glyph(freerdp* inst, int x, int y, int width, int height, rdpGlyph* glyph) {

    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;
    const guac_layer* current_surface = guac_client_data->current_surface; 
    guac_socket* socket = client->socket;

    /* NOTE: Originally: Stencil=SRC, FG=ATOP, BG=RATOP */
    /* Temporarily removed BG drawing... */

    /* Foreground */
    guac_protocol_send_rect(socket, GUAC_COMP_ATOP, (guac_layer*) glyph,
            0, 0, width, height,
            guac_client_data->foreground.red,
            guac_client_data->foreground.green,
            guac_client_data->foreground.blue,
            255);

    /* Background */
    /*guac_protocol_send_rect(socket, GUAC_COMP_OVER, current_surface,
            x, y, width, height,
            guac_client_data->background.red,
            guac_client_data->background.green,
            guac_client_data->background.blue,
            255);*/

    /* Draw */
    guac_protocol_send_copy(socket,
            (guac_layer*) glyph, 0, 0, width, height,
            GUAC_COMP_OVER, current_surface, x, y);

}

void guac_rdp_ui_end_draw_glyphs(freerdp* inst, int x, int y, int cx, int cy) {
    /* UNUSED */
}

void guac_rdp_ui_destblt(freerdp* inst, uint8 opcode, int x, int y, int cx, int cy) {
    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    guac_client_log_info(client, "guac_rdp_ui_destblt: STUB\n");
}

void guac_rdp_ui_patblt(freerdp* inst, uint8 opcode, int x, int y, int cx, int cy, rdpBrush* brush, uint32 bgcolor, uint32 fgcolor) {
    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    guac_client_log_info(client, "guac_rdp_ui_patblt: STUB\n");
}

void guac_rdp_ui_screenblt(freerdp* inst, uint8 opcode, int x, int y, int cx, int cy, int srcx, int srcy) {

    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;
    const guac_layer* current_surface = guac_client_data->current_surface; 
    guac_socket* socket = client->socket;

    guac_protocol_send_copy(socket,
            GUAC_DEFAULT_LAYER, srcx, srcy, cx, cy,
            GUAC_COMP_OVER, current_surface, x, y);

}

void guac_rdp_ui_memblt(freerdp* inst, uint8 opcode, int x, int y, int width, int height, rdpBitmap* src, int srcx, int srcy) {

    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;
    const guac_layer* current_surface = guac_client_data->current_surface;
    guac_socket* socket = client->socket;

    if (opcode != 204)
        guac_client_log_info(client,
                "guac_rdp_ui_memblt: opcode=%i, index=%i\n", opcode,
                ((guac_layer*) src)->index);

    guac_protocol_send_copy(socket,
            (guac_layer*) src, srcx, srcy, width, height,
            GUAC_COMP_OVER, current_surface, x, y);

}

void guac_rdp_ui_triblt(freerdp* inst, uint8 opcode, int x, int y, int cx, int cy, rdpBitmap* src, int srcx, int srcy, rdpBrush* brush, uint32 bgcolor,  uint32 fgcolor) {
    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    guac_client_log_info(client, "guac_rdp_ui_triblt: STUB\n");
}

rdpGlyph* guac_rdp_ui_create_glyph(freerdp* inst, int width, int height, uint8* data) {

    /* Allocate buffer */
    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    guac_socket* socket = client->socket;
    guac_layer* glyph = guac_client_alloc_buffer(client);

    int x, y, i;
    int stride;
    unsigned char* image_buffer;
    unsigned char* image_buffer_row;

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
    guac_protocol_send_png(socket, GUAC_COMP_SRC, glyph, 0, 0, surface);
    guac_socket_flush(socket);

    /* Free surface */
    cairo_surface_destroy(surface);
    free(image_buffer);


    return (rdpGlyph*) glyph;

}

void guac_rdp_ui_destroy_glyph(freerdp* inst, rdpGlyph* glyph) {

    /* Free buffer */
    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    guac_client_free_buffer(client, (guac_layer*) glyph);

}

int guac_rdp_ui_select(freerdp* inst, int rdp_socket) {
    return 1;
}

void guac_rdp_ui_set_clip(freerdp* inst, int x, int y, int cx, int cy) {

    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;
    const guac_layer* current_surface = guac_client_data->current_surface; 
    guac_socket* socket = client->socket;

    guac_protocol_send_clip(socket, current_surface, x, y, cx, cy);

}

void guac_rdp_ui_reset_clip(freerdp* inst) {

    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;
    const guac_layer* current_surface = guac_client_data->current_surface; 
    guac_socket* socket = client->socket;

    guac_protocol_send_clip(socket, current_surface, 0, 0, inst->settings->width, inst->settings->height);

}

void guac_rdp_ui_resize_window(freerdp* inst) {
    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    guac_client_log_info(client, "guac_rdp_ui_resize_window: %ix%i\n", inst->settings->width, inst->settings->height);
}

void guac_rdp_ui_set_cursor(freerdp* inst, rdpPointer* cursor) {
    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    guac_client_log_info(client, "guac_rdp_ui_set_cursor: STUB\n");
}

void guac_rdp_ui_destroy_cursor(freerdp* inst, rdpPointer* cursor) {
    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    guac_client_log_info(client, "guac_rdp_ui_destroy_cursor: STUB\n");
}

rdpPointer* guac_rdp_ui_create_cursor(freerdp* inst, unsigned int x, unsigned int y, int width, int height, uint8* andmask, uint8* xormask, int bpp) {
    
    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    guac_client_log_info(client, "guac_rdp_ui_create_cursor: STUB\n");
    return (rdpPointer*) guac_client_alloc_buffer(client);

}

void guac_rdp_ui_set_null_cursor(freerdp* inst) {
    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    guac_client_log_info(client, "guac_rdp_ui_set_null_cursor: STUB\n");
}

void guac_rdp_ui_set_default_cursor(freerdp* inst) {
    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    guac_client_log_info(client, "guac_rdp_ui_set_default_cursor: STUB\n");
}

void guac_rdp_ui_move_pointer(freerdp* inst, int x, int y) {
    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    guac_client_log_info(client, "guac_rdp_ui_move_pointer: STUB\n");
}

rdpBitmap* guac_rdp_ui_create_surface(freerdp* inst, int width, int height, rdpBitmap* old) {

    /* If old provided, just return that one ... */
    if (old != NULL)
        return old;

    /* Otherwise allocate and return new buffer */
    else {
        guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
        return (rdpBitmap*) guac_client_alloc_buffer(client);
    }

}

void guac_rdp_ui_set_surface(freerdp* inst, rdpBitmap* surface) {

    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    rdp_guac_client_data* guac_client_data = (rdp_guac_client_data*) client->data;
    guac_socket* socket = client->socket;

    guac_client_log_info(client, "guac_rdp_ui_set_surface: %p (index=%i)\n", surface, surface != NULL ? ((guac_layer*) surface)->index : 0);

    /* Init desktop */
    if (surface == NULL) {

        guac_protocol_send_name(socket, inst->settings->server);
        guac_protocol_send_size(socket, inst->settings->width, inst->settings->height);
        guac_socket_flush(socket);

        guac_client_data->current_surface = GUAC_DEFAULT_LAYER;
    }
    else {
        guac_client_data->current_surface = (guac_layer*) surface;
    }

}

void guac_rdp_ui_destroy_surface(freerdp* inst, rdpBitmap* surface) {

    /* Free buffer */
    guac_client* client = ((rdp_freerdp_context*) inst->context)->client;
    guac_client_free_buffer(client, (guac_layer*) surface);

}

