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


#ifndef _GUAC_RDP_CLIENT_H
#define _GUAC_RDP_CLIENT_H

#include "config.h"

#include "guac_clipboard.h"
#include "guac_list.h"
#include "guac_surface.h"
#include "rdp_fs.h"
#include "rdp_keymap.h"
#include "rdp_settings.h"

#include <freerdp/freerdp.h>
#include <freerdp/codec/color.h>
#include <guacamole/audio.h>
#include <guacamole/client.h>

#ifdef HAVE_FREERDP_CLIENT_DISP_H
#include <freerdp/client/disp.h>
#endif

#include <pthread.h>
#include <stdint.h>

/**
 * The maximum duration of a frame in milliseconds.
 */
#define GUAC_RDP_FRAME_DURATION 60

/**
 * The amount of time to allow per message read within a frame, in
 * milliseconds. If the server is silent for at least this amount of time, the
 * frame will be considered finished.
 */
#define GUAC_RDP_FRAME_TIMEOUT 10

/**
 * The native resolution of most RDP connections. As Windows and other systems
 * rely heavily on forced 96 DPI, we must assume 96 DPI.
 */
#define GUAC_RDP_NATIVE_RESOLUTION 96

/**
 * The resolution of an RDP connection that would be considered high, but is
 * tolerable in the case that the client display would be unreasonably small
 * otherwise.
 */
#define GUAC_RDP_HIGH_RESOLUTION 120

/**
 * The smallest area, in pixels^2, that would be considered reasonable large
 * screen DPI needs to be adjusted.
 */
#define GUAC_RDP_REASONABLE_AREA (800*600)

/**
 * The maximum number of bytes to allow within the clipboard.
 */
#define GUAC_RDP_CLIPBOARD_MAX_LENGTH 262144

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
     * All settings associated with the current or pending RDP connection.
     */
    guac_rdp_settings settings;

    /**
     * Button mask containing the OR'd value of all currently pressed buttons.
     */
    int mouse_button_mask;

    /**
     * Foreground color for any future glyphs.
     */
    uint32_t glyph_color;

    /**
     * The display.
     */
    guac_common_surface* default_surface;

    /**
     * The surface that GDI operations should draw to. RDP messages exist which
     * change this surface to allow drawing to occur off-screen.
     */
    guac_common_surface* current_surface;

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
     * The current clipboard contents.
     */
    guac_common_clipboard* clipboard;

    /**
     * The format of the clipboard which was requested. Data received from
     * the RDP server should conform to this format. This will be one of
     * several legal clipboard format values defined within FreeRDP, such as
     * CB_FORMAT_TEXT.
     */
    int requested_clipboard_format;

    /**
     * Audio output, if any.
     */
    guac_audio_stream* audio;

    /**
     * The filesystem being shared, if any.
     */
    guac_rdp_fs* filesystem;

#ifdef HAVE_FREERDP_DISPLAY_UPDATE_SUPPORT
    /**
     * Display control interface.
     */
    DispClientContext* disp;
#endif

    /**
     * List of all available static virtual channels.
     */
    guac_common_list* available_svc;

    /**
     * Lock which is locked and unlocked for each RDP message.
     */
    pthread_mutex_t rdp_lock;

    /**
     * Common attributes for locks.
     */
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

    /**
     * The current color palette, as received from the RDP server.
     */
    UINT32 palette[256];

} rdp_freerdp_context;

#endif

