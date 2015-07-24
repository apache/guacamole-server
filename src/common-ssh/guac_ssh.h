/*
 * Copyright (C) 2015 Glyptodon LLC
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

#ifndef GUAC_COMMON_SSH_H
#define GUAC_COMMON_SSH_H

#include "guac_ssh_user.h"

#include <guacamole/client.h>
#include <libssh2.h>

/**
 * An SSH session, backed by libssh2 and associated with a particular
 * Guacamole client.
 */
typedef struct guac_common_ssh_session {

    /**
     * The Guacamole client using this SSH session.
     */
    guac_client* client;

    /**
     * The user that will be authenticating via SSH.
     */
    guac_common_ssh_user* user;

    /**
     * The underlying SSH session from libssh2.
     */
    LIBSSH2_SESSION* session;

    /**
     * The file descriptor of the socket being used for the SSH connection.
     */
    int fd;

} guac_common_ssh_session;

/**
 * Initializes the underlying SSH and encryption libraries used by Guacamole.
 * This function must be called before any other guac_common_ssh_*() functions
 * are called.
 *
 * @param client
 *     The Guacamole client that will be using SSH.
 *
 * @return
 *     Zero if initialization, or non-zero if an error occurs.
 */
int guac_common_ssh_init(guac_client* client);

/**
 * Cleans up the underlying SSH and encryption libraries used by Guacamole.
 * This function must be called once no other guac_common_ssh_*() functions
 * will be used.
 */
void guac_common_ssh_uninit();

/**
 * Connects to the SSH server running at the given hostname and port, and
 * authenticates as the given user. If an error occurs while connecting or
 * authenticating, the Guacamole client will automatically and fatally abort.
 * The user object provided must eventually be explicitly destroyed, but should
 * not be destroyed until this session is destroyed, assuming the session is
 * successfully created.
 *
 * @param client
 *     The Guacamole client that will be using SSH.
 *
 * @param hostname
 *     The hostname of the SSH server to connect to.
 *
 * @param port
 *     The port to connect to on the given hostname.
 *
 * @param user
 *     The user to authenticate as, once connected.
 *
 * @return
 *     A new SSH session if the connection and authentication succeed, or NULL
 *     if the connection or authentication were not successful.
 */
guac_common_ssh_session* guac_common_ssh_create_session(guac_client* client,
        const char* hostname, const char* port, guac_common_ssh_user* user);

/**
 * Disconnects and destroys the given SSH session, freeing all associated
 * resources. Any associated user must be explicitly destroyed, and will not
 * be destroyed automatically.
 *
 * @param session
 *     The SSH session to destroy.
 */
void guac_common_ssh_destroy_session(guac_common_ssh_session* session);

#endif

