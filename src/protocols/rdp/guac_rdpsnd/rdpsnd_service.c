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

#include "rdpsnd_service.h"
#include "rdpsnd_messages.h"

#include <stdlib.h>
#include <string.h>

#include <freerdp/constants.h>
#include <freerdp/utils/svc_plugin.h>
#include <guacamole/client.h>

#ifdef ENABLE_WINPR
#include <winpr/stream.h>
#else
#include "compat/winpr-stream.h"
#endif

/**
 * Entry point for RDPSND virtual channel.
 */
int VirtualChannelEntry(PCHANNEL_ENTRY_POINTS pEntryPoints) {

    /* Allocate plugin */
    guac_rdpsndPlugin* rdpsnd =
        (guac_rdpsndPlugin*) calloc(1, sizeof(guac_rdpsndPlugin));

    /* Init channel def */
    strcpy(rdpsnd->plugin.channel_def.name, "rdpsnd");
    rdpsnd->plugin.channel_def.options = 
        CHANNEL_OPTION_INITIALIZED | CHANNEL_OPTION_ENCRYPT_RDP;

    /* Set callbacks */
    rdpsnd->plugin.connect_callback   = guac_rdpsnd_process_connect;
    rdpsnd->plugin.receive_callback   = guac_rdpsnd_process_receive;
    rdpsnd->plugin.event_callback     = guac_rdpsnd_process_event;
    rdpsnd->plugin.terminate_callback = guac_rdpsnd_process_terminate;

    /* Finish init */
    svc_plugin_init((rdpSvcPlugin*) rdpsnd, pEntryPoints);
    return 1;

}

/* 
 * Service Handlers
 */

void guac_rdpsnd_process_connect(rdpSvcPlugin* plugin) {

    guac_rdpsndPlugin* rdpsnd = (guac_rdpsndPlugin*) plugin;

    /* Get client from plugin parameters */
    guac_client* client = rdpsnd->client =
        (guac_client*) plugin->channel_entry_points.pExtendedData;

    /* NULL out pExtendedData so we don't lose our guac_client due to an
     * automatic free() within libfreerdp */
    plugin->channel_entry_points.pExtendedData = NULL;

#ifdef RDPSVCPLUGIN_INTERVAL_MS
    /* Update every 10 ms */
    plugin->interval_ms = 10;
#endif

    /* Log that sound has been loaded */
    guac_client_log(client, GUAC_LOG_INFO, "guacsnd connected.");

}

void guac_rdpsnd_process_terminate(rdpSvcPlugin* plugin) {
    free(plugin);
}

void guac_rdpsnd_process_event(rdpSvcPlugin* plugin, wMessage* event) {
    freerdp_event_free(event);
}

void guac_rdpsnd_process_receive(rdpSvcPlugin* plugin,
        wStream* input_stream) {

    guac_rdpsndPlugin* rdpsnd = (guac_rdpsndPlugin*) plugin;
    guac_rdpsnd_pdu_header header;

    /* Read RDPSND PDU header */
    Stream_Read_UINT8(input_stream, header.message_type);
    Stream_Seek_UINT8(input_stream);
    Stream_Read_UINT16(input_stream, header.body_size);

    /* 
     * If next PDU is SNDWAVE (due to receiving WaveInfo PDU previously),
     * ignore the header and parse as a Wave PDU.
     */
    if (rdpsnd->next_pdu_is_wave) {
        guac_rdpsnd_wave_handler(rdpsnd, input_stream, &header);
        return;
    }

    /* Dispatch message to standard handlers */
    switch (header.message_type) {

        /* Server Audio Formats and Version PDU */
        case SNDC_FORMATS:
            guac_rdpsnd_formats_handler(rdpsnd, input_stream, &header);
            break;

        /* Training PDU */
        case SNDC_TRAINING:
            guac_rdpsnd_training_handler(rdpsnd, input_stream, &header);
            break;

        /* WaveInfo PDU */
        case SNDC_WAVE:
            guac_rdpsnd_wave_info_handler(rdpsnd, input_stream, &header);
            break;

        /* Close PDU */
        case SNDC_CLOSE:
            guac_rdpsnd_close_handler(rdpsnd, input_stream, &header);
            break;

    }

}

