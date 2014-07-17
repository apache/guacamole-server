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

#ifndef GUAC_TELNET__CLIENT_H
#define GUAC_TELNET__CLIENT_H

#include "config.h"
#include "terminal.h"

#include <pthread.h>
#include <regex.h>
#include <sys/types.h>

#include <libtelnet.h>

#define GUAC_TELNET_DEFAULT_USERNAME_REGEX "[Ll]ogin:"
#define GUAC_TELNET_DEFAULT_PASSWORD_REGEX "[Pp]assword:"

/**
 * Telnet-specific client data.
 */
typedef struct guac_telnet_client_data {

    /**
     * The hostname of the telnet server to connect to.
     */
    char hostname[1024];

    /**
     * The port of the telnet server to connect to.
     */
    char port[64];

    /**
     * The name of the user to login as.
     */
    char username[1024];

    /**
     * The regular expression to use when searching for the username
     * prompt. This will be NULL unless the telnet client is currently
     * searching for the username prompt.
     */
    regex_t* username_regex;

    /**
     * The password to give when authenticating.
     */
    char password[1024];

    /**
     * The regular expression to use when searching for the password
     * prompt. This will be NULL unless the telnet client is currently
     * searching for the password prompt.
     */
    regex_t* password_regex;

    /**
     * The name of the font to use for display rendering.
     */
    char font_name[1024];

    /**
     * The size of the font to use, in points.
     */
    int font_size;

    /**
     * The telnet client thread.
     */
    pthread_t client_thread;

    /**
     * The file descriptor of the socket connected to the telnet server,
     * or -1 if no connection has been established.
     */
    int socket_fd;

    /**
     * Telnet connection, used by the telnet client thread.
     */
    telnet_t* telnet;

    /**
     * Whether window size should be sent when the window is resized.
     */
    int naws_enabled;

    /**
     * Whether all user input should be automatically echoed to the
     * terminal.
     */
    int echo_enabled;

    /**
     * The terminal which will render all output from the telnet client.
     */
    guac_terminal* term;
   
} guac_telnet_client_data;

#endif

