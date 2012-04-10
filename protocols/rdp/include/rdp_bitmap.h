
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

#ifndef _GUAC_RDP_RDP_BITMAP_H
#define _GUAC_RDP_RDP_BITMAP_H

#include <freerdp/freerdp.h>

#include <guacamole/protocol.h>

typedef struct guac_rdp_bitmap {

    /**
     * FreeRDP bitmap data - MUST GO FIRST.
     */
    rdpBitmap bitmap;

    /**
     * Guacamole layer containing cached image data.
     */
    guac_layer* layer;

    /**
     * The number of times a bitmap has been used.
     */
    int used;

} guac_rdp_bitmap;

void guac_rdp_bitmap_new(rdpContext* context, rdpBitmap* bitmap);
void guac_rdp_bitmap_decompress(rdpContext* context, rdpBitmap* bitmap, uint8* data, int width, int height, int bpp, int length, boolean compressed);
void guac_rdp_bitmap_paint(rdpContext* context, rdpBitmap* bitmap);
void guac_rdp_bitmap_free(rdpContext* context, rdpBitmap* bitmap);
void guac_rdp_bitmap_setsurface(rdpContext* context, rdpBitmap* bitmap, boolean primary);

#endif
