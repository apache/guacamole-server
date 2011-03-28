
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

#ifndef _GUAC_CLIENT_RDP_HANDLERS
#define _GUAC_CLIENT_RDP_HANDLERS

#include <freerdp/freerdp.h>

void guac_rdp_ui_error(rdpInst* inst, char* text);
void guac_rdp_ui_warning(rdpInst* inst, char* text);
void guac_rdp_ui_unimpl(rdpInst* inst, char* text);
void guac_rdp_ui_begin_update(rdpInst* inst);
void guac_rdp_ui_end_update(rdpInst* inst);
void guac_rdp_ui_desktop_save(rdpInst* inst, int offset, int x, int y, int cx, int cy);
void guac_rdp_ui_desktop_restore(rdpInst* inst, int offset, int x, int y, int cx, int cy);
RD_HBITMAP guac_rdp_ui_create_bitmap(rdpInst* inst, int width, int height, uint8* data);
void guac_rdp_ui_paint_bitmap(rdpInst* inst, int x, int y, int cx, int cy, int width, int height, uint8* data);
void guac_rdp_ui_destroy_bitmap(rdpInst* inst, RD_HBITMAP bmp);
void guac_rdp_ui_line(rdpInst* inst, uint8 opcode, int startx, int starty, int endx, int endy, RD_PEN* pen);
void guac_rdp_ui_rect(rdpInst* inst, int x, int y, int cx, int cy, int colour);
void guac_rdp_ui_polygon(rdpInst* inst, uint8 opcode, uint8 fillmode, RD_POINT* point, int npoints, RD_BRUSH* brush, int bgcolour, int fgcolour);
void guac_rdp_ui_polyline(rdpInst* inst, uint8 opcode, RD_POINT* points, int npoints, RD_PEN* pen);
void guac_rdp_ui_ellipse(rdpInst* inst, uint8 opcode, uint8 fillmode, int x, int y, int cx, int cy, RD_BRUSH* brush, int bgcolour, int fgcolour);
void guac_rdp_ui_start_draw_glyphs(rdpInst* inst, int bgcolour, int fgcolour);
void guac_rdp_ui_draw_glyph(rdpInst* inst, int x, int y, int cx, int cy, RD_HGLYPH glyph);
void guac_rdp_ui_end_draw_glyphs(rdpInst* inst, int x, int y, int cx, int cy);
uint32 guac_rdp_ui_get_toggle_keys_state(rdpInst* inst);
void guac_rdp_ui_bell(rdpInst* inst);
void guac_rdp_ui_destblt(rdpInst* inst, uint8 opcode, int x, int y, int cx, int cy);
void guac_rdp_ui_patblt(rdpInst* inst, uint8 opcode, int x, int y, int cx, int cy, RD_BRUSH* brush, int bgcolour, int fgcolour);
void guac_rdp_ui_screenblt(rdpInst* inst, uint8 opcode, int x, int y, int cx, int cy, int srcx, int srcy);
void guac_rdp_ui_memblt(rdpInst* inst, uint8 opcode, int x, int y, int cx, int cy, RD_HBITMAP src, int srcx, int srcy);
void guac_rdp_ui_triblt(rdpInst* inst, uint8 opcode, int x, int y, int cx, int cy, RD_HBITMAP src, int srcx, int srcy, RD_BRUSH* brush, int bgcolour,  int fgcolour);
RD_HGLYPH guac_rdp_ui_create_glyph(rdpInst* inst, int width, int height, uint8* data);
void guac_rdp_ui_destroy_glyph(rdpInst* inst, RD_HGLYPH glyph);
int guac_rdp_ui_select(rdpInst* inst, int rdp_socket);
void guac_rdp_ui_set_clip(rdpInst* inst, int x, int y, int cx, int cy);
void guac_rdp_ui_reset_clip(rdpInst* inst);
void guac_rdp_ui_resize_window(rdpInst* inst);
void guac_rdp_ui_set_cursor(rdpInst* inst, RD_HCURSOR cursor);
void guac_rdp_ui_destroy_cursor(rdpInst* inst, RD_HCURSOR cursor);
RD_HCURSOR guac_rdp_ui_create_cursor(rdpInst* inst, unsigned int x, unsigned int y, int width, int height, uint8* andmask, uint8* xormask, int bpp);
void guac_rdp_ui_set_null_cursor(rdpInst* inst);
void guac_rdp_ui_set_default_cursor(rdpInst* inst);
RD_HCOLOURMAP guac_rdp_ui_create_colourmap(rdpInst* inst, RD_COLOURMAP* colours);
void guac_rdp_ui_move_pointer(rdpInst* inst, int x, int y);
void guac_rdp_ui_set_colourmap(rdpInst* inst, RD_HCOLOURMAP map);
RD_HBITMAP guac_rdp_ui_create_surface(rdpInst* inst, int width, int height, RD_HBITMAP old);
void guac_rdp_ui_set_surface(rdpInst* inst, RD_HBITMAP surface);
void guac_rdp_ui_destroy_surface(rdpInst* inst, RD_HBITMAP surface);
void guac_rdp_ui_channel_data(rdpInst* inst, int chan_id, char* data, int data_size, int flags, int total_size);

#endif

