
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

#include <freerdp/constants.h>
#include "rdp_settings.h"

void guac_rdp_pull_settings(freerdp* rdp, guac_rdp_settings* guac_settings) {

    rdpSettings* rdp_settings = rdp->settings;

#ifdef LEGACY_RDPSETTINGS
    guac_settings->color_depth = rdp_settings->color_depth;
    guac_settings->width       = rdp_settings->width;
    guac_settings->height      = rdp_settings->height;
#else
    guac_settings->color_depth = rdp_settings->ColorDepth;
    guac_settings->width       = rdp_settings->DesktopWidth;
    guac_settings->height      = rdp_settings->DesktopHeight;
#endif

}

void guac_rdp_push_settings(guac_rdp_settings* guac_settings, freerdp* rdp) {

    BOOL bitmap_cache;
    rdpSettings* rdp_settings = rdp->settings;

    /* Authentication */
#ifdef LEGACY_RDPSETTINGS
    rdp_settings->domain = guac_settings->domain;
    rdp_settings->username = guac_settings->username;
    rdp_settings->password = guac_settings->password;
#else
    rdp_settings->Domain = guac_settings->domain;
    rdp_settings->Username = guac_settings->username;
    rdp_settings->Password = guac_settings->password;
#endif

    /* Connection */
#ifdef LEGACY_RDPSETTINGS
    rdp_settings->hostname = guac_settings->hostname;
    rdp_settings->port = guac_settings->port;
#else
    rdp_settings->ServerHostname = guac_settings->hostname;
    rdp_settings->ServerPort = guac_settings->port;
#endif

    /* Session */
#ifdef LEGACY_RDPSETTINGS
    rdp_settings->color_depth = guac_settings->color_depth;
    rdp_settings->width = guac_settings->width;
    rdp_settings->height = guac_settings->height;
    rdp_settings->shell = guac_settings->initial_program;
    rdp_settings->kbd_layout = guac_settings->server_layout->freerdp_keyboard_layout;
#else
    rdp_settings->ColorDepth = guac_settings->color_depth;
    rdp_settings->DesktopWidth = guac_settings->width;
    rdp_settings->DesktopHeight = guac_settings->height;
    rdp_settings->AlternateShell = guac_settings->initial_program;
    rdp_settings->KeyboardLayout = guac_settings->server_layout->freerdp_keyboard_layout;
#endif

    /* Console */
#ifdef LEGACY_RDPSETTINGS
    rdp_settings->console_session = guac_settings->console;
    rdp_settings->console_audio = guac_settings->console_audio;
#else
    rdp_settings->ConsoleSession = guac_settings->console;
    rdp_settings->RemoteConsoleAudio = guac_settings->console_audio;
#endif

    /* Security */
    switch (guac_settings->security_mode) {

        /* Standard RDP encryption */
        case GUAC_SECURITY_RDP:
#ifdef LEGACY_RDPSETTINGS
            rdp_settings->rdp_security = TRUE;
            rdp_settings->tls_security = FALSE;
            rdp_settings->nla_security = FALSE;
            rdp_settings->encryption_level = ENCRYPTION_LEVEL_CLIENT_COMPATIBLE;
            rdp_settings->encryption_method =
                  ENCRYPTION_METHOD_40BIT
                | ENCRYPTION_METHOD_128BIT
                | ENCRYPTION_METHOD_FIPS;
#else
            rdp_settings->RdpSecurity = TRUE;
            rdp_settings->TlsSecurity = FALSE;
            rdp_settings->NlaSecurity = FALSE;
            rdp_settings->EncryptionLevel = ENCRYPTION_LEVEL_CLIENT_COMPATIBLE;
            rdp_settings->EncryptionMethods =
                  ENCRYPTION_METHOD_40BIT
                | ENCRYPTION_METHOD_128BIT 
                | ENCRYPTION_METHOD_FIPS;
#endif
            break;

        /* TLS encryption */
        case GUAC_SECURITY_TLS:
#ifdef LEGACY_RDPSETTINGS
            rdp_settings->rdp_security = FALSE;
            rdp_settings->tls_security = TRUE;
            rdp_settings->nla_security = FALSE;
#else
            rdp_settings->RdpSecurity = FALSE;
            rdp_settings->TlsSecurity = TRUE;
            rdp_settings->NlaSecurity = FALSE;
#endif
            break;

        /* Network level authentication */
        case GUAC_SECURITY_NLA:
#ifdef LEGACY_RDPSETTINGS
            rdp_settings->rdp_security = FALSE;
            rdp_settings->tls_security = FALSE;
            rdp_settings->nla_security = TRUE;
#else
            rdp_settings->RdpSecurity = FALSE;
            rdp_settings->TlsSecurity = FALSE;
            rdp_settings->NlaSecurity = TRUE;
#endif
            break;

        /* All security types */
        case GUAC_SECURITY_ANY:
#ifdef LEGACY_RDPSETTINGS
            rdp_settings->rdp_security = TRUE;
            rdp_settings->tls_security = TRUE;
            rdp_settings->nla_security = TRUE;
#else
            rdp_settings->RdpSecurity = TRUE;
            rdp_settings->TlsSecurity = TRUE;
            rdp_settings->NlaSecurity = TRUE;
#endif
            break;

    }

    /* Authentication */
#ifdef LEGACY_RDPSETTINGS
    rdp_settings->authentication = !guac_settings->disable_authentication;
    rdp_settings->ignore_certificate = guac_settings->ignore_certificate;
    rdp_settings->encryption = TRUE;
#else
    rdp_settings->Authentication = !guac_settings->disable_authentication;
    rdp_settings->IgnoreCertificate = guac_settings->ignore_certificate;
    rdp_settings->DisableEncryption = FALSE;
#endif

    /* Order support */
#ifdef LEGACY_RDPSETTINGS
    bitmap_cache = rdp_settings->bitmap_cache;
    rdp_settings->os_major_type = OSMAJORTYPE_UNSPECIFIED;
    rdp_settings->os_minor_type = OSMINORTYPE_UNSPECIFIED;
    rdp_settings->order_support[NEG_DSTBLT_INDEX] = TRUE;
    rdp_settings->order_support[NEG_PATBLT_INDEX] = FALSE; /* PATBLT not yet supported */
    rdp_settings->order_support[NEG_SCRBLT_INDEX] = TRUE;
    rdp_settings->order_support[NEG_OPAQUE_RECT_INDEX] = TRUE;
    rdp_settings->order_support[NEG_DRAWNINEGRID_INDEX] = FALSE;
    rdp_settings->order_support[NEG_MULTIDSTBLT_INDEX] = FALSE;
    rdp_settings->order_support[NEG_MULTIPATBLT_INDEX] = FALSE;
    rdp_settings->order_support[NEG_MULTISCRBLT_INDEX] = FALSE;
    rdp_settings->order_support[NEG_MULTIOPAQUERECT_INDEX] = FALSE;
    rdp_settings->order_support[NEG_MULTI_DRAWNINEGRID_INDEX] = FALSE;
    rdp_settings->order_support[NEG_LINETO_INDEX] = FALSE;
    rdp_settings->order_support[NEG_POLYLINE_INDEX] = FALSE;
    rdp_settings->order_support[NEG_MEMBLT_INDEX] = bitmap_cache;
    rdp_settings->order_support[NEG_MEM3BLT_INDEX] = FALSE;
    rdp_settings->order_support[NEG_MEMBLT_V2_INDEX] = bitmap_cache;
    rdp_settings->order_support[NEG_MEM3BLT_V2_INDEX] = FALSE;
    rdp_settings->order_support[NEG_SAVEBITMAP_INDEX] = FALSE;
    rdp_settings->order_support[NEG_GLYPH_INDEX_INDEX] = TRUE;
    rdp_settings->order_support[NEG_FAST_INDEX_INDEX] = TRUE;
    rdp_settings->order_support[NEG_FAST_GLYPH_INDEX] = TRUE;
    rdp_settings->order_support[NEG_POLYGON_SC_INDEX] = FALSE;
    rdp_settings->order_support[NEG_POLYGON_CB_INDEX] = FALSE;
    rdp_settings->order_support[NEG_ELLIPSE_SC_INDEX] = FALSE;
    rdp_settings->order_support[NEG_ELLIPSE_CB_INDEX] = FALSE;
#else
    bitmap_cache = rdp_settings->BitmapCacheEnabled;
    rdp_settings->OsMajorType = OSMAJORTYPE_UNSPECIFIED;
    rdp_settings->OsMinorType = OSMINORTYPE_UNSPECIFIED;
    rdp_settings->OrderSupport[NEG_DSTBLT_INDEX] = TRUE;
    rdp_settings->OrderSupport[NEG_PATBLT_INDEX] = FALSE; /* PATBLT not yet supported */
    rdp_settings->OrderSupport[NEG_SCRBLT_INDEX] = TRUE;
    rdp_settings->OrderSupport[NEG_OPAQUE_RECT_INDEX] = TRUE;
    rdp_settings->OrderSupport[NEG_DRAWNINEGRID_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_MULTIDSTBLT_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_MULTIPATBLT_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_MULTISCRBLT_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_MULTIOPAQUERECT_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_MULTI_DRAWNINEGRID_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_LINETO_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_POLYLINE_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_MEMBLT_INDEX] = bitmap_cache;
    rdp_settings->OrderSupport[NEG_MEM3BLT_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_MEMBLT_V2_INDEX] = bitmap_cache;
    rdp_settings->OrderSupport[NEG_MEM3BLT_V2_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_SAVEBITMAP_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_GLYPH_INDEX_INDEX] = TRUE;
    rdp_settings->OrderSupport[NEG_FAST_INDEX_INDEX] = TRUE;
    rdp_settings->OrderSupport[NEG_FAST_GLYPH_INDEX] = TRUE;
    rdp_settings->OrderSupport[NEG_POLYGON_SC_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_POLYGON_CB_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_ELLIPSE_SC_INDEX] = FALSE;
    rdp_settings->OrderSupport[NEG_ELLIPSE_CB_INDEX] = FALSE;
#endif

}

