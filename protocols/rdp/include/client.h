
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

#ifndef _GUAC_RDP_CLIENT_H
#define _GUAC_RDP_CLIENT_H

#include <cairo/cairo.h>

#include <freerdp/freerdp.h>
#include <freerdp/codec/color.h>

#include <guacamole/client.h>

#include "audio.h"
#include "rdp_keymap.h"

/**
 * The default RDP port.
 */
#define RDP_DEFAULT_PORT 3389

/**
 * Default screen width, in pixels.
 */
#define RDP_DEFAULT_WIDTH  1024

/**
 * Default screen height, in pixels.
 */
#define RDP_DEFAULT_HEIGHT 768 

/**
 * Default color depth, in bits.
 */
#define RDP_DEFAULT_DEPTH  16 

/**
 * Client data that will remain accessible through the guac_client.
 * This should generally include data commonly used by Guacamole handlers.
 */
typedef struct rdp_guac_client_data {

    /**
     * Pointer to the FreeRDP client instance handling the current connection.
     */
    freerdp* rdp_inst;

    /**
     * The settings structure associated with the FreeRDP client instance
     * handling the current connection.
     */
    rdpSettings* settings;

    /**
     * Button mask containing the OR'd value of all currently pressed buttons.
     */
    int mouse_button_mask;

    /**
     * Cairo surface which will receive all TRANSPARENT glyphs.
     */
    cairo_surface_t* trans_glyph_surface;

    /**
     * Cairo surface which will receive all OPAQUE glyphs.
     */
    cairo_surface_t* opaque_glyph_surface;

    /**
     * The current Cairo surface which will receive all drawn glyphs,
     * depending on whether we are currently drawing transparent or
     * opaque glyphs.
     */
    cairo_surface_t* glyph_surface;

    /**
     * Cairo instance for drawing to the current glyph surface.
     */
    cairo_t* glyph_cairo;

    /**
     * The Guacamole layer that GDI operations should draw to. RDP messages
     * exist which change this surface to allow drawing to occur off-screen.
     */
    const guac_layer* current_surface;

    /**
     * The keymap to use when translating keysyms into scancodes or sequences
     * of scancodes for RDP.
     */
    guac_rdp_static_keymap keymap;

    /**
     * The state of all keys, based on whether events for pressing/releasing
     * particular keysyms have been received. This is necessary in order to
     * determine which keys must be released/pressed when a particular
     * keysym can only be typed through a sequence of scancodes (such as
     * an Alt-code) because the server-side keymap does not support that
     * keysym.
     */
    guac_rdp_keysym_state_map keysym_state;

    /**
     * The current text (NOT Unicode) clipboard contents.
     */
    char* clipboard;

    /**
     * Audio output, if any.
     */
    audio_stream* audio;

    /**
     * Lock which is locked and unlocked for each update.
     */
    pthread_mutex_t update_lock;

    pthread_mutexattr_t attributes;

} rdp_guac_client_data;

/**
 * Client data that will remain accessible through the RDP context.
 * This should generally include data commonly used by FreeRDP handlers.
 */
typedef struct rdp_freerdp_context {

    /**
     * The parent context. THIS MUST BE THE FIRST ELEMENT.
     */
    rdpContext _p;

    /**
     * Pointer to the guac_client instance handling the RDP connection with
     * this context.
     */
    guac_client* client;

    /**
     * Color conversion structure to be used to convert RDP images to PNGs.
     */
    CLRCONV* clrconv;

} rdp_freerdp_context;

#endif

