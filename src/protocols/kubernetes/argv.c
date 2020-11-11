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
#include "kubernetes.h"
#include "terminal/terminal.h"

#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/user.h>

#include <stdlib.h>
#include <string.h>

int guac_kubernetes_argv_callback(guac_user* user, const char* mimetype,
        const char* name, const char* value, void* data) {

    guac_client* client = user->client;
    guac_kubernetes_client* kubernetes_client = (guac_kubernetes_client*) client->data;
    guac_terminal* terminal = kubernetes_client->term;

    /* Update color scheme */
    if (strcmp(name, GUAC_KUBERNETES_ARGV_COLOR_SCHEME) == 0)
        guac_terminal_apply_color_scheme(terminal, value);

    /* Update font name */
    else if (strcmp(name, GUAC_KUBERNETES_ARGV_FONT_NAME) == 0)
        guac_terminal_apply_font(terminal, value, -1, 0);

    /* Update only if font size is sane */
    else if (strcmp(name, GUAC_KUBERNETES_ARGV_FONT_SIZE) == 0) {
        int size = atoi(value);
        if (size > 0)
            guac_terminal_apply_font(terminal, NULL, size,
                    kubernetes_client->settings->resolution);
    }

    /* Update Kubernetes terminal size */
    guac_kubernetes_resize(client, terminal->term_height,
            terminal->term_width);

    return 0;

}

void* guac_kubernetes_send_current_argv(guac_user* user, void* data) {

    guac_kubernetes_client* kubernetes_client = (guac_kubernetes_client*) data;
    guac_terminal* terminal = kubernetes_client->term;

    /* Send current color scheme */
    guac_user_stream_argv(user, user->socket, "text/plain",
            GUAC_KUBERNETES_ARGV_COLOR_SCHEME, terminal->color_scheme);

    /* Send current font name */
    guac_user_stream_argv(user, user->socket, "text/plain",
            GUAC_KUBERNETES_ARGV_FONT_NAME, terminal->font_name);

    /* Send current font size */
    char font_size[64];
    sprintf(font_size, "%i", terminal->font_size);
    guac_user_stream_argv(user, user->socket, "text/plain",
            GUAC_KUBERNETES_ARGV_FONT_SIZE, font_size);

    return NULL;


}

