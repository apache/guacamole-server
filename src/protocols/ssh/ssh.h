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

#ifndef GUAC_SSH_H
#define GUAC_SSH_H

#include "config.h"

#include "guac_sftp.h"
#include "guac_ssh.h"
#include "guac_ssh_user.h"
#include "settings.h"
#include "terminal.h"

#ifdef ENABLE_SSH_AGENT
#include "ssh_agent.h"
#endif

#include <guacamole/client.h>

#include <pthread.h>

/**
 * SSH-specific client data.
 */
typedef struct guac_ssh_client {

    /**
     * SSH connection settings.
     */
    guac_ssh_settings* settings;

#ifdef ENABLE_SSH_AGENT
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
    guac_common_ssh_sftp_filesystem* sftp_filesystem;

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
   
} guac_ssh_client ;

/**
 * Main SSH client thread, handling transfer of SSH output to STDOUT.
 */
void* ssh_client_thread(void* data);

#endif

