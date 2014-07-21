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


#ifndef __GUAC_RDP_SETTINGS_H
#define __GUAC_RDP_SETTINGS_H

#include "config.h"

#include "rdp_keymap.h"

#include <freerdp/freerdp.h>

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
 * All supported combinations of security types.
 */
typedef enum guac_rdp_security {

    /**
     * Standard RDP encryption.
     */
    GUAC_SECURITY_RDP,

    /**
     * TLS encryption.
     */
    GUAC_SECURITY_TLS,

    /**
     * Network level authentication.
     */
    GUAC_SECURITY_NLA,

    /**
     * Any method supported by the server.
     */
    GUAC_SECURITY_ANY

} guac_rdp_security;

/**
 * All settings supported by the Guacamole RDP client.
 */
typedef struct guac_rdp_settings {

    /**
     * The hostname to connect to.
     */
    char* hostname;

    /**
     * The port to connect to.
     */
    int port;

    /**
     * The domain of the user logging in.
     */
    char* domain;

    /**
     * The username of the user logging in.
     */
    char* username;

    /**
     * The password of the user logging in.
     */
    char* password;

    /**
     * The color depth of the display to request, in bits.
     */
    int color_depth;

    /**
     * The width of the display to request, in pixels.
     */
    int width;
    
    /**
     * The height of the display to request, in pixels.
     */
    int height;

    /**
     * Whether audio is enabled.
     */
    int audio_enabled;

    /**
     * Whether printing is enabled.
     */
    int printing_enabled;

    /**
     * Whether the virtual drive is enabled.
     */
    int drive_enabled;

    /**
     * The local system path which will be used to persist the
     * virtual drive.
     */
    char* drive_path;

    /**
     * Whether this session is a console session.
     */
    int console;

    /**
     * Whether to allow audio in the console session.
     */
    int console_audio;

    /**
     * The keymap chosen as the layout of the server.
     */
    const guac_rdp_keymap* server_layout;

    /**
     * The initial program to run, if any.
     */
    char* initial_program;

    /**
     * The type of security to use for the connection.
     */
    guac_rdp_security security_mode;

    /**
     * Whether bad server certificates should be ignored.
     */
    int ignore_certificate;

    /**
     * Whether authentication should be disabled. This is different from the
     * authentication that takes place when a user provides their username
     * and password. Authentication is required by definition for NLA.
     */
    int disable_authentication;

    /**
     * The application to launch, if RemoteApp is in use.
     */
    char* remote_app;

    /**
     * The working directory of the remote application, if RemoteApp is in use.
     */
    char* remote_app_dir;

    /**
     * The arguments to pass to the remote application, if RemoteApp is in use.
     */
    char* remote_app_args;

    /**
     * NULL-terminated list of all static virtual channel names, or NULL if
     * no channels whatsoever.
     */
    char** svc_names;

} guac_rdp_settings;

/**
 * Save all given settings to the given freerdp instance.
 */
void guac_rdp_push_settings(guac_rdp_settings* guac_settings, freerdp* rdp);

/**
 * Returns the width of the RDP session display.
 */
int guac_rdp_get_width(freerdp* rdp);

/**
 * Returns the height of the RDP session display.
 */
int guac_rdp_get_height(freerdp* rdp);

/**
 * Returns the depth of the RDP session display.
 */
int guac_rdp_get_depth(freerdp* rdp);

#endif

