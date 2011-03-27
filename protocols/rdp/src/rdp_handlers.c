
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

#include <freerdp/freerdp.h>

#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/guacio.h>

#include "rdp_handlers.h"

void guac_rdp_ui_error(rdpInst* inst, char* text) {

    guac_client* client = (guac_client*) inst->param1;
    GUACIO* io = client->io;

    guac_send_error(io, text);
    guac_flush(io);

}

void guac_rdp_ui_warning(rdpInst* inst, char* text) {
    fprintf(stderr, "guac_rdp_ui_warning: STUB\n");
}

void guac_rdp_ui_unimpl(rdpInst* inst, char* text) {
    fprintf(stderr, "guac_rdp_ui_unimpl: STUB\n");
}

void guac_rdp_ui_begin_update(rdpInst* inst) {
    fprintf(stderr, "guac_rdp_ui_begin_update: STUB\n");
}

void guac_rdp_ui_end_update(rdpInst* inst) {
    fprintf(stderr, "guac_rdp_ui_end_update: STUB\n");
}

void guac_rdp_ui_desktop_save(rdpInst* inst, int offset, int x, int y, int cx, int cy) {
    fprintf(stderr, "guac_rdp_ui_desktop_save: STUB\n");
}

void guac_rdp_ui_desktop_restore(rdpInst* inst, int offset, int x, int y, int cx, int cy) {
    fprintf(stderr, "guac_rdp_ui_desktop_restore: STUB\n");
}

RD_HBITMAP guac_rdp_ui_create_bitmap(rdpInst* inst, int width, int height, uint8* data) {
    fprintf(stderr, "guac_rdp_ui_create_bitmap: STUB\n");
    return NULL;
}

void guac_rdp_ui_paint_bitmap(rdpInst* inst, int x, int y, int cx, int cy, int width, int height, uint8* data) {
    fprintf(stderr, "guac_rdp_ui_paint_bitmap: STUB\n");
}

void guac_rdp_ui_destroy_bitmap(rdpInst* inst, RD_HBITMAP bmp) {
    fprintf(stderr, "guac_rdp_ui_destroy_bitmap: STUB\n");
}

void guac_rdp_ui_line(rdpInst* inst, uint8 opcode, int startx, int starty, int endx, int endy, RD_PEN* pen) {
    fprintf(stderr, "guac_rdp_ui_line: STUB\n");
}

void guac_rdp_ui_rect(rdpInst* inst, int x, int y, int cx, int cy, int colour) {
    fprintf(stderr, "guac_rdp_ui_rect: STUB\n");
}

void guac_rdp_ui_polygon(rdpInst* inst, uint8 opcode, uint8 fillmode, RD_POINT* point, int npoints, RD_BRUSH* brush, int bgcolour, int fgcolour) {
    fprintf(stderr, "guac_rdp_ui_polygon: STUB\n");
}

void guac_rdp_ui_polyline(rdpInst* inst, uint8 opcode, RD_POINT* points, int npoints, RD_PEN* pen) {
    fprintf(stderr, "guac_rdp_ui_polyline: STUB\n");
}

void guac_rdp_ui_ellipse(rdpInst* inst, uint8 opcode, uint8 fillmode, int x, int y, int cx, int cy, RD_BRUSH*  brush, int bgcolour, int fgcolour) {
    fprintf(stderr, "guac_rdp_ui_ellipse: STUB\n");
}

void guac_rdp_ui_start_draw_glyphs(rdpInst* inst, int bgcolour, int fgcolour) {
    fprintf(stderr, "guac_rdp_ui_start_draw_glyphs: STUB\n");
}

void guac_rdp_ui_draw_glyph(rdpInst* inst, int x, int y, int cx, int cy, RD_HGLYPH glyph) {
    fprintf(stderr, "guac_rdp_ui_draw_glyph: STUB\n");
}

void guac_rdp_ui_end_draw_glyphs(rdpInst* inst, int x, int y, int cx, int cy) {
    fprintf(stderr, "guac_rdp_ui_end_draw_glyphs: STUB\n");
}

uint32 guac_rdp_ui_get_toggle_keys_state(rdpInst* inst) {
    fprintf(stderr, "guac_rdp_ui_get_toggle_keys_state: STUB\n");
    return 0;
}

void guac_rdp_ui_bell(rdpInst* inst) {
    fprintf(stderr, "guac_rdp_ui_bell: STUB\n");
}

void guac_rdp_ui_destblt(rdpInst* inst, uint8 opcode, int x, int y, int cx, int cy) {
    fprintf(stderr, "guac_rdp_ui_destblt: STUB\n");
}

void guac_rdp_ui_patblt(rdpInst* inst, uint8 opcode, int x, int y, int cx, int cy, RD_BRUSH* brush, int bgcolour, int fgcolour) {
    fprintf(stderr, "guac_rdp_ui_patblt: STUB\n");
}

void guac_rdp_ui_screenblt(rdpInst* inst, uint8 opcode, int x, int y, int cx, int cy, int srcx, int srcy) {
    fprintf(stderr, "guac_rdp_ui_screenblt: STUB\n");
}

void guac_rdp_ui_memblt(rdpInst* inst, uint8 opcode, int x, int y, int cx, int cy, RD_HBITMAP src, int srcx, int srcy) {
    fprintf(stderr, "guac_rdp_ui_memblt: STUB\n");
}

void guac_rdp_ui_triblt(rdpInst* inst, uint8 opcode, int x, int y, int cx, int cy, RD_HBITMAP src, int srcx, int srcy, RD_BRUSH* brush, int bgcolour,  int fgcolour) {
    fprintf(stderr, "guac_rdp_ui_triblt: STUB\n");
}

RD_HGLYPH guac_rdp_ui_create_glyph(rdpInst* inst, int width, int height, uint8* data) {
    fprintf(stderr, "guac_rdp_ui_create_glyph: STUB\n");
    return NULL;
}

void guac_rdp_ui_destroy_glyph(rdpInst* inst, RD_HGLYPH glyph) {
    fprintf(stderr, "guac_rdp_ui_destroy_glyph: STUB\n");
}

int guac_rdp_ui_select(rdpInst* inst, int rdp_socket) {
    return 1;
}

void guac_rdp_ui_set_clip(rdpInst* inst, int x, int y, int cx, int cy) {
    fprintf(stderr, "guac_rdp_ui_set_clip: STUB\n");
}

void guac_rdp_ui_reset_clip(rdpInst* inst) {
    fprintf(stderr, "guac_rdp_ui_reset_clip: STUB\n");
}

void guac_rdp_ui_resize_window(rdpInst* inst) {
    fprintf(stderr, "guac_rdp_ui_resize_window: STUB\n");
}

void guac_rdp_ui_set_cursor(rdpInst* inst, RD_HCURSOR cursor) {
    fprintf(stderr, "guac_rdp_ui_set_cursor: STUB\n");
}

void guac_rdp_ui_destroy_cursor(rdpInst* inst, RD_HCURSOR cursor) {
    fprintf(stderr, "guac_rdp_ui_destroy_cursor: STUB\n");
}

RD_HCURSOR guac_rdp_ui_create_cursor(rdpInst* inst, unsigned int x, unsigned int y, int width, int height, uint8* andmask, uint8* xormask, int bpp) {
    fprintf(stderr, "guac_rdp_ui_create_cursor: STUB\n");
    return NULL;
}

void guac_rdp_ui_set_null_cursor(rdpInst* inst) {
    fprintf(stderr, "guac_rdp_ui_set_null_cursor: STUB\n");
}

void guac_rdp_ui_set_default_cursor(rdpInst* inst) {
    fprintf(stderr, "guac_rdp_ui_set_default_cursor: STUB\n");
}

RD_HCOLOURMAP guac_rdp_ui_create_colourmap(rdpInst* inst, RD_COLOURMAP* colours) {
    fprintf(stderr, "guac_rdp_ui_create_colourmap: STUB\n");
    return NULL;
}

void guac_rdp_ui_move_pointer(rdpInst* inst, int x, int y) {
    fprintf(stderr, "guac_rdp_ui_move_pointer: STUB\n");
}

void guac_rdp_ui_set_colourmap(rdpInst* inst, RD_HCOLOURMAP map) {
    fprintf(stderr, "guac_rdp_ui_set_colourmap: STUB\n");
}

RD_HBITMAP guac_rdp_ui_create_surface(rdpInst* inst, int width, int height, RD_HBITMAP old) {
    fprintf(stderr, "guac_rdp_ui_create_surface: STUB\n");
    return NULL;
}

void guac_rdp_ui_set_surface(rdpInst* inst, RD_HBITMAP surface) {
    fprintf(stderr, "guac_rdp_ui_set_surface: STUB\n");
}

void guac_rdp_ui_destroy_surface(rdpInst* inst, RD_HBITMAP surface) {
    fprintf(stderr, "guac_rdp_ui_destroy_surface: STUB\n");
}

void guac_rdp_ui_channel_data(rdpInst* inst, int chan_id, char* data, int data_size, int flags, int total_size) {
    fprintf(stderr, "guac_rdp_ui_channel_data: STUB\n");
}


