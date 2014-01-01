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


#ifndef _GUAC_SSH_AGENT_H
#define _GUAC_SSH_AGENT_H

#include "config.h"

#include "ssh_key.h"

/**
 * Packet type of an agent identity request.
 */
#define SSH2_AGENT_REQUEST_IDENTITIES 0x0B

/**
 * Packet type of an agent identity response.
 */
#define SSH2_AGENT_IDENTITIES_ANSWER 0x0C

/**
 * Packet type of an agent sign request.
 */
#define SSH2_AGENT_SIGN_REQUEST 0x0D

/**
 * Packet type of an agent sign response.
 */
#define SSH2_AGENT_SIGN_RESPONSE 0x0E

/**
 * The comment to associate with public keys when listed.
 */
#define SSH_AGENT_COMMENT "Guacamole SSH Agent"

/**
 * The packet sent by the SSH agent when an operation is not supported.
 */
#define UNSUPPORTED "\x00\x00\x00\x0C\x05Unsupported"

/**
 * Data representing an SSH auth agent.
 */
typedef struct ssh_auth_agent {

    /**
     * The SSH channel being used for SSH agent protocol.
     */
    LIBSSH2_CHANNEL* channel;

    /**
     * The single private key to use for authentication.
     */
    ssh_key* identity;

    /**
     * Data read from the agent channel.
     */
    char buffer[4096];

    /**
     * The number of bytes of data currently stored in the buffer.
     */
    int buffer_length;

} ssh_auth_agent;

/**
 * Handler for an agent sign request.
 */
void ssh_auth_agent_sign(ssh_auth_agent* auth_agent,
        char* data, int data_length);

/**
 * Handler for an agent identity request.
 */
void ssh_auth_agent_list_identities(ssh_auth_agent* auth_agent);

/**
 * Generic handler for all packets received over the auth agent channel.
 */
void ssh_auth_agent_handle_packet(ssh_auth_agent* auth_agent,
        uint8_t type, char* data, int data_length);

/**
 * Reads and handles a single packet from the SSH agent channel associated
 * with the given ssh_auth_agent, returning the size of that packet, the size
 * of the partial packet read, or a negative value if an error occurs.
 */
int ssh_auth_agent_read(ssh_auth_agent* auth_agent);

/**
 * Libssh2 callback, invoked when the auth agent channel is opened.
 */
void ssh_auth_agent_callback(LIBSSH2_SESSION *session,
        LIBSSH2_CHANNEL *channel, void **abstract);

#endif

