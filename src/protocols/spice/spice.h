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

#ifndef GUAC_SPICE_H
#define GUAC_SPICE_H

#include "config.h"

#include "channels/file.h"
#include "common/clipboard.h"
#include "common/display.h"
#include "common/iconv.h"
#include "common/surface.h"
#include "keyboard.h"
#include "settings.h"

#include <guacamole/audio.h>
#include <guacamole/client.h>
#include <guacamole/layer.h>
#include <guacamole/recording.h>
#include <glib.h>
#include <spice-client-glib-2.0/spice-client.h>

#ifdef ENABLE_COMMON_SSH
#include "common-ssh/sftp.h"
#include "common-ssh/ssh.h"
#include "common-ssh/user.h"
#endif

#include <pthread.h>

/**
 * Spice-specific client data.
 */
typedef struct guac_spice_client {

    /**
     * The Spice client thread.
     */
    pthread_t client_thread;

    /**
     * The underlying Spice session.
     */
    SpiceSession* spice_session;
    
    /**
     * The main Spice channel.
     */
    SpiceMainChannel* main_channel;
    
    /**
     * The Spice audio playback channel.
     */
    SpicePlaybackChannel* playback_channel;
    
    /**
     * The Spice audio recording/input channel.
     */
    SpiceRecordChannel* record_channel;
    
    /**
     * The Spice channel that handles the cursor display and events.
     */
    SpiceCursorChannel* cursor_channel;
    
    /**
     * The Spice channel that handles mouse and keyboard inputs.
     */
    SpiceInputsChannel* inputs_channel;

    /**
     * Client settings, parsed from args.
     */
    guac_spice_settings* settings;

    /**
     * The current display state.
     */
    guac_common_display* display;
    
    /**
     * The Spice display channel.
     */
    SpiceDisplayChannel* spice_display;

    /**
     * The current state of the keyboard with respect to the RDP session.
     */
    guac_spice_keyboard* keyboard;

    /**
     * The glib main loop
     */
    GMainLoop* spice_mainloop;

    /**
     * Internal clipboard.
     */
    guac_common_clipboard* clipboard;

    /**
     * Shared folder.
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

    /**
     * Common attributes for locks.
     */
    pthread_mutexattr_t attributes;

    /**
     * Lock which is used to synchronizes access to Spice data structures
     * between user input and client threads. It prevents input handlers
     * from running when Spice data structures are allocated or freed
     * by the client thread.
     */
    pthread_rwlock_t lock;

    /**
     * Lock which synchronizes the sending of each Spice message, ensuring
     * attempts to send Spice messages never overlap.
     */
    pthread_mutex_t message_lock;

    /**
     * Audio output stream, if any.
     */
    guac_audio_stream* audio_playback;

    /**
     * Audio input stream, if any.
     */
    guac_stream* audio_input;

} guac_spice_client;

/**
 * Allocates a new Spice client session given the parameters stored within the
 * client, returning NULL on failure.
 *
 * @param client
 *     The guac_client associated with the settings of the desired Spice
 *     connection.
 *
 * @return
 *     A new Spice session instance allocated and connected according to the
 *     parameters stored within the given client, or NULL if connecting to the
 *     Spice server fails.
 */
SpiceSession* guac_spice_get_session(guac_client* client);

/**
 * Spice client thread. This thread initiates the Spice connection and
 * ultimately runs throughout the duration of the client, existing as a single
 * instance, shared by all users.
 *
 * @param data
 *     The guac_client instance associated with the requested Spice connection.
 *
 * @return
 *     Always NULL.
 */
void* guac_spice_client_thread(void* data);

#endif /* GUAC_SPICE_H */

