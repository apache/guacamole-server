
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
 * The Original Code is libguac-client-vnc.
 *
 * The Initial Developer of the Original Code is
 * Michael Jumper.
 * Portions created by the Initial Developer are Copyright (C) 2010
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

#ifndef __GUAC_VNC_CLIENT_H
#define __GUAC_VNC_CLIENT_H

#include <guacamole/client.h>
#include <guacamole/audio.h>
#include <rfb/rfbclient.h>

#ifdef ENABLE_PULSE
#include <pulse/pulseaudio.h>
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
#define GUAC_VNC_FRAME_TIMEOUT 0

/**
 * The number of milliseconds to wait between connection attempts.
 */
#define GUAC_VNC_CONNECT_INTERVAL 1000

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
    
} vnc_guac_client_data;

#endif

