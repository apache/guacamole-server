
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

#include "rdp_settings.h"

#ifdef LEGACY_RDPSETTINGS

void guac_rdp_commit_settings(guac_rdp_settings* guac_settings, rdpSettings* rdp_settings) {
    /* STUB */

    /*
    settings->authentication
    settings->autologon
    settings->bitmap_cache
    settings->color_depth
    settings->console_audio
    settings->console_session
    settings->domain
    settings->encryption
    settings->encryption_level
    settings->encryption_method
    settings->height
    settings->hostname
    settings->kbd_layout
    settings->nla_security
    settings->order_support
    settings->os_major_type
    settings->os_minor_type
    settings->password
    settings->port
    settings->rdp_security
    settings->shell
    settings->tls_security
    settings->username
    settings->width
    settings->window_title
    */

#if 0

    BOOL bitmap_cache;

    /* --no-auth */
    settings->authentication = FALSE;

    /* --sec rdp */
    settings->rdp_security = TRUE;
    settings->tls_security = FALSE;
    settings->nla_security = FALSE;
    settings->encryption = TRUE;
    settings->encryption_method = ENCRYPTION_METHOD_40BIT | ENCRYPTION_METHOD_128BIT | ENCRYPTION_METHOD_FIPS;
    settings->encryption_level = ENCRYPTION_LEVEL_CLIENT_COMPATIBLE;

    /* Order support */
    bitmap_cache = settings->bitmap_cache;
    settings->os_major_type = OSMAJORTYPE_UNSPECIFIED;
    settings->os_minor_type = OSMINORTYPE_UNSPECIFIED;
    settings->order_support[NEG_DSTBLT_INDEX] = TRUE;
    settings->order_support[NEG_PATBLT_INDEX] = FALSE; /* PATBLT not yet supported */
    settings->order_support[NEG_SCRBLT_INDEX] = TRUE;
    settings->order_support[NEG_OPAQUE_RECT_INDEX] = TRUE;
    settings->order_support[NEG_DRAWNINEGRID_INDEX] = FALSE;
    settings->order_support[NEG_MULTIDSTBLT_INDEX] = FALSE;
    settings->order_support[NEG_MULTIPATBLT_INDEX] = FALSE;
    settings->order_support[NEG_MULTISCRBLT_INDEX] = FALSE;
    settings->order_support[NEG_MULTIOPAQUERECT_INDEX] = FALSE;
    settings->order_support[NEG_MULTI_DRAWNINEGRID_INDEX] = FALSE;
    settings->order_support[NEG_LINETO_INDEX] = FALSE;
    settings->order_support[NEG_POLYLINE_INDEX] = FALSE;
    settings->order_support[NEG_MEMBLT_INDEX] = bitmap_cache;
    settings->order_support[NEG_MEM3BLT_INDEX] = FALSE;
    settings->order_support[NEG_MEMBLT_V2_INDEX] = bitmap_cache;
    settings->order_support[NEG_MEM3BLT_V2_INDEX] = FALSE;
    settings->order_support[NEG_SAVEBITMAP_INDEX] = FALSE;
    settings->order_support[NEG_GLYPH_INDEX_INDEX] = TRUE;
    settings->order_support[NEG_FAST_INDEX_INDEX] = TRUE;
    settings->order_support[NEG_FAST_GLYPH_INDEX] = TRUE;
    settings->order_support[NEG_POLYGON_SC_INDEX] = FALSE;
    settings->order_support[NEG_POLYGON_CB_INDEX] = FALSE;
    settings->order_support[NEG_ELLIPSE_SC_INDEX] = FALSE;
    settings->order_support[NEG_ELLIPSE_CB_INDEX] = FALSE;
#endif

}

#else

void guac_rdp_commit_settings(guac_rdp_settings* guac_settings, rdpSettings* rdp_settings) {
    /* STUB */
}

#endif

