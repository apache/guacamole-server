
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

#ifndef _GUAC_RDP_RDP_HANDLERS_H
#define _GUAC_RDP_RDP_HANDLERS_H

#include <freerdp/freerdp.h>
#include <freerdp/codec/bitmap.h>

int guac_rdp_ui_select(freerdp* inst, int rdp_socket);

void guac_rdp_ui_resize_window(freerdp* inst);

void guac_rdp_ui_move_pointer(freerdp* inst, int x, int y);

void guac_rdp_ui_set_clip(freerdp* inst, int x, int y, int cx, int cy);
void guac_rdp_ui_reset_clip(freerdp* inst);

void guac_rdp_ui_rect(freerdp* inst, int x, int y, int cx, int cy, uint32 colour);

rdpBitmap* guac_rdp_ui_create_bitmap(freerdp* inst, int width, int height, uint8* data);
void guac_rdp_ui_paint_bitmap(freerdp* inst, int x, int y, int cx, int cy, int width, int height, uint8* data);
void guac_rdp_ui_destroy_bitmap(freerdp* inst, rdpBitmap* bmp);

void guac_rdp_ui_destblt(freerdp* inst, uint8 opcode, int x, int y, int cx, int cy);
void guac_rdp_ui_patblt(freerdp* inst, uint8 opcode, int x, int y, int cx, int cy, rdpBrush* brush, uint32 bgcolor, uint32 fgcolor);
void guac_rdp_ui_screenblt(freerdp* inst, uint8 opcode, int x, int y, int cx, int cy, int srcx, int srcy);
void guac_rdp_ui_memblt(freerdp* inst, uint8 opcode, int x, int y, int width, int height, rdpBitmap* src, int srcx, int srcy);
void guac_rdp_ui_triblt(freerdp* inst, uint8 opcode, int x, int y, int cx, int cy, rdpBitmap* src, int srcx, int srcy, rdpBrush* brush, uint32 bgcolor,  uint32 fgcolor);

void guac_rdp_ui_start_draw_glyphs(freerdp* inst, uint32 bgcolor, uint32 fgcolor);
void guac_rdp_ui_draw_glyph(freerdp* inst, int x, int y, int cx, int cy, rdpGlyph* glyph);
void guac_rdp_ui_end_draw_glyphs(freerdp* inst, int x, int y, int cx, int cy);
rdpGlyph* guac_rdp_ui_create_glyph(freerdp* inst, int width, int height, uint8* data);
void guac_rdp_ui_destroy_glyph(freerdp* inst, rdpGlyph* glyph);

void guac_rdp_ui_set_pointer(freerdp* inst, rdpPointer pointer);
void guac_rdp_ui_destroy_pointer(freerdp* inst, rdpPointer pointer);
rdpPointer guac_rdp_ui_create_pointer(freerdp* inst, unsigned int x, unsigned int y, int width, int height, uint8* andmask, uint8* xormask, int bpp);
void guac_rdp_ui_set_null_pointer(freerdp* inst);
void guac_rdp_ui_set_default_pointer(freerdp* inst);

rdpBitmap* guac_rdp_ui_create_surface(freerdp* inst, int width, int height, rdpBitmap* old);
void guac_rdp_ui_set_surface(freerdp* inst, rdpBitmap* surface);
void guac_rdp_ui_destroy_surface(freerdp* inst, rdpBitmap* surface);

#endif

