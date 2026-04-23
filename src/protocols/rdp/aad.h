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

#ifndef GUAC_RDP_AAD_H
#define GUAC_RDP_AAD_H

#include <guacamole/client.h>

/**
 * Default tenant ID. The "common" endpoint supports multi-tenant
 * authentication for both organizational and personal accounts.
 */
#define GUAC_AAD_DEFAULT_TENANT_ID "common"

/**
 * Azure AD authentication parameters used across all AAD auth flows.
 */
typedef struct guac_rdp_aad_params {

    /**
     * The Azure AD tenant ID (or "common" for multi-tenant apps).
     */
    char* tenant_id;

    /**
     * The application (client) ID from Azure AD app registration.
     */
    char* client_id;

    /**
     * The username (email) for authentication.
     */
    char* username;

    /**
     * The password for authentication.
     */
    char* password;

    /**
     * The OAuth2 scope to request.
     */
    char* scope;

    /**
     * The Proof-of-Possession key confirmation parameter (req_cnf) provided
     * by FreeRDP's AAD layer. This is a base64url-encoded JSON string
     * containing the key ID (kid) derived from the POP RSA key pair. Azure AD
     * uses this to bind the access token to the key. May be NULL if POP is
     * not required.
     */
    char* req_cnf;

} guac_rdp_aad_params;

/**
 * Decodes a percent-encoded (URL-encoded) string. Each %XX sequence is
 * replaced with the corresponding byte value.
 *
 * @param str
 *     The percent-encoded string to decode, or NULL.
 *
 * @return
 *     A newly allocated decoded string, or NULL if the input was NULL.
 *     The caller must free the returned string with guac_mem_free().
 */
char* guac_rdp_percent_decode(const char* str);

/**
 * Retrieves an Azure AD access token using the OAuth2 Authorization Code
 * flow. This function automates the browser-based login by fetching the
 * Microsoft login page, extracting session tokens, posting credentials,
 * and exchanging the resulting authorization code for an access token.
 *
 * @param client
 *     The guac_client associated with the RDP connection.
 *
 * @param params
 *     The AAD authentication parameters including tenant ID, client ID,
 *     username, password, scope, and optional req_cnf.
 *
 * @return
 *     A newly allocated string containing the access token, or NULL if
 *     authentication failed. The caller must free the returned string.
 */
char* guac_rdp_aad_get_token_authcode(guac_client* client,
        guac_rdp_aad_params* params);

#endif /* GUAC_RDP_AAD_H */
