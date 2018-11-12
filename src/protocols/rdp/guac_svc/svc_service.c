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

#include "svc_service.h"

#include <stdlib.h>
#include <string.h>

#include <freerdp/constants.h>
#include <freerdp/utils/svc_plugin.h>
#include <guacamole/client.h>
#include <guacamole/protocol.h>
#include <guacamole/socket.h>
#include <guacamole/string.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

/**
 * Entry point for arbitrary SVC.
 */
int VirtualChannelEntry(PCHANNEL_ENTRY_POINTS pEntryPoints) {

    /* Gain access to plugin data */
    CHANNEL_ENTRY_POINTS_FREERDP* entry_points_ex =
        (CHANNEL_ENTRY_POINTS_FREERDP*) pEntryPoints;

    /* Allocate plugin */
    guac_svcPlugin* svc_plugin =
        (guac_svcPlugin*) calloc(1, sizeof(guac_svcPlugin));

    /* Get SVC descriptor from plugin parameters */
    guac_rdp_svc* svc = (guac_rdp_svc*) entry_points_ex->pExtendedData;

    /* Init channel def */
    guac_strlcpy(svc_plugin->plugin.channel_def.name, svc->name,
            GUAC_RDP_SVC_MAX_LENGTH);
    svc_plugin->plugin.channel_def.options = 
          CHANNEL_OPTION_INITIALIZED
        | CHANNEL_OPTION_ENCRYPT_RDP
        | CHANNEL_OPTION_COMPRESS_RDP;

    /* Init plugin */
    svc_plugin->svc = svc;

    /* Set callbacks */
    svc_plugin->plugin.connect_callback   = guac_svc_process_connect;
    svc_plugin->plugin.receive_callback   = guac_svc_process_receive;
    svc_plugin->plugin.event_callback     = guac_svc_process_event;
    svc_plugin->plugin.terminate_callback = guac_svc_process_terminate;

    /* Store plugin reference in SVC */
    svc->plugin = (rdpSvcPlugin*) svc_plugin;

    /* Finish init */
    svc_plugin_init((rdpSvcPlugin*) svc_plugin, pEntryPoints);
    return 1;

}

/* 
 * Service Handlers
 */

void guac_svc_process_connect(rdpSvcPlugin* plugin) {

    /* Get corresponding guac_rdp_svc */
    guac_svcPlugin* svc_plugin = (guac_svcPlugin*) plugin;
    guac_rdp_svc* svc = svc_plugin->svc;

    /* NULL out pExtendedData so we don't lose our guac_rdp_svc due to an
     * automatic free() within libfreerdp */
    plugin->channel_entry_points.pExtendedData = NULL;

    /* Create pipe */
    svc->output_pipe = guac_client_alloc_stream(svc->client);

    /* Notify of pipe's existence */
    guac_rdp_svc_send_pipe(svc->client->socket, svc);

    /* Log connection to static channel */
    guac_client_log(svc->client, GUAC_LOG_INFO,
            "Static channel \"%s\" connected.", svc->name);

}

void guac_svc_process_terminate(rdpSvcPlugin* plugin) {

    /* Get corresponding guac_rdp_svc */
    guac_svcPlugin* svc_plugin = (guac_svcPlugin*) plugin;
    guac_rdp_svc* svc = svc_plugin->svc;

    /* Remove and free SVC */
    guac_client_log(svc->client, GUAC_LOG_INFO, "Closing channel \"%s\"...", svc->name);
    guac_rdp_remove_svc(svc->client, svc->name);
    free(svc);

    free(plugin);

}

void guac_svc_process_event(rdpSvcPlugin* plugin, wMessage* event) {
    freerdp_event_free(event);
}

void guac_svc_process_receive(rdpSvcPlugin* plugin,
        wStream* input_stream) {

    /* Get corresponding guac_rdp_svc */
    guac_svcPlugin* svc_plugin = (guac_svcPlugin*) plugin;
    guac_rdp_svc* svc = svc_plugin->svc;

    /* Fail if output not created */
    if (svc->output_pipe == NULL) {
        guac_client_log(svc->client, GUAC_LOG_ERROR,
                "Output for channel \"%s\" dropped.",
                svc->name);
        return;
    }

    /* Send blob */
    guac_protocol_send_blob(svc->client->socket, svc->output_pipe,
            Stream_Buffer(input_stream),
            Stream_Length(input_stream));

    guac_socket_flush(svc->client->socket);

}

