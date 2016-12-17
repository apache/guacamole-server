
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

xcb_connection_t* guac_drv_get_connection() {

    char display_name[64];

    /* Get display name of display served by Guacamole X.Org driver */
    sprintf(display_name, ":%s", display);

    /* Connect to X server hosting display */
    xcb_connection_t* connection = xcb_connect(display_name, NULL);

    /* Return NULL if connection fails */
    if (xcb_connection_has_error(connection)) {
        xcb_disconnect(connection);
        return NULL;
    }

    /* Connection succeeded */
    return connection;

}

