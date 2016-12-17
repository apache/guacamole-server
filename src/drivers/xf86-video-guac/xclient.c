
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

#include <config.h>

#ifdef HAVE_OSSP_UUID_H
#include <ossp/uuid.h>
#else
#include <uuid.h>
#endif

#include <xf86.h>
#include <xf86str.h>
#include <opaque.h>

#include <xcb/xcb.h>

#include <string.h>

/**
 * Generates a 128-bit cryptographically-random value for use with X
 * authorization. This value will be dynamically allocated, and must eventually
 * be freed by a call to free().
 *
 * @param data
 *     A pointer to the pointer which should contain the address of the
 *     newly-allocated random value.
 *
 * @param data_length
 *     A pointer to an integer which should receive the length of the
 *     newly-allocated random value.
 *
 * @return
 *     Zero on success, non-zero if the value could not be generated. Neither
 *     data nor data_length will be touched if generation fails.
 */
static int guac_drv_generate_cookie_data(char** data, int* data_length) {

    uuid_t* uuid;

    /* Attempt to create UUID object */
    if (uuid_create(&uuid) != UUID_RC_OK)
        return 1;

    /* Generate random UUID */
    if (uuid_make(uuid, UUID_MAKE_V4) != UUID_RC_OK) {
        uuid_destroy(uuid);
        return 1;
    }

    /* Allocate buffer for future formatted ID */
    size_t identifier_length = UUID_LEN_BIN;
    char* identifier = malloc(UUID_LEN_BIN);

    /* Build connection ID from UUID */
    if (uuid_export(uuid, UUID_FMT_BIN, &identifier,
                &identifier_length) != UUID_RC_OK) {
        free(identifier);
        uuid_destroy(uuid);
        return 1;
    }

    uuid_destroy(uuid);

    /* Generation was successful */
    *data_length = identifier_length;
    *data = identifier;
    return 0;

}

xcb_auth_info_t* guac_drv_authorize() {

    const char* name = "MIT-MAGIC-COOKIE-1";
    int name_length = strlen(name);

    char* data;
    int data_length;

    /* Generate random data for cookie */
    if (guac_drv_generate_cookie_data(&data, &data_length))
        return NULL;

    /* Attempt to add generated authorization */
    if (!AddAuthorization(name_length, name, data_length, data)) {
        free(data);
        return NULL;
    }

    /* Allocate new authorization structure */
    xcb_auth_info_t* auth = malloc(sizeof(xcb_auth_info_t));

    /* Copy name (which must be dynamically-allocated) */
    auth->name = strdup(name);
    auth->namelen = name_length;

    /* Point to newly-allocated cookie data */
    auth->data = data;
    auth->datalen = data_length;

    return auth;

}

void guac_drv_revoke_authorization(xcb_auth_info_t* auth) {

    RemoveAuthorization(auth->namelen, auth->name,
            auth->datalen, auth->data);

    /* Free cookie type name and data */
    free(auth->name);
    free(auth->data);

    /* Free structure itself */
    free(auth);

}

xcb_connection_t* guac_drv_get_connection(xcb_auth_info_t* auth) {

    char display_name[64];

    /* Get display name of display served by Guacamole X.Org driver */
    sprintf(display_name, ":%s", display);

    /* Connect to X server hosting display */
    xcb_connection_t* connection = xcb_connect_to_display_with_auth_info(
            display_name, auth, NULL);

    /* Return NULL if connection fails */
    if (xcb_connection_has_error(connection)) {
        xcb_disconnect(connection);
        return NULL;
    }

    /* Connection succeeded */
    return connection;

}

