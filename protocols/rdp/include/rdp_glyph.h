
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

#ifndef _GUAC_RDP_RDP_GLYPH_H
#define _GUAC_RDP_RDP_GLYPH_H

#include <freerdp/freerdp.h>

#include <guacamole/protocol.h>

typedef struct guac_rdp_glyph {

    /**
     * FreeRDP glyph data - MUST GO FIRST.
     */
    rdpGlyph glyph;

    /**
     * Guacamole layer containing cached image data.
     */
    guac_layer* layer;

} guac_rdp_glyph;

void guac_rdp_glyph_new(rdpContext* context, rdpGlyph* glyph);
void guac_rdp_glyph_draw(rdpContext* context, rdpGlyph* glyph, int x, int y);
void guac_rdp_glyph_free(rdpContext* context, rdpGlyph* glyph);
void guac_rdp_glyph_begindraw(rdpContext* context,
        int x, int y, int width, int height, uint32 bgcolor, uint32 fgcolor);
void guac_rdp_glyph_enddraw(rdpContext* context,
        int x, int y, int width, int height, uint32 bgcolor, uint32 fgcolor);

#endif
