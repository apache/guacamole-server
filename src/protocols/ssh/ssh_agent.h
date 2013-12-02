
#ifndef _GUAC_SSH_AGENT_H
#define _GUAC_SSH_AGENT_H

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
 * Auth agent channel thread.
 */
void* auth_agent_read_thread(void* arg);

/**
 * Libssh2 callback, invoked when the auth agent channel is opened.
 */
void ssh_auth_agent_callback(LIBSSH2_SESSION *session,
        LIBSSH2_CHANNEL *channel, void **abstract);

#endif

