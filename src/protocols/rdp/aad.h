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

#include "config.h"

#ifdef HAVE_FREERDP_AAD_SUPPORT

#include <guacamole/client.h>

/**
 * Default tenant ID. The "organizations" endpoint supports authentication for
 * work/school (Azure AD) accounts, which is the case for Entra-joined RDP.
 */
#define GUAC_AAD_DEFAULT_TENANT_ID "organizations"

/**
 * Obtains an Azure AD access token for the RDP connection using the OAuth2
 * device authorization grant (RFC 8628). A device code is requested and its
 * user code and verification URI are pushed to the session owner's browser for
 * display (as a QR code) over a pipe stream; the user completes sign-in on a
 * separate device while guacd polls for completion. The resulting token is
 * bound to FreeRDP's key via the given req_cnf.
 *
 * @param client
 *     The guac_client associated with the RDP connection.
 *
 * @param tenant_id
 *     The Azure AD tenant ID (or "organizations").
 *
 * @param client_id
 *     The application (client) ID to authenticate as. The app must have public
 *     client flows enabled and be an approved client of the target resource.
 *
 * @param scope
 *     The OAuth2 scope to request (the device-bound scope FreeRDP derives).
 *
 * @param req_cnf
 *     The proof-of-possession key confirmation supplied by FreeRDP, or NULL.
 *
 * @return
 *     A newly-allocated, NULL-terminated access token which the caller must
 *     free with guac_mem_free(), or NULL if no token could be obtained.
 */
char* guac_rdp_aad_get_token(guac_client* client, const char* tenant_id,
        const char* client_id, const char* scope, const char* req_cnf);

/**
 * Decodes the given percent-encoded string, returning a newly-allocated copy.
 *
 * @param str
 *     The percent-encoded string to decode, which may be NULL.
 *
 * @return
 *     A newly-allocated, NULL-terminated decoded copy which the caller must
 *     free with guac_mem_free(), or NULL if str was NULL.
 */
char* guac_rdp_percent_decode(const char* str);

#endif /* HAVE_FREERDP_AAD_SUPPORT */

#endif /* GUAC_RDP_AAD_H */
