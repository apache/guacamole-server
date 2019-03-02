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

#include "config.h"
#include "argv.h"
#include "rdp.h"

#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/user.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

/**
 * All RDP connection settings which may be updated by unprivileged users
 * through "argv" streams.
 */
typedef enum guac_rdp_argv_setting {

    /**
     * The username of the connection.
     */
    GUAC_RDP_ARGV_SETTING_USERNAME,
            
    /**
     * The password to authenticate the connection.
     */
    GUAC_RDP_ARGV_SETTING_PASSWORD,
            
    /**
     * The domain to use for connection authentication.
     */
    GUAC_RDP_ARGV_SETTING_DOMAIN

} guac_rdp_argv_setting;

/**
 * The value or current status of a connection parameter received over an
 * "argv" stream.
 */
typedef struct guac_rdp_argv {

    /**
     * The specific setting being updated.
     */
    guac_rdp_argv_setting setting;

    /**
     * Buffer space for containing the received argument value.
     */
    char buffer[GUAC_RDP_ARGV_MAX_LENGTH];

    /**
     * The number of bytes received so far.
     */
    int length;

} guac_rdp_argv;

/**
 * Handler for "blob" instructions which appends the data from received blobs
 * to the end of the in-progress argument value buffer.
 *
 * @see guac_user_blob_handler
 */
static int guac_rdp_argv_blob_handler(guac_user* user,
        guac_stream* stream, void* data, int length) {

    guac_rdp_argv* argv = (guac_rdp_argv*) stream->data;
    
    /* Calculate buffer size remaining, including space for null terminator,
     * adjusting received length accordingly */
    int remaining = sizeof(argv->buffer) - argv->length - 1;
    if (length > remaining)
        length = remaining;

    /* Append received data to end of buffer */
    memcpy(argv->buffer + argv->length, data, length);
    argv->length += length;

    return 0;

}

/**
 * Handler for "end" instructions which applies the changes specified by the
 * argument value buffer associated with the stream.
 *
 * @see guac_user_end_handler
 */
static int guac_rdp_argv_end_handler(guac_user* user,
        guac_stream* stream) {

    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_rdp_settings* settings = rdp_client->settings;
    
    /* Append null terminator to value */
    guac_rdp_argv* argv = (guac_rdp_argv*) stream->data;
    argv->buffer[argv->length] = '\0';

    /* Apply changes to chosen setting */
    switch (argv->setting) {

        /* Update RDP username. */
        case GUAC_RDP_ARGV_SETTING_USERNAME:
            free(settings->username);
            settings->username = malloc(strlen(argv->buffer) * sizeof(char));
            strcpy(settings->username, argv->buffer);
            pthread_cond_broadcast(&(rdp_client->rdp_cond));
            break;
            
        case GUAC_RDP_ARGV_SETTING_PASSWORD:
            free(settings->password);
            settings->password = malloc(strlen(argv->buffer) * sizeof(char));
            strcpy(settings->password, argv->buffer);
            pthread_cond_broadcast(&(rdp_client->rdp_cond));
            break;
            
        case GUAC_RDP_ARGV_SETTING_DOMAIN:
            free(settings->domain);
            settings->domain = malloc(strlen(argv->buffer) * sizeof(char));
            strcpy(settings->domain, argv->buffer);
            pthread_cond_broadcast(&(rdp_client->rdp_cond));
            break;
            
    }

    free(argv);
    return 0;

}

int guac_rdp_argv_handler(guac_user* user, guac_stream* stream,
        char* mimetype, char* name) {

    guac_rdp_argv_setting setting;

    /* Allow users to update authentication details */
    if (strcmp(name, "username") == 0)
        setting = GUAC_RDP_ARGV_SETTING_USERNAME;
    else if (strcmp(name, "password") == 0)
        setting = GUAC_RDP_ARGV_SETTING_PASSWORD;
    else if (strcmp(name, "domain") == 0)
        setting = GUAC_RDP_ARGV_SETTING_DOMAIN;

    /* No other connection parameters may be updated */
    else {
        guac_protocol_send_ack(user->socket, stream, "Not allowed.",
                GUAC_PROTOCOL_STATUS_CLIENT_FORBIDDEN);
        guac_socket_flush(user->socket);
        return 0;
    }

    guac_rdp_argv* argv = malloc(sizeof(guac_rdp_argv));
    argv->setting = setting;
    argv->length = 0;

    /* Prepare stream to receive argument value */
    stream->blob_handler = guac_rdp_argv_blob_handler;
    stream->end_handler = guac_rdp_argv_end_handler;
    stream->data = argv;

    /* Signal stream is ready */
    guac_protocol_send_ack(user->socket, stream, "Ready for updated "
            "parameter.", GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(user->socket);
    return 0;

}

