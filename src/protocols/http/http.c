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

#include "http.h"

#include <guacamole/client.h>
#include <guacamole/error.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


void* guac_http_client_thread(void* data) {

    guac_client* client = (guac_client*)data;

    /* Log that the HTTP client thread is starting */
    guac_client_log(client, GUAC_LOG_INFO, "Starting HTTP client thread");

    /* Create a new process */
    pid_t pid = fork();

    if (pid == -1) {
        /* Fork failed */
        guac_client_log(client, GUAC_LOG_ERROR, "Failed to fork CEF process.");
        return NULL;
    }

    if (pid == 0) {
        /* Child process - run the CEF code */
        execl(CEF_PROCESS_PATH, "cef_process", NULL);

        /* If execl returns, an error occurred */
        guac_client_log(client, GUAC_LOG_ERROR, "Failed to execute CEF process.");
        exit(1);
    }

    /**
     * Main loop to handle HTTP protocol messages, graphical updates
     * and other processing specific to the HTTP protocol.
     */
    while (client->state == GUAC_CLIENT_RUNNING) {

    }

    /* Log that the HTTP client thread is closing */
    guac_client_log(client, GUAC_LOG_INFO, "Closing HTTP client thread");

    return NULL;
}
