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
#include "vnc.h"

#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/user.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>

/**
 * All VNC connection settings which may be updated by unprivileged users
 * through "argv" streams.
 */
typedef enum guac_vnc_argv_setting {

    /**
     * The username for the connection.
     */
    GUAC_VNC_ARGV_SETTING_USERNAME,
    
    /**
     * The password for the connection.
     */
    GUAC_VNC_ARGV_SETTING_PASSWORD

} guac_vnc_argv_setting;

/**
 * The value or current status of a connection parameter received over an
 * "argv" stream.
 */
typedef struct guac_vnc_argv {

    /**
     * The specific setting being updated.
     */
    guac_vnc_argv_setting setting;

    /**
     * Buffer space for containing the received argument value.
     */
    char buffer[GUAC_VNC_ARGV_MAX_LENGTH];

    /**
     * The number of bytes received so far.
     */
    int length;

} guac_vnc_argv;

/**
 * Handler for "blob" instructions which appends the data from received blobs
 * to the end of the in-progress argument value buffer.
 *
 * @see guac_user_blob_handler
 */
static int guac_vnc_argv_blob_handler(guac_user* user, guac_stream* stream,
        void* data, int length) {

    guac_vnc_argv* argv = (guac_vnc_argv*) stream->data;

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
static int guac_vnc_argv_end_handler(guac_user* user, guac_stream* stream) {

    guac_client* client = user->client;
    guac_vnc_client* vnc_client = (guac_vnc_client*) client->data;
    guac_vnc_settings* settings = vnc_client->settings;

    /* Append null terminator to value */
    guac_vnc_argv* argv = (guac_vnc_argv*) stream->data;
    argv->buffer[argv->length] = '\0';

    /* Apply changes to chosen setting */
    switch (argv->setting) {
        
        /* Update username */
        case GUAC_VNC_ARGV_SETTING_USERNAME:
            
            /* Update username in settings. */
            free(settings->username);
            settings->username = strndup(argv->buffer, argv->length);
            
            /* Remove the username conditional flag. */
            vnc_client->vnc_credential_flags &= ~GUAC_VNC_COND_FLAG_USERNAME;
            break;
        
        /* Update password */
        case GUAC_VNC_ARGV_SETTING_PASSWORD:
            
            /* Update password in settings */
            free(settings->password);
            settings->password = strndup(argv->buffer, argv->length);
            
            /* Remove the password conditional flag. */
            vnc_client->vnc_credential_flags &= ~GUAC_VNC_COND_FLAG_PASSWORD;
            break;

    }
    
    /* If no flags are set, signal the conditional. */
    if (!vnc_client->vnc_credential_flags)
        pthread_cond_broadcast(&(vnc_client->vnc_credential_cond));

    free(argv);
    return 0;

}

int guac_vnc_argv_handler(guac_user* user, guac_stream* stream, char* mimetype,
        char* name) {

    guac_vnc_argv_setting setting;

    /* Allow users to update authentication information */
    if (strcmp(name, GUAC_VNC_PARAMETER_NAME_USERNAME) == 0)
        setting = GUAC_VNC_ARGV_SETTING_USERNAME;
    else if (strcmp(name, GUAC_VNC_PARAMETER_NAME_PASSWORD) == 0)
        setting = GUAC_VNC_ARGV_SETTING_PASSWORD;

    /* No other connection parameters may be updated */
    else {
        guac_protocol_send_ack(user->socket, stream, "Not allowed.",
                GUAC_PROTOCOL_STATUS_CLIENT_FORBIDDEN);
        guac_socket_flush(user->socket);
        return 0;
    }

    guac_vnc_argv* argv = malloc(sizeof(guac_vnc_argv));
    argv->setting = setting;
    argv->length = 0;

    /* Prepare stream to receive argument value */
    stream->blob_handler = guac_vnc_argv_blob_handler;
    stream->end_handler = guac_vnc_argv_end_handler;
    stream->data = argv;

    /* Signal stream is ready */
    guac_protocol_send_ack(user->socket, stream, "Ready for updated "
            "parameter.", GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(user->socket);
    return 0;

}
