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

#include "guac_list.h"
#include "rdp_fs.h"
#include "rdp_keymap.h"
#include "rdp_settings.h"

#include <pthread.h>

#include <cairo/cairo.h>
#include <freerdp/freerdp.h>
#include <freerdp/codec/color.h>
#include <guacamole/audio.h>
#include <guacamole/client.h>

/**
 * The maximum duration of a frame in milliseconds.
 */
#define GUAC_RDP_FRAME_DURATION 40

/**
 * The amount of time to allow per message read within a frame, in
 * milliseconds. If the server is silent for at least this amount of time, the
 * frame will be considered finished.
 */
#define GUAC_RDP_FRAME_TIMEOUT 0

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
     * Whether graphical operations are restricted to a specific bounding
     * rectangle.
     */
    int bounded;

    /**
     * The X coordinate of the upper-left corner of the bounding rectangle,
     * if any.
     */
    int bounds_left;

    /**
     * The Y coordinate of the upper-left corner of the bounding rectangle,
     * if any.
     */
    int bounds_top;

    /**
     * The X coordinate of the lower-right corner of the bounding rectangle,
     * if any.
     */
    int bounds_right;

    /**
     * The Y coordinate of the lower-right corner of the bounding rectangle,
     * if any.
     */
    int bounds_bottom;

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
    guac_audio_stream* audio;

    /**
     * The filesystem being shared, if any.
     */
    guac_rdp_fs* filesystem;

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

} rdp_freerdp_context;

/**
 * The transfer status of a file being downloaded.
 */
typedef struct guac_rdp_download_status {

    /**
     * The file ID of the file being downloaded.
     */
    int file_id;

    /**
     * The current position within the file.
     */
    uint64_t offset;

} guac_rdp_download_status;

/**
 * Given the coordinates and dimensions of a rectangle, clips the rectangle to be
 * within the clipping bounds of the client data, if clipping is active.
 *
 * Returns 0 if the rectangle given is visible at all, and 1 if the entire
 * rectangls is outside the clipping rectangle and this invisible.
 */
int guac_rdp_clip_rect(rdp_guac_client_data* data, int* x, int* y, int* w, int* h);

#endif

