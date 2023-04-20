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

#include "client.h"
#include "http.h"
#include "settings.h"
#include "user.h"

#include <stdlib.h>

int guac_client_init(guac_client* client, int argc, char** argv) {

    /* Allocate client struct */
    guac_http_client* http_client = calloc(1, sizeof(guac_http_client));
    client->data = http_client;

    /* Set client args */
    client->args = GUAC_HTTP_CLIENT_ARGS;

    /* Handlers specific for HTTP protocol */
    client->join_handler  = guac_http_user_join_handler;
    client->leave_handler = guac_http_user_leave_handler;
    client->free_handler  = guac_http_client_free_handler;

    return 0;
}

int guac_http_client_free_handler(guac_client* client) {

    guac_http_client* http_client = (guac_http_client*) client->data;

    /* Wait for client thread */
    pthread_join(http_client->client_thread, NULL);

    /* Terminate client thread if necessary */
    /* Free parsed settings */
    if (http_client->settings != NULL)
        guac_http_settings_free(http_client->settings);

    /* Free HTTP client struct */
    free(http_client);

    return 0;
}
