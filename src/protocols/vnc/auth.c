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

#include "auth.h"
#include "vnc.h"

#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <rfb/rfbclient.h>
#include <rfb/rfbproto.h>

#include <pthread.h>
#include <string.h>

char* guac_vnc_get_password(rfbClient* client) {
    guac_client* gc = rfbClientGetClientData(client, GUAC_VNC_CLIENT_KEY);
    guac_vnc_client* vnc_client = ((guac_vnc_client*) gc->data);
    guac_vnc_settings* settings = vnc_client->settings;
    
    /* If password isn't around, prompt for it. */
    if (settings->password == NULL || strcmp(settings->password, "") == 0) {
        /* Lock the thread. */
        pthread_mutex_lock(&(vnc_client->vnc_credential_lock));
        
        /* Send the request for password and flush the socket. */
        guac_protocol_send_required(gc->socket,
                (const char* []) {"password", NULL});
        guac_socket_flush(gc->socket);
        
        /* Set the conditional flag. */
        vnc_client->vnc_credential_flags |= GUAC_VNC_COND_FLAG_PASSWORD;
        
        /* Wait for the condition. */
        pthread_cond_wait(&(vnc_client->vnc_credential_cond),
                &(vnc_client->vnc_credential_lock));
        
        /* Unlock the thread. */
        pthread_mutex_unlock(&(vnc_client->vnc_credential_lock));
    }
    
    return settings->password;
    
}

rfbCredential* guac_vnc_get_credentials(rfbClient* client, int credentialType) {
    guac_client* gc = rfbClientGetClientData(client, GUAC_VNC_CLIENT_KEY);
    guac_vnc_client* vnc_client = ((guac_vnc_client*) gc->data);
    guac_vnc_settings* settings = vnc_client->settings;

    /* Handle request for Username/Password credentials */
    if (credentialType == rfbCredentialTypeUser) {
        rfbCredential *creds = malloc(sizeof(rfbCredential));
        char* params[2] = {NULL};
        int i = 0;
        
        /* Check if username is null or empty. */
        if (settings->username == NULL || strcmp(settings->username, "") == 0) {
            params[i] = "username";
            i++;
            vnc_client->vnc_credential_flags |= GUAC_VNC_COND_FLAG_USERNAME;
        }
        
        /* Check if password is null or empty. */
        if (settings->password == NULL || strcmp(settings->password, "") == 0) {
            params[i] = "password";
            i++;
            vnc_client->vnc_credential_flags |= GUAC_VNC_COND_FLAG_PASSWORD;
        }
        
        /* If we have empty parameters, request them. */
        if (i > 0) {
            /* Lock the thread. */
            pthread_mutex_lock(&(vnc_client->vnc_credential_lock));
            
            /* Send required parameters to client and flush the socket. */
            guac_protocol_send_required(gc->socket, (const char**) params);
            guac_socket_flush(gc->socket);
            
            /* Wait for the parameters to be returned. */
            pthread_cond_wait(&(vnc_client->vnc_credential_cond),
                    &(vnc_client->vnc_credential_lock));
            
            /* Pull the credentials from updated settings. */
            creds->userCredential.username = settings->username;
            creds->userCredential.password = settings->password;
            
            /* Unlock the thread. */
            pthread_mutex_unlock(&(vnc_client->vnc_credential_lock));
            
            return creds;
            
        }
        
    }

    guac_client_abort(gc, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
            "Unsupported credential type requested.");
    guac_client_log(gc, GUAC_LOG_DEBUG,
            "Unable to provide requested type of credential: %d.",
            credentialType);
    return NULL;
    
}
