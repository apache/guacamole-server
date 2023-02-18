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

#include "common-ssh/ssh.h"
#include "common-ssh/tunnel.h"

#include <guacamole/mem.h>
#include <guacamole/string.h>

#include <errno.h>
#include <libgen.h>
#include <libssh2.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

/**
 * A collection of the data required to establish the tunnel connection to the
 * remote host over SSH and pass data between threads.
 */
typedef struct ssh_tunnel_data {

    /**
     * The SSH tunnel.
     */
    guac_ssh_tunnel* ssh_tunnel;

    /**
     * The thread used to run the main tunnel worker.
     */
    pthread_t tunnel_thread;

    /**
     * The thread used to run the tunnel input worker.
     */
    pthread_t tunnel_input_thread;

    /**
     * A lock used to manage concurrent access to the libsh2 channel.
     */
    pthread_mutex_t tunnel_channel_lock;

    /**
     * The file descriptor of the socket that guacd will listen on to start
     * the tunnel to the remote system.
     */
    int listen_socket;

    /**
     * The file descriptor of the socket that will be used to read and write
     * data to the remote tunnel.
     */
    int tunnel_socket;

    /**
     * The UNIX address family data structure.
     */
    struct sockaddr_un tunnel_addr;

    /**
     * The hostname or IP address of the remote host.
     */
    char* remote_host;

    /**
     * The TCP port to connect to on the remote host.
     */
    int remote_port;

} ssh_tunnel_data;

/**
 * A function called by pthread_create that will be the worker function for
 * incoming data on the SSH tunnel.
 *
 * @param data
 *     A pointer to the ssh_tunnel_parameters structure that contains the data
 *     required to pass data from the local system over the tunnel to the remote
 *     SSH server.
 */
static void* guac_common_ssh_tunnel_input_worker(void* data) {

    ssh_tunnel_data* tunnel_data = (ssh_tunnel_data*) data;

    char buffer[8192];
    int bytes_read;
    int retval = 0;

    guac_client_log(tunnel_data->ssh_tunnel->client, GUAC_LOG_DEBUG,
            "Waiting for data on socket.");

    /* Read data from the socket and write it to the channel. */
    while (true) {
        bytes_read = read(tunnel_data->tunnel_socket, buffer, sizeof(buffer));
        if (bytes_read <= 0)
            break;

        pthread_mutex_lock(&(tunnel_data->tunnel_channel_lock));
        libssh2_channel_write(tunnel_data->ssh_tunnel->channel, buffer, bytes_read);
        pthread_mutex_unlock(&(tunnel_data->tunnel_channel_lock));
    }

    guac_client_log(tunnel_data->ssh_tunnel->client, GUAC_LOG_DEBUG,
            "Finished reading from socket, exiting.");

    pthread_exit(&retval);
    return NULL;

}

/**
 * A function passed to phtread_create that will be the worker function for
 * the SSH tunnel. The data passed should be a ssh_tunnel_parameters structure
 * that contains all of the information this function will need to start
 * the remote connection and process the data. Note that the socket passed
 * via this data structure should already be in the LISTENING state by the
 * time this function is called, and this function will wait for and accept
 * a connection on the socket in order to start the process of connecting to
 * the remote host over the tunnel and pass data.
 *
 * @param data
 *     A pointer to a ssh_tunnel_parameters structure that contains the data
 *     required to establish the connection over the SSH tunnel.
 */
static void* guac_common_ssh_tunnel_worker(void* data) {

    ssh_tunnel_data* tunnel_data = (ssh_tunnel_data*) data;
    int bytes_read, bytes_written, bytes_current;
    int retval = 0;
    char buffer[8192];
    fd_set fds;
    struct timeval tv;

    guac_client_log(tunnel_data->ssh_tunnel->client, GUAC_LOG_DEBUG,
            "Starting tunnel worker - waiting for connection.");

    /* Wait for a connection on the listening socket and process if we get it. */
    socklen_t addr_len = sizeof(tunnel_data->tunnel_addr);
    tunnel_data->tunnel_socket = accept(tunnel_data->listen_socket,
                                        (struct sockaddr*)(&(tunnel_data->tunnel_addr)),
                                        &addr_len);
    if (tunnel_data->tunnel_socket < 0) {
        pthread_exit(&retval);
        return NULL;
    }

    guac_client_log(tunnel_data->ssh_tunnel->client, GUAC_LOG_DEBUG,
            "Connection received, starting libssh2 channel.");

    /* Get the libssh2 Direct TCP/IP channel. */
    tunnel_data->ssh_tunnel->channel = libssh2_channel_direct_tcpip(
                                        tunnel_data->ssh_tunnel->session->session,
                                        tunnel_data->remote_host,
                                        tunnel_data->remote_port);

    if (!tunnel_data->ssh_tunnel->channel) {
        pthread_exit(&retval);
        return NULL;
    }

    guac_client_log(tunnel_data->ssh_tunnel->client,
                    GUAC_LOG_DEBUG,
                    "Channel started, starting output thread.");

    /* Turn off blocking on the socket, and start the input thread. */
    libssh2_session_set_blocking(tunnel_data->ssh_tunnel->session->session, 0);
    pthread_create(&(tunnel_data->tunnel_input_thread), NULL,
                   guac_common_ssh_tunnel_input_worker, (void *) tunnel_data);

    guac_client_log(tunnel_data->ssh_tunnel->client, GUAC_LOG_DEBUG,
            "Processing tunnel data.");

    /* Loop to process the data. */
    while (true) {
        FD_ZERO(&fds);
        FD_SET(tunnel_data->tunnel_socket, &fds);
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        retval = select(tunnel_data->tunnel_socket, &fds, NULL, NULL, &tv);

        if (retval < 0) {
            guac_client_log(tunnel_data->ssh_tunnel->client, GUAC_LOG_ERROR,
                    "Error receiving data from socket.");
            pthread_exit(&retval);
            return NULL;
        }

        pthread_mutex_lock(&(tunnel_data->tunnel_channel_lock));
        guac_client_log(tunnel_data->ssh_tunnel->client, GUAC_LOG_TRACE,
                "Lock acquired, reading data from channel.");
        bytes_read = libssh2_channel_read(tunnel_data->ssh_tunnel->channel,
                buffer, sizeof(buffer));
        pthread_mutex_unlock(&(tunnel_data->tunnel_channel_lock));
        
        /* No data read from the channel, skip the rest of the loop. */
        if (bytes_read == LIBSSH2_ERROR_EAGAIN)
            continue;

        if (bytes_read < 0) {
            guac_client_log(tunnel_data->ssh_tunnel->client, GUAC_LOG_ERROR,
                    "Error reading from libssh2 channel, giving up.");
            pthread_exit(&retval);
            return NULL;
        }

        bytes_written = 0;
        while (bytes_written < bytes_read) {
            guac_client_log(tunnel_data->ssh_tunnel->client, GUAC_LOG_TRACE,
                    "Writing channel data to socket.");
            bytes_current = send(tunnel_data->tunnel_socket,
                    buffer + bytes_written, bytes_read - bytes_written, 0);

            if (bytes_current <= 0) {
                guac_client_log(tunnel_data->ssh_tunnel->client, GUAC_LOG_ERROR,
                        "Error writing to socket, ending thread.");
                pthread_exit(&retval);
                return NULL;
            }
            bytes_written += bytes_current;
        }

        if (libssh2_channel_eof(tunnel_data->ssh_tunnel->channel)) {
            guac_client_log(tunnel_data->ssh_tunnel->client, GUAC_LOG_ERROR,
                    "Received eof on libssh2 channel, giving up.");
            pthread_exit(&retval);
            return NULL;
        }

    }

    guac_client_log(tunnel_data->ssh_tunnel->client, GUAC_LOG_DEBUG,
            "Waiting for input thread to exit.");

    /* Error or closed socket - wait for the input thread to exit. */
    pthread_join(tunnel_data->tunnel_input_thread, NULL);
    
    /* Close file descriptors and free data. */
    close(tunnel_data->tunnel_socket);
    close(tunnel_data->listen_socket);
    guac_mem_free(tunnel_data->remote_host);
    return NULL;

}

int guac_common_ssh_tunnel_init(guac_ssh_tunnel* ssh_tunnel, char* remote_host,
                                int remote_port) {

    struct stat socket_stat;
    
    ssh_tunnel_data* tunnel_data = calloc(1, sizeof(ssh_tunnel_data));
    tunnel_data->ssh_tunnel = ssh_tunnel;
    tunnel_data->remote_host = guac_strdup(remote_host);
    tunnel_data->remote_port = remote_port;

    /* Assemble the expected path to the socket. */
    ssh_tunnel->socket_path = malloc(4096);
    snprintf(ssh_tunnel->socket_path, 4096, "%s/%s/tunnel",
            GUACD_STATE_DIR, ssh_tunnel->client->connection_id);

    guac_client_log(ssh_tunnel->client, GUAC_LOG_DEBUG,
            "Socket: %s", ssh_tunnel->socket_path);
    const char* socket_dir = dirname(guac_strdup(ssh_tunnel->socket_path));

    /* Check if the socket already exists, and abort if it does. */
    if (stat((const char *)ssh_tunnel->socket_path, &socket_stat) == 0) {
        guac_client_abort(ssh_tunnel->client, GUAC_PROTOCOL_STATUS_RESOURCE_CONFLICT,
                "Socket already exists: %s", ssh_tunnel->socket_path);
        return -1;
    }
 
    /* Create the directory and the socket. */
    if (mkdir(socket_dir, GUAC_COMMON_SSH_TUNNEL_DIRECTORY_MODE)) {
        guac_client_abort(ssh_tunnel->client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "Failed to make socket directory \"%s\": %s", socket_dir,
                strerror(errno));
        return -1;

    }

    /* Set up the socket and listen on it. */
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) {
        guac_client_abort(ssh_tunnel->client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "Failed to create UNIX domain socket.");
        return -1;
    }
    
    guac_client_log(ssh_tunnel->client, GUAC_LOG_DEBUG, "Socket created, binding.");

    /* Bind to the UNIX domain socket. */
    tunnel_data->tunnel_addr.sun_family = AF_UNIX;
    strncpy(tunnel_data->tunnel_addr.sun_path, ssh_tunnel->socket_path,
            sizeof(tunnel_data->tunnel_addr.sun_path) - 1);

    if (bind(fd, (const struct sockaddr *) &tunnel_data->tunnel_addr, sizeof(struct sockaddr_un)) < 0) {
        guac_client_abort(ssh_tunnel->client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "Failed to bind to UNIX domain socket at \"%s\": %s",
                ssh_tunnel->socket_path, strerror(errno));
        return -1;
    }

    /* Listen on the UNIX domain socket for an incoming connection */
    if (listen(fd, GUAC_COMMON_SSH_TUNNEL_BACKLOG_SIZE) < 0) {
        guac_client_abort(ssh_tunnel->client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "Failed to listen on UNIX domain socket at \"%s\": %s",
                ssh_tunnel->socket_path, strerror(errno));
        return -1;
    }
    tunnel_data->listen_socket = fd;

    guac_client_log(ssh_tunnel->client, GUAC_LOG_DEBUG,
            "Listening on socket, creating worker thread.");

    /* Create a thread to wait for the incoming connection and do the work. */
    int retval = pthread_create(&(tunnel_data->tunnel_thread),
                                NULL, guac_common_ssh_tunnel_worker,
                                (void *) tunnel_data);
    if (retval) {
        guac_client_abort(ssh_tunnel->client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "Failed to start worker thread: %d", retval);
        return -1;
    }

    guac_client_log(ssh_tunnel->client, GUAC_LOG_DEBUG,
            "Worker created, return socket path to client.");

    /* Return success */
    return 0;

}


int guac_common_ssh_tunnel_cleanup(guac_ssh_tunnel* ssh_tunnel) {

    /* Stop libssh2 channel and free it */
    if (ssh_tunnel->channel) {
        libssh2_channel_close(ssh_tunnel->channel);
        libssh2_channel_free(ssh_tunnel->channel);
    }

    /* Clean up the SSH session */
    if (ssh_tunnel->session)
        guac_common_ssh_destroy_session(ssh_tunnel->session);

    /* Remove socket and directory, and free string */
    unlink(ssh_tunnel->socket_path);
    rmdir(dirname(ssh_tunnel->socket_path));
    guac_mem_free(ssh_tunnel->socket_path);

    return 0;

}