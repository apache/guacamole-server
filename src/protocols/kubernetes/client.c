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
#include "client.h"
#include "common/recording.h"
#include "kubernetes.h"
#include "settings.h"
#include "terminal/terminal.h"
#include "user.h"

#include <langinfo.h>
#include <locale.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <guacamole/client.h>

int guac_client_init(guac_client* client) {

    /* Set client args */
    client->args = GUAC_KUBERNETES_CLIENT_ARGS;

    /* Allocate client instance data */
    guac_kubernetes_client* kubernetes_client = calloc(1, sizeof(guac_kubernetes_client));
    client->data = kubernetes_client;

    /* Init clipboard */
    kubernetes_client->clipboard = guac_common_clipboard_alloc(GUAC_KUBERNETES_CLIPBOARD_MAX_LENGTH);

    /* Set handlers */
    client->join_handler = guac_kubernetes_user_join_handler;
    client->free_handler = guac_kubernetes_client_free_handler;

    /* Set locale and warn if not UTF-8 */
    setlocale(LC_CTYPE, "");
    if (strcmp(nl_langinfo(CODESET), "UTF-8") != 0) {
        guac_client_log(client, GUAC_LOG_INFO,
                "Current locale does not use UTF-8. Some characters may "
                "not render correctly.");
    }

    /* Success */
    return 0;

}

int guac_kubernetes_client_free_handler(guac_client* client) {

    guac_kubernetes_client* kubernetes_client =
        (guac_kubernetes_client*) client->data;

    /* Wait client thread to terminate */
    pthread_join(kubernetes_client->client_thread, NULL);

    /* Free settings */
    if (kubernetes_client->settings != NULL)
        guac_kubernetes_settings_free(kubernetes_client->settings);

    guac_common_clipboard_free(kubernetes_client->clipboard);
    free(kubernetes_client);
    return 0;

}

