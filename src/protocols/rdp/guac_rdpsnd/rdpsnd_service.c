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

#include "rdpsnd_service.h"
#include "rdpsnd_messages.h"

#include <stdlib.h>
#include <string.h>

#include <freerdp/constants.h>
#include <freerdp/utils/svc_plugin.h>
#include <guacamole/audio.h>
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

    /* Get audio stream from plugin parameters */
    guac_audio_stream* audio = rdpsnd->audio =
        (guac_audio_stream*) plugin->channel_entry_points.pExtendedData;

    /* NULL out pExtendedData so we don't lose our guac_audio_stream due to an
     * automatic free() within libfreerdp */
    plugin->channel_entry_points.pExtendedData = NULL;

#ifdef RDPSVCPLUGIN_INTERVAL_MS
    /* Update every 10 ms */
    plugin->interval_ms = 10;
#endif

    /* Log that sound has been loaded */
    guac_client_log(audio->client, GUAC_LOG_INFO, "guacsnd connected.");

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

    /* Get audio stream from plugin */
    guac_audio_stream* audio = rdpsnd->audio;

    /* Read RDPSND PDU header */
    Stream_Read_UINT8(input_stream, header.message_type);
    Stream_Seek_UINT8(input_stream);
    Stream_Read_UINT16(input_stream, header.body_size);

    /* 
     * If next PDU is SNDWAVE (due to receiving WaveInfo PDU previously),
     * ignore the header and parse as a Wave PDU.
     */
    if (rdpsnd->next_pdu_is_wave) {
        guac_rdpsnd_wave_handler(rdpsnd, audio, input_stream, &header);
        return;
    }

    /* Dispatch message to standard handlers */
    switch (header.message_type) {

        /* Server Audio Formats and Version PDU */
        case SNDC_FORMATS:
            guac_rdpsnd_formats_handler(rdpsnd, audio,
                    input_stream, &header);
            break;

        /* Training PDU */
        case SNDC_TRAINING:
            guac_rdpsnd_training_handler(rdpsnd, audio,
                    input_stream, &header);
            break;

        /* WaveInfo PDU */
        case SNDC_WAVE:
            guac_rdpsnd_wave_info_handler(rdpsnd, audio,
                    input_stream, &header);
            break;

        /* Close PDU */
        case SNDC_CLOSE:
            guac_rdpsnd_close_handler(rdpsnd, audio,
                    input_stream, &header);
            break;

    }

}

