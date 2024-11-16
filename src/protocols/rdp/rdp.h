/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef GUAC_RDP_H
#define GUAC_RDP_H

#include "channels/audio-input/audio-buffer.h"
#include "channels/cliprdr.h"
#include "channels/disp.h"
#include "channels/rdpei.h"
#include "common/clipboard.h"
#include "common/list.h"
#include "config.h"
#include "fs.h"
#include "keyboard.h"
#include "print-job.h"
#include "settings.h"

#ifdef ENABLE_COMMON_SSH
#include "common-ssh/sftp.h"
#include "common-ssh/ssh.h"
#include "common-ssh/user.h"
#endif

#include <freerdp/codec/color.h>
#include <freerdp/freerdp.h>
#include <freerdp/client/rail.h>
#include <guacamole/audio.h>
#include <guacamole/client.h>
#include <guacamole/display.h>
#include <guacamole/rwlock.h>
#include <guacamole/recording.h>
#include <winpr/wtypes.h>

#include <pthread.h>
#include <stdint.h>

#ifdef HAVE_WINPR_ALIGNED
#define GUAC_ALIGNED_FREE winpr_aligned_free
#define GUAC_ALIGNED_MALLOC winpr_aligned_malloc
#else
#define GUAC_ALIGNED_FREE _aligned_free
#define GUAC_ALIGNED_MALLOC _aligned_malloc
#endif

#ifdef FREERDP_HAS_CONTEXT
#define GUAC_RDP_CONTEXT(rdp_instance) ((rdp_instance)->context)
#else
#define GUAC_RDP_CONTEXT(rdp_instance) ((rdp_instance))
#endif

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
    guac_display* display;

    /**
     * The surface that GDI operations should draw to. RDP messages exist which
     * change this surface to allow drawing to occur off-screen.
     */
    guac_display_layer* current_surface;

    /**
     * The current raw context that can be used to draw to Guacamole's default
     * layer. This context is obtained prior to FreeRDP manipulation of the GDI
     * buffer and closed when FreeRDP is done with the GDI buffer. If no
     * drawing to the GDI is currently underway, this will be NULL.
     */
    guac_display_layer_raw_context* current_context;

    /**
     * The current instance of the guac_display render thread. If the thread
     * has not yet been started, this will be NULL.
     */
    guac_display_render_thread* render_thread;

    /**
     * The current state of the keyboard with respect to the RDP session.
     */
    guac_rdp_keyboard* keyboard;

    /**
     * The current state of the clipboard and the CLIPRDR channel.
     */
    guac_rdp_clipboard* clipboard;

    /**
     * Audio output, if any.
     */
    guac_audio_stream* audio;

    /**
     * Audio input buffer, if audio input is enabled.
     */
    guac_rdp_audio_buffer* audio_input;

    /**
     * The filesystem being shared, if any.
     */
    guac_rdp_fs* filesystem;

    /**
     * The currently-active print job, or NULL if no print job is active.
     */
    guac_rdp_print_job* active_job;

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

    /**
     * The in-progress session recording, or NULL if no recording is in
     * progress.
     */
    guac_recording* recording;

    /**
     * Display size update module.
     */
    guac_rdp_disp* disp;

    /**
     * Multi-touch support module (RDPEI).
     */
    guac_rdp_rdpei* rdpei;

    /**
     * List of all available static virtual channels.
     */
    guac_common_list* available_svc;

    /**
     * Common attributes for locks.
     */
    pthread_mutexattr_t attributes;

    /**
     * Lock which is used to synchronizes access to RDP data structures
     * between user input and client threads. It prevents input handlers
     * from running when RDP data structures are allocated or freed
     * by the client thread.
     */
    guac_rwlock lock;

    /**
     * Lock which synchronizes the sending of each RDP message, ensuring
     * attempts to send RDP messages never overlap.
     */
    pthread_mutex_t message_lock;

    /**
     * A pointer to the RAIL interface provided by the RDP client when rail is
     * in use.
     */
    RailClientContext* rail_interface;

    /**
     * Non-zero if XPS mode is enabled, or zero otherwise.
     * NOTE: XPS mode is currently only supported on Windows. Attempts to set
     * XPS mode on linux will be ignored. For more, see:
     * https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-rdpepc/f97d26e7-d862-4cca-8ec9-98d3c69e2717
     */
    int xps_printer_mode_enabled;

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
