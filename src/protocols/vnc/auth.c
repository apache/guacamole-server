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
#include <rfb/rfbclient.h>
#include <rfb/rfbproto.h>

char* guac_vnc_get_password(rfbClient* client) {
    guac_client* gc = rfbClientGetClientData(client, GUAC_VNC_CLIENT_KEY);
    return ((guac_vnc_client*) gc->data)->settings->password;
}

rfbCredential* guac_vnc_get_credentials(rfbClient* client, int credentialType) {
    guac_client* gc = rfbClientGetClientData(client, GUAC_VNC_CLIENT_KEY);
    rfbCredential *creds = malloc(sizeof(rfbCredential));
    guac_vnc_settings* settings = ((guac_vnc_client*) gc->data)->settings;
    
    if (credentialType == rfbCredentialTypeUser) {
        creds->userCredential.username = settings->username;
        creds->userCredential.password = settings->password;
        return creds;
    }
    
    else if (credentialType == rfbCredentialTypeX509) {
        char* template = "guac_XXXXXX";
        
        if (settings->client_cert != NULL) {
            settings->client_cert_temp = strdup(template);
            int cert_fd = mkstemp(settings->client_cert_temp);
            write(cert_fd, settings->client_cert, strlen(settings->client_cert));
            close(cert_fd);
            creds->x509Credential.x509ClientCertFile = settings->client_cert_temp;
        }
        
        if (settings->client_key != NULL) {
            settings->client_key_temp = strdup(template);
            int key_fd = mkstemp(settings->client_key_temp);
            write(key_fd, settings->client_key, strlen(settings->client_key));
            close(key_fd);
            creds->x509Credential.x509ClientKeyFile = settings->client_key_temp;
        }
        
        if (settings->ca_cert != NULL) {
            settings->ca_cert_temp = strdup(template);
            int ca_fd = mkstemp(settings->ca_cert_temp);
            write(ca_fd, settings->ca_cert, strlen(settings->ca_cert));
            close(ca_fd);
            creds->x509Credential.x509CACertFile = settings->ca_cert_temp;
        }
        
        if (settings->ca_crl != NULL) {
            settings->ca_crl_temp = strdup(template);
            int crl_fd = mkstemp(settings->ca_crl_temp);
            write(crl_fd, settings->ca_crl, strlen(settings->ca_crl));
            close(crl_fd);
            creds->x509Credential.x509CACrlFile = settings->ca_crl_temp;
        }
        
        return creds;
    }
    
    guac_client_log(gc, GUAC_LOG_ERROR, "Unknown credential type requested.");
    return NULL;
    
}
