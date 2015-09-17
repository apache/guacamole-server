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


#ifndef __GUAC_VNC_CLIENT_H
#define __GUAC_VNC_CLIENT_H

#include "config.h"
#include "guac_clipboard.h"
#include "guac_surface.h"
#include "guac_iconv.h"

#include <guacamole/audio.h>
#include <guacamole/layer.h>
#include <rfb/rfbclient.h>

#ifdef ENABLE_PULSE
#include <pulse/pulseaudio.h>
#endif

#ifdef ENABLE_COMMON_SSH
#include "guac_sftp.h"
#include "guac_ssh.h"
#include "guac_ssh_user.h"
#endif

/**
 * The maximum duration of a frame in milliseconds.
 */
#define GUAC_VNC_FRAME_DURATION 40

/**
 * The amount of time to allow per message read within a frame, in
 * milliseconds. If the server is silent for at least this amount of time, the
 * frame will be considered finished.
 */
#define GUAC_VNC_FRAME_TIMEOUT 10

/**
 * The number of milliseconds to wait between connection attempts.
 */
#define GUAC_VNC_CONNECT_INTERVAL 1000

/**
 * The maximum number of bytes to allow within the clipboard.
 */
#define GUAC_VNC_CLIPBOARD_MAX_LENGTH 262144

extern char* __GUAC_CLIENT;

/**
 * VNC-specific client data.
 */
typedef struct vnc_guac_client_data {

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
     * The hostname of the VNC server (or repeater) to connect to.
     */
    char* hostname;

    /**
     * The port of the VNC server (or repeater) to connect to.
     */
    int port;

    /**
     * The password given in the arguments.
     */
    char* password;

    /**
     * Space-separated list of encodings to use within the VNC session.
     */
    char* encodings;

    /**
     * Whether the red and blue components of each color should be swapped.
     * This is mainly used for VNC servers that do not properly handle
     * colors.
     */
    int swap_red_blue;

    /**
     * The color depth to request, in bits.
     */
    int color_depth;

    /**
     * Whether this connection is read-only, and user input should be dropped.
     */
    int read_only;

    /**
     * The VNC host to connect to, if using a repeater.
     */
    char* dest_host;

    /**
     * The VNC port to connect to, if using a repeater.
     */
    int dest_port;

#ifdef ENABLE_VNC_LISTEN
    /**
     * Whether not actually connecting to a VNC server, but rather listening
     * for a connection from the VNC server (reverse connection).
     */
    int reverse_connect;

    /**
     * The maximum amount of time to wait when listening for connections, in
     * milliseconds.
     */
    int listen_timeout;
#endif

    /**
     * Whether the cursor should be rendered on the server (remote) or on the
     * client (local).
     */
    int remote_cursor;

    /**
     * The layer holding the cursor image.
     */
    guac_layer* cursor;
    
    /**
     * Whether audio is enabled.
     */
    int audio_enabled;
    
    /**
     * Audio output, if any.
     */
    guac_audio_stream* audio;

#ifdef ENABLE_PULSE
    /**
     * The name of the PulseAudio server to connect to.
     */
    char* pa_servername;

    /**
     * PulseAudio event loop.
     */
    pa_threaded_mainloop* pa_mainloop;
#endif

    /**
     * Internal clipboard.
     */
    guac_common_clipboard* clipboard;

    /**
     * Default surface.
     */
    guac_common_surface* default_surface;

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
     * The exposed filesystem object, implemented with SFTP.
     */
    guac_object* sftp_filesystem;
#endif

    /**
     * Clipboard encoding-specific reader.
     */
    guac_iconv_read* clipboard_reader;

    /**
     * Clipboard encoding-specific writer.
     */
    guac_iconv_write* clipboard_writer;

} vnc_guac_client_data;

#endif

