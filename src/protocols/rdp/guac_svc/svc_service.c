/*
 * Copyright (C) 2013 Glyptodon LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
    strncpy(svc_plugin->plugin.channel_def.name, svc->name,
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
    guac_protocol_send_pipe(svc->client->socket, svc->output_pipe,
            "application/octet-stream", svc->name);

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

