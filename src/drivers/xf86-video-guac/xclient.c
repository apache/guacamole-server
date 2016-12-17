
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

#include <xf86.h>
#include <xf86str.h>
#include <opaque.h>

#include <xcb/xcb.h>

/**
 * Populates the given xcb_auth_info_t structure with valid X authorization
 * data.
 *
 * @return
 *     Zero on success, non-zero if X authorization data could not be
 *     created.
 */
static int guac_drv_authorize(xcb_auth_info_t* auth) {

    /* FIXME: Generate and add authorization, rather than searching */

    int id;

    /* Loop through all fake client IDs */
    for (id = 0; id < 1024; id++) {

        unsigned short name_len;
        const char* name;

        unsigned short data_len;
        char* data;

        /* Attempt to read authorization for (fake) client */
        if (AuthorizationFromID(id, &name_len, &name, &data_len, &data)) {

            /* Copy name */
            auth->namelen = name_len;
            auth->name = strdup(name);

            /* Copy data */
            auth->datalen = data_len;
            auth->data = malloc(data_len);
            memcpy(auth->data, data, data_len);

            /* Authorization successfully retrieved */
            return 0;

        }

    }

    /* Authorization could not be read */
    return 1;

}

xcb_connection_t* guac_drv_get_connection() {

    char display_name[64];

    /* Attempt to read X authorization */
    xcb_auth_info_t auth;
    if (guac_drv_authorize(&auth))
        return NULL;

    /* Get display name of display served by Guacamole X.Org driver */
    sprintf(display_name, ":%s", display);

    /* Connect to X server hosting display */
    xcb_connection_t* connection = xcb_connect_to_display_with_auth_info(
            display_name, &auth, NULL);

    /* Return NULL if connection fails */
    if (xcb_connection_has_error(connection)) {
        xcb_disconnect(connection);
        free(auth.name);
        free(auth.data);
        return NULL;
    }

    /* Connection succeeded */
    free(auth.name);
    free(auth.data);
    return connection;

}

