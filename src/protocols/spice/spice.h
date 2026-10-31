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

#ifndef GUAC_SPICE_SPICE_H
#define GUAC_SPICE_SPICE_H

#include "channels/file.h"
#include "common/clipboard.h"
#include "keyboard.h"
#include "settings.h"

#include <guacamole/audio.h>
#include <guacamole/client.h>
#include <guacamole/display.h>
#include <guacamole/recording.h>
#include <guacamole/thread.h>
#include <spice-client.h>

#ifdef ENABLE_COMMON_SSH
#include "common-ssh/sftp.h"
#include "common-ssh/ssh.h"
#include "common-ssh/user.h"
#endif

#include <pthread.h>
#include <stdbool.h>

/**
 * The maximum number of milliseconds to wait between iterations of the
 * SPICE client event loop while checking whether the connection should
 * continue running.
 */
#define GUAC_SPICE_STATE_CHECK_INTERVAL 250

/**
 * The maximum number of main context iterations performed when draining
 * pending sources after the SPICE session is disconnected. This is a safeguard
 * against a source that never stops being ready; normal teardown needs only a
 * handful of iterations.
 */
#define GUAC_SPICE_DRAIN_MAX_ITERATIONS 10000

/**
 * SPICE-specific client data.
 */
typedef struct guac_spice_client {

    /**
     * The SPICE client thread.
     */
    guac_thread client_thread;

    /**
     * The underlying SPICE session, which manages all individual channel
     * connections to the SPICE server.
     */
    SpiceSession* spice_session;

    /**
     * The main SPICE channel, used for session-wide state such as mouse mode,
     * agent connectivity, and clipboard exchange. NULL until the channel has
     * been opened.
     */
    SpiceMainChannel* main_channel;

    /**
     * The SPICE display channel, providing the remote framebuffer. NULL until
     * the channel has been opened.
     */
    SpiceChannel* display_channel;

    /**
     * The SPICE inputs channel, used for sending keyboard and mouse events.
     * NULL until the channel has been opened.
     */
    SpiceInputsChannel* inputs_channel;

    /**
     * The current keyboard state, including the mapping of keysyms to SPICE
     * scancodes for the negotiated keyboard layout. NULL until allocated within
     * the SPICE client thread.
     */
    guac_spice_keyboard* keyboard;

    /**
     * Lock which serializes messages sent to the SPICE server (such as keyboard
     * scancode and lock-synchronization events), as these may be generated from
     * multiple user input threads concurrently.
     */
    pthread_mutex_t message_lock;

    /**
     * Lock which guards access to the keyboard state. Input handlers acquire a
     * read lock while translating and sending key events, while allocation and
     * teardown of the keyboard acquire a write lock.
     */
    pthread_rwlock_t keyboard_lock;

    /**
     * The SPICE cursor channel, providing the remote cursor shape. NULL until
     * the channel has been opened.
     */
    SpiceChannel* cursor_channel;

    /**
     * Lock which serializes scheduling of deferred calls (see
     * guac_spice_defer_call()) against the SPICE client thread marking the
     * event loop as finished, guaranteeing that every call scheduled before
     * loop_finished is set is drained by the client thread, and that every
     * call scheduled afterward is freed immediately.
     */
    pthread_mutex_t defer_lock;

    /**
     * Non-zero once the SPICE event loop has stopped and the session is
     * shutting down, zero otherwise. Once set, deferred calls are no longer
     * dispatched to their handlers (no further spice-gtk calls are made); they
     * are only freed. Written under defer_lock.
     */
    int loop_finished;

    /**
     * The GLib main context within which all SPICE event processing occurs.
     */
    GMainContext* main_context;

    /**
     * The GLib main loop driving SPICE event processing. spice-gtk is an
     * event-driven, signal-based library, so all channel I/O is dispatched by
     * running this loop within the client thread.
     */
    GMainLoop* main_loop;

    /**
     * Client settings, parsed from the arguments provided by the client upon
     * connection.
     */
    guac_spice_settings* settings;

    /**
     * The guac_display to which guacd renders the remote SPICE display, and
     * from which display updates are sent to all connected users.
     */
    guac_display* display;

    /**
     * The current instance of the guac_display render thread. If the thread
     * has not yet been started, this will be NULL.
     */
    guac_display_render_thread* render_thread;

    /**
     * Lock which synchronizes access to the primary surface metadata below,
     * which is updated from SPICE display channel signal handlers and read
     * while copying damaged regions into the guac_display.
     */
    pthread_mutex_t surface_lock;

    /**
     * Pointer to the primary surface buffer provided by the SPICE display
     * channel, or NULL if no primary surface currently exists.
     */
    void* surface_data;

    /**
     * The width of the primary surface, in pixels.
     */
    int surface_width;

    /**
     * The height of the primary surface, in pixels.
     */
    int surface_height;

    /**
     * The stride (number of bytes per row) of the primary surface.
     */
    int surface_stride;

    /**
     * The SPICE surface format (one of the SPICE_SURFACE_FMT_* constants) of
     * the primary surface.
     */
    int surface_format;

    /**
     * The currently negotiated mouse mode, one of SPICE_MOUSE_MODE_CLIENT or
     * SPICE_MOUSE_MODE_SERVER.
     */
    int mouse_mode;

    /**
     * The Guacamole button mask (bitwise OR of GUAC_CLIENT_MOUSE_*) as of the
     * most recently handled mouse event. Used to detect button press/release
     * transitions and, in SPICE server (relative) mouse mode, to compute
     * motion deltas.
     */
    int last_mouse_mask;

    /**
     * The X coordinate of the most recently handled mouse event.
     */
    int last_mouse_x;

    /**
     * The Y coordinate of the most recently handled mouse event.
     */
    int last_mouse_y;

    /*
     * Dynamic display-resize state. A client-requested resize is queued in
     * resize_pending_width/height and sent to the guest only once the SPICE
     * agent (advertising monitors-config support) and the display's primary
     * surface are both ready, which is the readiness spice-gtk itself requires
     * before a monitors config will actually be sent. Apart from the initial
     * size seeded by the connection owner before the SPICE client thread is
     * started, these fields are accessed only from the SPICE event-loop
     * thread.
     */

    /**
     * Non-zero if the SPICE guest agent is connected and advertises support
     * for monitors-config messages (VD_AGENT_CAP_MONITORS_CONFIG), zero
     * otherwise.
     */
    int resize_agent_ready;

    /**
     * Non-zero if the SPICE display channel currently has a primary surface,
     * such that a monitors config sent to the guest will take effect, zero
     * otherwise.
     */
    int resize_display_ready;

    /**
     * Non-zero if a client-requested display resize is queued and has not yet
     * been sent to the guest, zero otherwise.
     */
    int resize_pending;

    /**
     * The width, in pixels, of the most recently queued display resize.
     * Meaningful only while resize_pending is non-zero.
     */
    int resize_pending_width;

    /**
     * The height, in pixels, of the most recently queued display resize.
     * Meaningful only while resize_pending is non-zero.
     */
    int resize_pending_height;

    /**
     * The GLib source ID of the pending debounced monitors-config flush, or 0
     * if none is currently scheduled. Rapid client resize requests are
     * coalesced into a single guest monitors config, sent when this timer
     * fires, so that the guest's SPICE agent has time to apply each size
     * without being forced to reconfigure repeatedly. Accessed only from the
     * SPICE event-loop thread.
     */
    guint resize_timer_source;

    /**
     * The clipboard contents (data and mimetype) shared between the SPICE
     * guest and all connected Guacamole users. It holds the most recent
     * clipboard data received from either side: data received from the guest
     * is stored here and broadcast to all users, while data received from a
     * user is stored here and offered to the guest, which then requests it.
     * NULL if the clipboard has not been allocated.
     */
    guac_common_clipboard* clipboard;

    /**
     * Audio output stream, or NULL if audio is not enabled.
     */
    guac_audio_stream* audio;

    /**
     * The number of audio channels last reported by the SPICE playback
     * channel.
     */
    int audio_channels;

    /**
     * The sample rate (in Hz) last reported by the SPICE playback channel.
     */
    int audio_rate;

    /**
     * Whether audio playback is currently muted, as reported by the SPICE
     * playback channel. While set, received PCM data is not forwarded to the
     * Guacamole audio stream.
     */
    int audio_muted;

    /**
     * The SPICE record channel, used to forward audio (e.g. microphone) input
     * from the connection owner to the SPICE server. NULL until the channel
     * has been opened, or if audio input is not enabled.
     */
    SpiceRecordChannel* record_channel;

    /**
     * The Guacamole audio input stream over which the connection owner is
     * currently sending audio to be forwarded to the SPICE record channel, or
     * NULL if no such stream is active.
     */
    guac_stream* audio_input;

    /**
     * The sample rate (in Hz) the SPICE record channel expects, as reported by
     * the "record-start" signal. Zero until recording has started, and reset
     * to zero when recording stops. Guarded by message_lock.
     */
    int record_rate;

    /**
     * The number of audio channels the SPICE record channel expects, as
     * reported by the "record-start" signal. Zero until recording has
     * started, and reset to zero when recording stops. Guarded by
     * message_lock.
     */
    int record_channels;

    /**
     * The sample rate (in Hz) of the inbound audio stream, as advertised by
     * the connection owner in the audio input mimetype. Zero until an audio
     * input stream has been established.
     */
    int input_rate;

    /**
     * The number of audio channels of the inbound audio stream, as advertised
     * by the connection owner in the audio input mimetype. Zero until an
     * audio input stream has been established.
     */
    int input_channels;

    /**
     * The shared folder exposed to the connected user as a Guacamole filesystem
     * object (file browser) and to the SPICE server via the WebDAV channel, or
     * NULL if file transfer has not been enabled.
     */
    guac_spice_folder* shared_folder;

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

} guac_spice_client;

/**
 * A spice-gtk channel operation deferred to run on the SPICE event-loop thread.
 *
 * spice-gtk is not thread-safe: each channel's outbound messages are dispatched
 * by a coroutine scheduled on the GMainContext driven by guac_spice_client_thread.
 * Invoking channel functions (keyboard, mouse, clipboard) directly from
 * Guacamole user/handler threads races with that loop (via spice_channel_wakeup)
 * and can freeze the session. Such calls must therefore be marshalled onto the
 * loop thread using guac_spice_defer_call().
 */
typedef struct guac_spice_deferred_call {

    /**
     * Handler which performs the actual spice-gtk call. Invoked on the SPICE
     * event-loop thread.
     */
    void (*handler)(struct guac_spice_deferred_call* call);

    /**
     * The guac_client associated with the SPICE connection. Handlers that
     * operate on a SPICE channel must read the live channel pointer (such as
     * inputs_channel or main_channel) from the guac_spice_client on the
     * event-loop thread, and do nothing if it is NULL. Channel pointers are
     * never captured on the calling (user) thread, as the channel may be
     * destroyed before the call is dispatched.
     */
    guac_client* client;

    /**
     * Generic integer arguments for the deferred call. Signed values may be
     * stored here and recovered by casting back to int.
     */
    unsigned int args[5];

    /**
     * Optional heap-allocated payload (e.g. a copy of clipboard data or a types
     * array), or NULL. Freed automatically after the call has been dispatched,
     * or without dispatching if the SPICE event loop has finished, using
     * data_destroy if set and g_free() otherwise. A handler that takes
     * ownership of data must set this field to NULL to suppress the automatic
     * free.
     */
    gpointer data;

    /**
     * Optional destructor for data. When NULL, data is released with g_free().
     * Set this when data requires more than a flat free (e.g. owns nested
     * allocations, file descriptors, or temporary files).
     */
    GDestroyNotify data_destroy;

    /**
     * The length of data, in bytes.
     */
    gsize data_len;

} guac_spice_deferred_call;

/**
 * Schedules the given deferred spice-gtk call to run on the SPICE event-loop
 * thread. The call structure must be allocated with g_new0(); ownership is
 * transferred to this function, which frees both the structure and call->data
 * once the call has been dispatched. If the SPICE event loop has already
 * finished, the call is not dispatched and is freed immediately; calls still
 * queued when the loop finishes are freed (without being dispatched) when the
 * SPICE client thread drains the main context during shutdown. The call's
 * client member must be set.
 *
 * @param call
 *     The heap-allocated deferred call to schedule.
 */
void guac_spice_defer_call(guac_spice_deferred_call* call);

/**
 * SPICE client thread. This thread establishes the SPICE session and runs the
 * GLib main loop which drives all SPICE event processing for the duration of
 * the connection. It exists as a single instance, shared by all users.
 *
 * @param data
 *     The guac_client instance associated with the requested SPICE
 *     connection.
 *
 * @return
 *     Always NULL.
 */
void* guac_spice_client_thread(void* data);

#endif
