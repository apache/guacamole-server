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

#ifndef GUAC_COMMON_SSH_TUNNEL_H
#define GUAC_COMMON_SSH_TUNNEL_H

#include "common-ssh/ssh.h"

#include <libssh2.h>
#include <pthread.h>

/**
 * Default backlog size for the socket used for the SSH tunnel.
 */
#define GUAC_COMMON_SSH_TUNNEL_BACKLOG_SIZE 8

/**
 * The default directory mode that will be used to create the directory that
 * will store the sockets.
 */
#define GUAC_COMMON_SSH_TUNNEL_DIRECTORY_MODE 0700

/**
 * The default mode of the file that will be used to access the UNIX domain
 * socket.
 */
#define GUAC_COMMON_SSH_TUNNEL_SOCKET_MODE 0600

/**
 * A data structure that contains the elements needed to be passed between
 * the various Guacamole Client protocol implementations and the common SSH
 * tunnel code.
 */
typedef struct guac_ssh_tunnel {

    /**
     * The Guacamole Client that is using this SSH tunnel.
     */
    guac_client* client;

    /**
     * The user and credentials for authenticating the SSH tunnel.
     */
    guac_common_ssh_user* user;

    /**
     * The SSH session to use to tunnel the data.
     */
    guac_common_ssh_session* session;

    /**
     * The libssh2 channel that will carry the tunnel data over the SSH connection.
     */
    LIBSSH2_CHANNEL *channel;

    /**
     * The path to the local socket that will be used by guacd to communicate
     * with the SSH tunnel.
     */
    char* socket_path;

} guac_ssh_tunnel;

/**
 * Initialize the SSH tunnel to the given host and port combination through
 * the provided SSH session, and open a socket at the specified path for the
 * communication. This function will place the absolute path of the domain
 * socket in the socket_path variable and return zero on success or non-zero
 * on failure.
 *
 * @param ssh_tunnel
 *     The data structure containing relevant SSH tunnel information, including
 *     the guac_client that initialized the tunnel and the various libssh2
 *     session and channel objects.
 *
 * @param host
 *     The hostname or IP address to connect to over the tunnel.
 *
 * @param port
 *     The TCP port to connect to over the tunnel.
 *
 * @return
 *     Zero on success, non-zero on failure.
 */
int guac_common_ssh_tunnel_init(guac_ssh_tunnel* ssh_tunnel,
                                  char* remote_host,
                                  int remote_port);

/**
 * Clean up the SSH tunnel, shutting down the channel and freeing the
 * various data items created for the tunnel.
 *
 * @param ssh_tunnel
 *     The guac_common_ssh_session used to establish the tunnel.
 *
 * @return
 *     Zero on success, non-zero on failure.
 */
int guac_common_ssh_tunnel_cleanup(guac_ssh_tunnel* ssh_tunnel);

#endif