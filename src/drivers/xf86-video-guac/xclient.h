
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

#ifndef GUAC_DRV_XCLIENT_H
#define GUAC_DRV_XCLIENT_H

#include <xcb/xcb.h>

/**
 * Allocates a new xcb_auth_info_t structure containing newly-generated and
 * valid X authorization data. Future connections to the X server will be
 * authorized if they use this data. The allocated xcb_auth_info_t structure
 * MUST eventually be freed with guac_drv_revoke_authorization().
 *
 * @return
 *     A pointer to a newly-allocated xcb_auth_info_t structure, or NULL if
 *     authorization fails for any reason.
 */
xcb_auth_info_t* guac_drv_authorize();

/**
 * Revokes the authorization described by the given xcb_auth_info_t structure,
 * freeing any associated memory. The xcb_auth_info_t structure MUST have been
 * previously allocated by guac_drv_authorize().
 *
 * @param auth
 *     A pointer to an xcb_auth_info_t structure allocated by
 *     guac_drv_authorize().
 */
void guac_drv_revoke_authorization(xcb_auth_info_t* auth);

/**
 * Creates a new client connection to display associated with the Guacamole
 * X.Org driver using XCB.
 *
 * @param auth
 *     A pointer to the xcb_auth_info_t structure that should be used to
 *     authorize with the X server.
 *
 * @return
 *     A new XCB connection to the display associated with the Guacamole
 *     X.Org driver, or NULL if the connection cannot be established.
 */
xcb_connection_t* guac_drv_get_connection(xcb_auth_info_t* auth);

/**
 * Looks up the definition of the atom having the given name. If no such atom
 * is defined, XCB_ATOM_NONE is returned.
 *
 * @param connection
 *     The connection to use to look up the atom definition.
 *
 * @param name
 *     The name of the atom to look up.
 *
 * @return
 *     The atom having the given name, or XCB_ATOM_NONE if no such atom is
 *     defined.
 */
xcb_atom_t guac_drv_get_atom(xcb_connection_t* connection,
        const char* name);

#endif

