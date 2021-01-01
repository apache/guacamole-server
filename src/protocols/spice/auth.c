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
#include "auth.h"
#include "spice.h"

#include <guacamole/argv.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/string.h>

#include <glib-unix.h>
#include <pthread.h>
#include <string.h>

gboolean guac_spice_get_credentials(guac_client* client) {
    
    guac_spice_client* spice_client = ((guac_spice_client*) client->data);
    guac_spice_settings* settings = spice_client->settings;
    
    char* params[3] = {NULL};
    int i = 0;
    
    /* Check if username is not provided. */
    if (settings->username == NULL) {
        guac_argv_register(GUAC_SPICE_ARGV_USERNAME, guac_spice_argv_callback, NULL, 0);
        params[i] = GUAC_SPICE_ARGV_USERNAME;
        i++;
    }

    /* Check if password is not provided. */
    if (settings->password == NULL) {
        guac_argv_register(GUAC_SPICE_ARGV_PASSWORD, guac_spice_argv_callback, NULL, 0);
        params[i] = GUAC_SPICE_ARGV_PASSWORD;
        i++;
    }

    params[i] = NULL;

    /* If we have empty parameters, request them and await response. */
    if (i > 0) {
        guac_client_owner_send_required(client, (const char**) params);
        guac_argv_await((const char**) params);
        return true;
    }
    
    guac_client_log(client, GUAC_LOG_DEBUG,
            "Unable to retrieve any credentials from the user.");
    return false;
    
}
