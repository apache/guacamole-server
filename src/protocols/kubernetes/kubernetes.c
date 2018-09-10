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
#include "common/recording.h"
#include "kubernetes.h"
#include "terminal/terminal.h"

#include <guacamole/client.h>
#include <guacamole/protocol.h>

#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

/**
 * Input thread, started by the main Kubernetes client thread. This thread
 * continuously reads from the terminal's STDIN and transfers all read
 * data to the Kubernetes connection.
 *
 * @param data
 *     The current guac_client instance.
 *
 * @return
 *     Always NULL.
 */
static void* guac_kubernetes_input_thread(void* data) {

    guac_client* client = (guac_client*) data;
    guac_kubernetes_client* kubernetes_client =
        (guac_kubernetes_client*) client->data;

    char buffer[8192];
    int bytes_read;

    /* Write all data read */
    while ((bytes_read = guac_terminal_read_stdin(kubernetes_client->term, buffer, sizeof(buffer))) > 0) {

        /* TODO: Send to Kubernetes */
        guac_terminal_write(kubernetes_client->term, buffer, bytes_read);

    }

    return NULL;

}

void* guac_kubernetes_client_thread(void* data) {

    guac_client* client = (guac_client*) data;
    guac_kubernetes_client* kubernetes_client =
        (guac_kubernetes_client*) client->data;

    guac_kubernetes_settings* settings = kubernetes_client->settings;

    pthread_t input_thread;

    /* Set up screen recording, if requested */
    if (settings->recording_path != NULL) {
        kubernetes_client->recording = guac_common_recording_create(client,
                settings->recording_path,
                settings->recording_name,
                settings->create_recording_path,
                !settings->recording_exclude_output,
                !settings->recording_exclude_mouse,
                settings->recording_include_keys);
    }

    /* Create terminal */
    kubernetes_client->term = guac_terminal_create(client,
            kubernetes_client->clipboard,
            settings->max_scrollback, settings->font_name, settings->font_size,
            settings->resolution, settings->width, settings->height,
            settings->color_scheme, settings->backspace);

    /* Fail if terminal init failed */
    if (kubernetes_client->term == NULL) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR,
                "Terminal initialization failed");
        return NULL;
    }

    /* Set up typescript, if requested */
    if (settings->typescript_path != NULL) {
        guac_terminal_create_typescript(kubernetes_client->term,
                settings->typescript_path,
                settings->typescript_name,
                settings->create_typescript_path);
    }

    /* TODO: Open WebSocket connection to Kubernetes */

    /* Logged in */
    guac_client_log(client, GUAC_LOG_INFO,
            "Kubernetes connection successful.");

    /* Start input thread */
    if (pthread_create(&(input_thread), NULL, guac_kubernetes_input_thread, (void*) client)) {
        guac_client_abort(client, GUAC_PROTOCOL_STATUS_SERVER_ERROR, "Unable to start input thread");
        return NULL;
    }

    /* TODO: While data available, write to terminal */

    /* Kill client and Wait for input thread to die */
    guac_client_stop(client);
    pthread_join(input_thread, NULL);

    guac_client_log(client, GUAC_LOG_INFO, "Kubernetes connection ended.");
    return NULL;

}

