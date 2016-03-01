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

#ifndef GUAC_RDP_H
#define GUAC_RDP_H

#include "config.h"

#include "guac_clipboard.h"
#include "guac_display.h"
#include "guac_surface.h"
#include "guac_list.h"
#include "rdp_fs.h"
#include "rdp_keymap.h"
#include "rdp_settings.h"

#include <freerdp/freerdp.h>
#include <freerdp/codec/color.h>
#include <guacamole/audio.h>
#include <guacamole/client.h>

#ifdef ENABLE_COMMON_SSH
#include "guac_sftp.h"
#include "guac_ssh.h"
#include "guac_ssh_user.h"
#endif

#ifdef HAVE_FREERDP_DISPLAY_UPDATE_SUPPORT
#include "rdp_disp.h"
#endif

#include <pthread.h>
#include <stdint.h>

/**
 * RDP-specific client data.
 */
typedef struct guac_rdp_client {

    /**
     * The RDP client thread.
     */
    pthread_t client_thread;

    /**
     * Pointer to the FreeRDP client instance handling the current connection.
     */
    freerdp* rdp_inst;

    /**
     * All settings associated with the current or pending RDP connection.
     */
    guac_rdp_settings* settings;

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
    guac_common_display* display;

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

#ifdef ENABLE_COMMON_SSH
    /**
     * The user and credentials used to authenticate for SFTP.
     */
    guac_common_ssh_user* sftp_user;

    /**
     * The SSH session used for SFTP.
     */
    guac_common_ssh_session* sftp_session;

    /**
     * An SFTP-based filesystem.
     */
    guac_common_ssh_sftp_filesystem* sftp_filesystem;
#endif

#ifdef HAVE_FREERDP_DISPLAY_UPDATE_SUPPORT
    /**
     * Display size update module.
     */
    guac_rdp_disp* disp;
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

} guac_rdp_client;

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

/**
 * RDP client thread. This thread runs throughout the duration of the client,
 * existing as a single instance, shared by all users.
 *
 * @param data
 *     The guac_client to associate with an RDP session, once the RDP
 *     connection succeeds.
 *
 * @return
 *     NULL in all cases. The return value of this thread is expected to be
 *     ignored.
 */
void* guac_rdp_client_thread(void* data);

#endif

