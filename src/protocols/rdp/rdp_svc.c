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
#include "common/list.h"
#include "rdp.h"
#include "rdp_svc.h"

#include <freerdp/utils/svc_plugin.h>
#include <guacamole/client.h>
#include <guacamole/string.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

#include <stdlib.h>

guac_rdp_svc* guac_rdp_alloc_svc(guac_client* client, char* name) {

    guac_rdp_svc* svc = malloc(sizeof(guac_rdp_svc));

    /* Init SVC */
    svc->client = client;
    svc->plugin = NULL;
    svc->output_pipe = NULL;

    /* Init name */
    int name_length = guac_strlcpy(svc->name, name, GUAC_RDP_SVC_MAX_LENGTH);

    /* Warn about name length */
    if (name_length >= GUAC_RDP_SVC_MAX_LENGTH)
        guac_client_log(client, GUAC_LOG_INFO,
                "Static channel name \"%s\" exceeds maximum of %i characters "
                "and will be truncated", name, GUAC_RDP_SVC_MAX_LENGTH - 1);

    return svc;
}

void guac_rdp_free_svc(guac_rdp_svc* svc) {
    free(svc);
}

void guac_rdp_svc_send_pipe(guac_socket* socket, guac_rdp_svc* svc) {

    /* Send pipe instruction for the SVC's output stream */
    guac_protocol_send_pipe(socket, svc->output_pipe,
            "application/octet-stream", svc->name);

}

void guac_rdp_svc_send_pipes(guac_user* user) {

    guac_client* client = user->client;
    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    guac_common_list_lock(rdp_client->available_svc);

    /* Send pipe for each allocated SVC's output stream */
    guac_common_list_element* current = rdp_client->available_svc->head;
    while (current != NULL) {
        guac_rdp_svc_send_pipe(user->socket, (guac_rdp_svc*) current->data);
        current = current->next;
    }

    guac_common_list_unlock(rdp_client->available_svc);

}

void guac_rdp_add_svc(guac_client* client, guac_rdp_svc* svc) {

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Add to list of available SVC */
    guac_common_list_lock(rdp_client->available_svc);
    guac_common_list_add(rdp_client->available_svc, svc);
    guac_common_list_unlock(rdp_client->available_svc);

}

guac_rdp_svc* guac_rdp_get_svc(guac_client* client, const char* name) {

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_common_list_element* current;
    guac_rdp_svc* found = NULL;

    /* For each available SVC */
    guac_common_list_lock(rdp_client->available_svc);
    current = rdp_client->available_svc->head;
    while (current != NULL) {

        /* If name matches, found */
        guac_rdp_svc* current_svc = (guac_rdp_svc*) current->data;
        if (strcmp(current_svc->name, name) == 0) {
            found = current_svc;
            break;
        }

        current = current->next;

    }
    guac_common_list_unlock(rdp_client->available_svc);

    return found;

}

guac_rdp_svc* guac_rdp_remove_svc(guac_client* client, const char* name) {

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_common_list_element* current;
    guac_rdp_svc* found = NULL;

    /* For each available SVC */
    guac_common_list_lock(rdp_client->available_svc);
    current = rdp_client->available_svc->head;
    while (current != NULL) {

        /* If name matches, remove entry */
        guac_rdp_svc* current_svc = (guac_rdp_svc*) current->data;
        if (strcmp(current_svc->name, name) == 0) {
            guac_common_list_remove(rdp_client->available_svc, current);
            found = current_svc;
            break;
        }

        current = current->next;

    }
    guac_common_list_unlock(rdp_client->available_svc);

    /* Return removed entry, if any */
    return found;

}

void guac_rdp_svc_write(guac_rdp_svc* svc, void* data, int length) {

    wStream* output_stream;

    /* Do not write of plugin not associated */
    if (svc->plugin == NULL) {
        guac_client_log(svc->client, GUAC_LOG_ERROR,
                "Channel \"%s\" output dropped.",
                svc->name);
        return;
    }

    /* Build packet */
    output_stream = Stream_New(NULL, length);
    Stream_Write(output_stream, data, length);

    /* Send packet */
    svc_plugin_send(svc->plugin, output_stream);

}

