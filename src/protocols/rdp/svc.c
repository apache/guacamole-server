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

#include "channels.h"
#include "client.h"
#include "common/list.h"
#include "rdp.h"
#include "svc.h"

#include <freerdp/svc.h>
#include <guacamole/client.h>
#include <guacamole/string.h>
#include <winpr/stream.h>
#include <winpr/wtsapi.h>

#include <stdlib.h>

void guac_rdp_svc_send_pipe(guac_socket* socket, guac_rdp_svc* svc) {

    /* Send pipe instruction for the SVC's output stream */
    guac_protocol_send_pipe(socket, svc->output_pipe,
            "application/octet-stream", svc->channel_def.name);

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

void guac_rdp_svc_add(guac_client* client, guac_rdp_svc* svc) {

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;

    /* Add to list of available SVC */
    guac_common_list_lock(rdp_client->available_svc);
    guac_common_list_add(rdp_client->available_svc, svc);
    guac_common_list_unlock(rdp_client->available_svc);

}

guac_rdp_svc* guac_rdp_svc_get(guac_client* client, const char* name) {

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_common_list_element* current;
    guac_rdp_svc* found = NULL;

    /* For each available SVC */
    guac_common_list_lock(rdp_client->available_svc);
    current = rdp_client->available_svc->head;
    while (current != NULL) {

        /* If name matches, found */
        guac_rdp_svc* current_svc = (guac_rdp_svc*) current->data;
        if (strcmp(current_svc->channel_def.name, name) == 0) {
            found = current_svc;
            break;
        }

        current = current->next;

    }
    guac_common_list_unlock(rdp_client->available_svc);

    return found;

}

guac_rdp_svc* guac_rdp_svc_remove(guac_client* client, const char* name) {

    guac_rdp_client* rdp_client = (guac_rdp_client*) client->data;
    guac_common_list_element* current;
    guac_rdp_svc* found = NULL;

    /* For each available SVC */
    guac_common_list_lock(rdp_client->available_svc);
    current = rdp_client->available_svc->head;
    while (current != NULL) {

        /* If name matches, remove entry */
        guac_rdp_svc* current_svc = (guac_rdp_svc*) current->data;
        if (strcmp(current_svc->channel_def.name, name) == 0) {
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

    /* Do not write of plugin not associated */
    if (!svc->open_handle) {
        guac_client_log(svc->client, GUAC_LOG_WARNING, "%i bytes of data "
                "received from the Guacamole client for SVC \"%s\" are being "
                "dropped because the remote desktop side of that SVC is "
                "connected.", length, svc->channel_def.name);
        return;
    }

    /* FreeRDP_VirtualChannelWriteEx() assumes that sent data is dynamically
     * allocated and will free() the data after it is sent */
    void* data_copy = malloc(length);
    memcpy(data_copy, data, length);

    /* Send received data */
    svc->entry_points.pVirtualChannelWriteEx(svc->init_handle,
            svc->open_handle, data_copy, length,
            NULL /* NOTE: If non-NULL, this MUST be a pointer to a wStream
                    containing the supplied buffer, and that wStream will be
                    automatically freed when FreeRDP handles the write */);

}

int guac_rdp_svc_pipe_handler(guac_user* user, guac_stream* stream,
        char* mimetype, char* name) {

    guac_rdp_svc* svc = guac_rdp_svc_get(user->client, name);

    /* Fail if no such SVC */
    if (svc == NULL) {
        guac_user_log(user, GUAC_LOG_WARNING, "User requested non-existent "
                "pipe (no such SVC configured): \"%s\"", name);
        guac_protocol_send_ack(user->socket, stream, "FAIL (NO SUCH PIPE)",
                GUAC_PROTOCOL_STATUS_CLIENT_BAD_REQUEST);
        guac_socket_flush(user->socket);
        return 0;
    }
    else
        guac_user_log(user, GUAC_LOG_DEBUG, "Inbound half of channel \"%s\" "
                "connected.", name);

    /* Init stream data */
    stream->data = svc;
    stream->blob_handler = guac_rdp_svc_blob_handler;

    return 0;

}

int guac_rdp_svc_blob_handler(guac_user* user, guac_stream* stream,
        void* data, int length) {

    /* Write blob data to SVC directly */
    guac_rdp_svc* svc = (guac_rdp_svc*) stream->data;
    guac_rdp_svc_write(svc, data, length);

    guac_protocol_send_ack(user->socket, stream, "OK (DATA RECEIVED)",
            GUAC_PROTOCOL_STATUS_SUCCESS);
    guac_socket_flush(user->socket);
    return 0;

}

void guac_rdp_svc_load_plugin(rdpContext* context, char* name) {

    guac_client* client = ((rdp_freerdp_context*) context)->client;
    guac_rdp_svc* svc = calloc(1, sizeof(guac_rdp_svc));
    svc->client = client;

    /* Init FreeRDP channel definition */
    int name_length = guac_strlcpy(svc->channel_def.name, name, GUAC_RDP_SVC_MAX_LENGTH);
    svc->channel_def.options = CHANNEL_OPTION_INITIALIZED
        | CHANNEL_OPTION_ENCRYPT_RDP
        | CHANNEL_OPTION_COMPRESS_RDP;

    /* Warn about name length */
    if (name_length >= GUAC_RDP_SVC_MAX_LENGTH)
        guac_client_log(client, GUAC_LOG_WARNING,
                "Static channel name \"%s\" exceeds maximum length of %i "
                "characters and will be truncated to \"%s\".",
                name, GUAC_RDP_SVC_MAX_LENGTH - 1, svc->channel_def.name);

    /* Attempt to load guacsvc plugin for new static channel */
    if (guac_freerdp_channels_load_plugin(context->channels, context->settings, "guacsvc", svc)) {
        guac_client_log(client, GUAC_LOG_WARNING, "Cannot create static "
                "channel \"%s\": failed to load guacsvc plugin.",
                svc->channel_def.name);
        free(svc);
    }

    /* Store and log on success (SVC structure will be freed on channel termination) */
    else
        guac_client_log(client, GUAC_LOG_INFO, "Created static channel "
                "\"%s\"...", svc->channel_def.name);

}

