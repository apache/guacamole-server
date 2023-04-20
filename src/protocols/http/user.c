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
#include "http.h"
#include "settings.h"
#include "user.h"

int guac_http_user_join_handler(guac_user* user, int argc, char** argv) {

    guac_client* client = user->client;
    guac_http_client* http_client = (guac_http_client*) client->data;

    /* Parse provided arguments */
    guac_http_settings* settings = guac_http_parse_args(user,
            argc, (const char**) argv);

    /* Fail if settings cannot be parsed */
    if (settings == NULL) {
        guac_user_log(user, GUAC_LOG_INFO,
                "Badly formatted client arguments.");
        return 1;
    }

    /* Store settings at user level */
    user->data = settings;

    /* Store owner's settings at the client level */
    http_client->settings = settings;

    /* Start the client thread */
    if (pthread_create(&http_client->client_thread, NULL,
                guac_http_client_thread, client)) {
        guac_user_log(user, GUAC_LOG_ERROR,
                "Unable to start HTTP client thread.");
        return 1;
    }

    return 0;

}

int guac_http_user_leave_handler(guac_user* user) {

    /* Free settings if not owner (owner settings will be freed with client) */
    if (!user->owner) {
        guac_http_settings* settings = (guac_http_settings*) user->data;
        guac_http_settings_free(settings);
    }

    return 0;
}
