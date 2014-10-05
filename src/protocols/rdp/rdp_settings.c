/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "config.h"

#include "rdp_settings.h"

#include <freerdp/constants.h>

#ifdef ENABLE_WINPR
#include <winpr/wtypes.h>
#else
#include "compat/winpr-wtypes.h"
#endif

#include <stddef.h>

int guac_rdp_get_width(freerdp* rdp) {
#ifdef LEGACY_RDPSETTINGS
    return rdp->settings->width;
#else
    return rdp->settings->DesktopWidth;
#endif
}

int guac_rdp_get_height(freerdp* rdp) {
#ifdef LEGACY_RDPSETTINGS
    return rdp->settings->height;
#else
    return rdp->settings->DesktopHeight;
#endif
}

int guac_rdp_get_depth(freerdp* rdp) {
#ifdef LEGACY_RDPSETTINGS
    return rdp->settings->color_depth;
#else
    return rdp->settings->ColorDepth;
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

    /* Audio */
#ifdef LEGACY_RDPSETTINGS
#ifdef HAVE_RDPSETTINGS_AUDIOPLAYBACK
    rdp_settings->audio_playback = guac_settings->audio_enabled;
#endif
#else
#ifdef HAVE_RDPSETTINGS_AUDIOPLAYBACK
    rdp_settings->AudioPlayback = guac_settings->audio_enabled;
#endif
#endif

    /* Device redirection */
#ifdef LEGACY_RDPSETTINGS
#ifdef HAVE_RDPSETTINGS_DEVICEREDIRECTION
    rdp_settings->device_redirection =  guac_settings->audio_enabled
                                     || guac_settings->drive_enabled
                                     || guac_settings->printing_enabled;
#endif
#else
#ifdef HAVE_RDPSETTINGS_DEVICEREDIRECTION
    rdp_settings->DeviceRedirection =  guac_settings->audio_enabled
                                    || guac_settings->drive_enabled
                                    || guac_settings->printing_enabled;
#endif
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

    /* RemoteApp */
    if (guac_settings->remote_app != NULL) {
#ifdef LEGACY_RDPSETTINGS
        rdp_settings->workarea = TRUE;
        rdp_settings->remote_app = TRUE;
        rdp_settings->rail_langbar_supported = TRUE;
#else
        rdp_settings->Workarea = TRUE;
        rdp_settings->RemoteApplicationMode = TRUE;
        rdp_settings->RemoteAppLanguageBarSupported = TRUE;
        rdp_settings->RemoteApplicationProgram = guac_settings->remote_app;
        rdp_settings->ShellWorkingDirectory = guac_settings->remote_app_dir;
        rdp_settings->RemoteApplicationCmdLine = guac_settings->remote_app_args;
#endif
    }

    /* Order support */
#ifdef LEGACY_RDPSETTINGS
    bitmap_cache = rdp_settings->bitmap_cache;
    rdp_settings->os_major_type = OSMAJORTYPE_UNSPECIFIED;
    rdp_settings->os_minor_type = OSMINORTYPE_UNSPECIFIED;
    rdp_settings->desktop_resize = TRUE;
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
    rdp_settings->DesktopResize = TRUE;
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

