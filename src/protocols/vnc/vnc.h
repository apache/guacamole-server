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

#ifndef GUAC_VNC_VNC_H
#define GUAC_VNC_VNC_H

#include "config.h"

#include "common/clipboard.h"
#include "common/display.h"
#include "common/iconv.h"
#include "common/recording.h"
#include "common/surface.h"
#include "settings.h"

#include <guacamole/client.h>
#include <guacamole/layer.h>
#include <rfb/rfbclient.h>

#ifdef ENABLE_PULSE
#include "pulse/pulse.h"
#endif

#ifdef ENABLE_COMMON_SSH
#include "common-ssh/sftp.h"
#include "common-ssh/ssh.h"
#include "common-ssh/user.h"
#endif

#include <pthread.h>

/**
 * VNC-specific client data.
 */
typedef struct guac_vnc_client {

    /**
     * The VNC client thread.
     */
    pthread_t client_thread;

#ifdef ENABLE_VNC_TLS_LOCKING
    /**
     * The TLS mutex lock for the client.
     */
    pthread_mutex_t tls_lock;
#endif

    /**
     * The underlying VNC client.
     */
    rfbClient* rfb_client;

    /**
     * The original framebuffer malloc procedure provided by the initialized
     * rfbClient.
     */
    MallocFrameBufferProc rfb_MallocFrameBuffer;

    /**
     * Whether copyrect  was used to produce the latest update received
     * by the VNC server.
     */
    int copy_rect_used;

    /**
     * Client settings, parsed from args.
     */
    guac_vnc_settings* settings;

    /**
     * The current display state.
     */
    guac_common_display* display;

    /**
     * Internal clipboard.
     */
    guac_common_clipboard* clipboard;

#ifdef ENABLE_PULSE
    /**
     * PulseAudio output, if any.
     */
    guac_pa_stream* audio;
#endif

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
    guac_common_recording* recording;

    /**
     * Clipboard encoding-specific reader.
     */
    guac_iconv_read* clipboard_reader;

    /**
     * Clipboard encoding-specific writer.
     */
    guac_iconv_write* clipboard_writer;

} guac_vnc_client;

/**
 * Allocates a new rfbClient instance given the parameters stored within the
 * client, returning NULL on failure.
 *
 * @param client
 *     The guac_client associated with the settings of the desired VNC
 *     connection.
 *
 * @return
 *     A new rfbClient instance allocated and connected according to the
 *     parameters stored within the given client, or NULL if connecting to the
 *     VNC server fails.
 */
rfbClient* guac_vnc_get_client(guac_client* client);

/**
 * VNC client thread. This thread initiates the VNC connection and ultimately
 * runs throughout the duration of the client, existing as a single instance,
 * shared by all users.
 *
 * @param data
 *     The guac_client instance associated with the requested VNC connection.
 *
 * @return
 *     Always NULL.
 */
void* guac_vnc_client_thread(void* data);

/**
 * Key which can be used with the rfbClientGetClientData function to return
 * the associated guac_client.
 */
extern char* GUAC_VNC_CLIENT_KEY;

#endif

