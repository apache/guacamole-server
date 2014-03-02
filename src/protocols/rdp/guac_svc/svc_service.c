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

#include "client.h"
#include "debug.h"
#include "svc_service.h"

#include <stdlib.h>
#include <string.h>

#include <freerdp/constants.h>
#include <freerdp/utils/svc_plugin.h>
#include <guacamole/client.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#include <winpr/wtypes.h>
#else
#include "compat/winpr-stream.h"
#include "compat/winpr-wtypes.h"
#endif

/**
 * Entry point for arbitrary SVC.
 */
int VirtualChannelEntry(PCHANNEL_ENTRY_POINTS pEntryPoints) {

    /* Allocate plugin */
    guac_svcPlugin* svc =
        (guac_svcPlugin*) calloc(1, sizeof(guac_svcPlugin));

    /* Init channel def */
    strcpy(svc->plugin.channel_def.name, "FIXME");
    svc->plugin.channel_def.options = 
          CHANNEL_OPTION_INITIALIZED
        | CHANNEL_OPTION_ENCRYPT_RDP
        | CHANNEL_OPTION_COMPRESS_RDP;

    /* Set callbacks */
    svc->plugin.connect_callback   = guac_svc_process_connect;
    svc->plugin.receive_callback   = guac_svc_process_receive;
    svc->plugin.event_callback     = guac_svc_process_event;
    svc->plugin.terminate_callback = guac_svc_process_terminate;

    /* Finish init */
    svc_plugin_init((rdpSvcPlugin*) svc, pEntryPoints);
    return 1;

}

/* 
 * Service Handlers
 */

void guac_svc_process_connect(rdpSvcPlugin* plugin) {

    /* Get SVC plugin */
    guac_svcPlugin* svc = (guac_svcPlugin*) plugin;

    /* Get client from plugin parameters */
    guac_client* client = (guac_client*)
        plugin->channel_entry_points.pExtendedData;

    /* NULL out pExtendedData so we don't lose our guac_client due to an
     * automatic free() within libfreerdp */
    plugin->channel_entry_points.pExtendedData = NULL;

    /* Get data from client */
    /*rdp_guac_client_data* client_data = (rdp_guac_client_data*) client->data;*/

    /* Init plugin */
    svc->client = client;

    /* Log that printing, etc. has been loaded */
    guac_client_log_info(client, "guacsvc connected.");

}

void guac_svc_process_terminate(rdpSvcPlugin* plugin) {
    free(plugin);
}

void guac_rdpdr_process_event(rdpSvcPlugin* plugin, wMessage* event) {
    freerdp_event_free(event);
}

void guac_rdpdr_process_receive(rdpSvcPlugin* plugin,
        wStream* input_stream) {
    /* STUB */
}

