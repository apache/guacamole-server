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


#ifndef _SSH_GUAC_CLIENT_H
#define _SSH_GUAC_CLIENT_H

#include "config.h"

#include "guac_ssh.h"
#include "guac_ssh_user.h"
#include "sftp.h"
#include "terminal.h"

#include <libssh2.h>
#include <libssh2_sftp.h>

#include <guacamole/object.h>

#ifdef ENABLE_SSH_AGENT
#include "ssh_agent.h"
#endif

#include <pthread.h>
#include <stdbool.h>

/**
 * SSH-specific client data.
 */
typedef struct ssh_guac_client_data {

    /**
     * The hostname of the SSH server to connect to.
     */
    char hostname[1024];

    /**
     * The port of the SSH server to connect to.
     */
    char port[64];

    /**
     * The name of the user to login as.
     */
    char username[1024];

    /**
     * The password to give when authenticating.
     */
    char password[1024];

    /**
     * The private key, encoded as base64.
     */
    char key_base64[4096];

    /**
     * The password to use to decrypt the given private key.
     */
    char key_passphrase[1024];

    /**
     * The name of the font to use for display rendering.
     */
    char font_name[1024];

    /**
     * The size of the font to use, in points.
     */
    int font_size;

    /**
     * Whether SFTP is enabled.
     */
    bool enable_sftp;

#ifdef ENABLE_SSH_AGENT
    /**
     * Whether the SSH agent is enabled.
     */
    bool enable_agent;

    /**
     * The current agent, if any.
     */
    ssh_auth_agent* auth_agent;
#endif

    /**
     * The SSH client thread.
     */
    pthread_t client_thread;

    /**
     * The user and credentials to use for all SSH sessions.
     */
    guac_common_ssh_user* user;

    /**
     * SSH session, used by the SSH client thread.
     */
    guac_common_ssh_session* session;

    /**
     * SFTP session, used by the SFTP client/filesystem.
     */
    guac_common_ssh_session* sftp_session;

    /**
     * The filesystem object exposed for the SFTP session.
     */
    guac_object* sftp_filesystem;

    /**
     * SSH terminal channel, used by the SSH client thread.
     */
    LIBSSH2_CHANNEL* term_channel;

    /**
     * Lock dictating access to the SSH terminal channel.
     */
    pthread_mutex_t term_channel_lock;

    /**
     * The terminal which will render all output from the SSH client.
     */
    guac_terminal* term;
   
} ssh_guac_client_data;

#endif

